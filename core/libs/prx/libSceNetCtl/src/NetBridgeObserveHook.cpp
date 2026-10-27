
#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libc/include/CallTrace.hpp"

#include <nid/NidCompute.hpp>
#include <nid/NidPatcherUtils.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct ExportRow {
    const char* library;
    const char* name;
    const char* nid;
    char returns;
};

constexpr ExportRow ExportRows[] = {
#include "ObservedExports.inc"
};

struct ArgumentRow {
    const char* name;
    const char* spec;
};

constexpr ArgumentRow ArgumentRows[] = {
#include "ObservedArguments.inc"
};

enum class Kind : std::uint8_t { I8, U8, I16, U16, I32, U32, I64, U64, Text };

struct Argument {
    std::string name;
    Kind kind;
    std::uint8_t reg;
};

struct Export {
    std::string library;
    std::string name;
    char returns;
    std::vector<Argument> arguments;
};

struct CallState {
    std::string in;
    std::uint64_t cut;
};

bool ParseKind(std::string_view text, Kind& kind) {
    static constexpr std::pair<std::string_view, Kind> Kinds[] = {
        {"i8", Kind::I8}, {"u8", Kind::U8}, {"i16", Kind::I16}, {"u16", Kind::U16}, {"i32", Kind::I32},
        {"u32", Kind::U32}, {"i64", Kind::I64}, {"u64", Kind::U64}, {"text", Kind::Text},
    };
    for (const auto& [name, value] : Kinds) {
        if (name == text) {
            kind = value;
            return true;
        }
    }
    return false;
}

std::vector<Argument> ParseArguments(std::string_view spec) {
    std::vector<Argument> arguments;
    while (!spec.empty()) {
        const auto comma = spec.find(',');
        std::string_view item = spec.substr(0, comma);
        spec = comma == std::string_view::npos ? std::string_view{} : spec.substr(comma + 1);
        const auto first = item.find(':');
        const auto second = first == std::string_view::npos ? first : item.find(':', first + 1);
        Kind kind = Kind::I32;
        if (second == std::string_view::npos || first != 1 || item[0] < '0' || item[0] > '5' || !ParseKind(item.substr(first + 1, second - first - 1), kind)) continue;
        arguments.push_back({std::string(item.substr(second + 1)), kind, static_cast<std::uint8_t>(item[0] - '0')});
    }
    return arguments;
}

class Table {
public:
    Table() {
        std::unordered_map<std::string_view, std::string_view> specs;
        for (const auto& row : ArgumentRows) specs.emplace(row.name, row.spec);
        for (const auto& row : ExportRows) {
            const std::string name = row.name;
            if (Nid::Internal::IsNidNoPatch(name)) continue;
            std::string nid = row.nid != nullptr ? row.nid : Nid::ComputeNid(Nid::Internal::StripNidPostfix(name), "");
            const auto spec = specs.find(name);
            auto& entry = exports.emplace_back(Export{row.library, name, row.returns, spec == specs.end() ? std::vector<Argument>{} : ParseArguments(spec->second)});
            byNid.emplace(std::move(nid), &entry);
            byName.emplace(entry.library + "/" + entry.name, &entry);
        }
    }

    Export* FindNid(std::string_view nid) const {
        nid = nid.substr(0, nid.find('#'));
        const auto found = byNid.find(std::string(nid));
        return found == byNid.end() ? nullptr : found->second;
    }

    Export* FindName(std::string_view library, std::string_view name) const {
        const auto found = byName.find(std::string(library) + "/" + std::string(name));
        return found == byName.end() ? nullptr : found->second;
    }

    std::size_t Count(std::string_view library) const {
        std::size_t count = 0;
        for (const auto& entry : exports) count += entry.library == library;
        return count;
    }

    Export* Unknown(std::string_view library, std::string_view nid) const {
        if (Count(library) == 0) return nullptr;
        nid = nid.substr(0, nid.find('#'));
        std::lock_guard lock(unknownMutex);
        const std::string key = std::string(library) + "/" + std::string(nid);
        auto& entry = unknown[key];
        if (entry == nullptr) entry = new Export{std::string(library), std::string(nid), 'i', {}};
        return entry;
    }

private:
    std::deque<Export> exports;
    std::unordered_map<std::string, Export*> byNid;
    std::unordered_map<std::string, Export*> byName;
    mutable std::mutex unknownMutex;
    mutable std::unordered_map<std::string, Export*> unknown;
};

