#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_CALLTRACE_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_CALLTRACE_HPP

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

struct CallTraceEntry {
    std::uint64_t sequence;
    std::uint64_t thread;
    std::uint64_t returnAddress;
    const char* name;
    const char* library;
};

struct CallTraceCount {
    std::uint64_t calls;
    const char* name;
    const char* library;
};

struct CallObserver {
    void* (APS5_VABI* select)(const char* library, const char* name);
    void* (APS5_VABI* enter)(void* cookie, const std::uint64_t* arguments);
    void (APS5_VABI* leave)(void* cookie, void* state, std::uint64_t result, int claimed);
};

extern "C" {
void* TraceImport_nid_no_patch(void* target, const char* name, const char* library);
void SetCallObserver_nid_no_patch(const CallObserver* observer);
int CallObserverClaim_nid_no_patch(void* cookie);
std::size_t CopyCallTrace_nid_no_patch(CallTraceEntry* entries, std::size_t capacity);
std::size_t CopyThreadCallTrace_nid_no_patch(std::uint64_t thread, CallTraceEntry* entries, std::size_t capacity);
std::size_t CopyCallCounts_nid_no_patch(CallTraceCount* entries, std::size_t capacity, std::uint64_t* total);
bool WriteCallCounts_nid_no_patch(const char* path);
}

#endif