const Table& Exports() {
    static const Table table;
    return table;
}

std::string_view Stem(std::string_view library) {
    if (library.ends_with(".prx")) library.remove_suffix(4);
    return library;
}

void AppendArgument(NetBridgeJson& json, const Argument& argument, const std::uint64_t value) {
    switch (argument.kind) {
    case Kind::I8: json.Int(argument.name, static_cast<std::int8_t>(value)); break;
    case Kind::U8: json.Uint(argument.name, static_cast<std::uint8_t>(value)); break;
    case Kind::I16: json.Int(argument.name, static_cast<std::int16_t>(value)); break;
    case Kind::U16: json.Uint(argument.name, static_cast<std::uint16_t>(value)); break;
    case Kind::I32: json.Int(argument.name, static_cast<std::int32_t>(value)); break;
    case Kind::U32: json.Uint(argument.name, static_cast<std::uint32_t>(value)); break;
    case Kind::I64: json.Int(argument.name, static_cast<std::int64_t>(value)); break;
    case Kind::U64: json.Uint(argument.name, value); break;
    case Kind::Text: json.Text(argument.name, reinterpret_cast<const char*>(value)); break;
    }
}

void* APS5_VABI Select(const char* library, const char* name) {
    if (NetBridgeObserveEnabled_nid_no_patch() == 0) return nullptr;
    if (Export* known = Exports().FindNid(name)) return known;
    return Exports().Unknown(Stem(library), name);
}

void* APS5_VABI Enter(void* cookie, const std::uint64_t* arguments) {
    if (NetBridgeObserveEnabled_nid_no_patch() == 0) return nullptr;
    const auto& entry = *static_cast<const Export*>(cookie);
    NetBridgeJson in;
    for (const auto& argument : entry.arguments) AppendArgument(in, argument, arguments[argument.reg]);
    return new CallState{in.Finish(), in.Cut()};
}

void APS5_VABI Leave(void* cookie, void* state, std::uint64_t result, int claimed) {
    const std::unique_ptr<CallState> call(static_cast<CallState*>(state));
    if (call == nullptr || claimed != 0) return;
    const auto& entry = *static_cast<const Export*>(cookie);
    NetBridgeJson out;
    if (entry.returns == 'b') out.Bool("value", (result & 0xFFu) != 0);
    const std::string outText = out.Finish();
    const NetBridgeObservation observation{entry.library.c_str(), entry.name.c_str(), call->in.c_str(), outText.c_str(), static_cast<std::int32_t>(result), entry.returns == 'i' ? 1 : 0, std::max(call->cut, out.Cut())};
    NetBridgeObserve_nid_no_patch(&observation);
}

const CallObserver g_observer{&Select, &Enter, &Leave};

const bool g_observerRegistered = [] {
    SetCallObserver_nid_no_patch(&g_observer);
    return true;
}();

}

extern "C" int NetBridgeObserveClaim_nid_no_patch(const char* library, const char* function) {
    if (library == nullptr || function == nullptr) return 0;
    void* cookie = Exports().FindName(library, function);
    return cookie != nullptr ? CallObserverClaim_nid_no_patch(cookie) : 0;
}

extern "C" void* NetBridgeObserveImport_nid_no_patch(void* function, const char* library, const char* name) {
    if (function == nullptr || library == nullptr || name == nullptr) return function;
    const std::string nid = Nid::ComputeNid(Nid::Internal::StripNidPostfix(name), "");
    const std::string module = std::string(library) + ".prx";
    return TraceImport_nid_no_patch(function, nid.c_str(), module.c_str());
}

extern "C" std::size_t NetBridgeObservedExports_nid_no_patch(const char* library) {
    return library == nullptr ? 0 : Exports().Count(Stem(library));
}

extern "C" int NetBridgeObservedExport_nid_no_patch(const char* library, const char* name) {
    return library != nullptr && name != nullptr && Exports().FindName(Stem(library), name) != nullptr ? 1 : 0;
}
