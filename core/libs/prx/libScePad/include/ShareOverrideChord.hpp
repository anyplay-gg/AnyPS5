#ifndef CORE_LIBS_PRX_LIBSCEPAD_SHAREOVERRIDECHORD_HPP
#define CORE_LIBS_PRX_LIBSCEPAD_SHAREOVERRIDECHORD_HPP

#include <chrono>
#include <cstdint>

namespace Pad {

class ShareOverrideChord {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::uint32_t SelectBit = 0x100000;
    static constexpr std::uint32_t StartBit = 0x0008;
    static constexpr std::chrono::milliseconds HoldTime{1000};

    static bool Held(std::uint32_t hostButtons) {
        return (hostButtons & (SelectBit | StartBit)) == (SelectBit | StartBit);
    }

    bool Advance(bool held, Clock::time_point now) {
        if (!held) {
            running = false;
            fired = false;
            return false;
        }
        if (!running) {
            running = true;
            since = now;
        }
        if (fired || now - since < HoldTime) return false;
        fired = true;
        return true;
    }

private:
    Clock::time_point since{};
    bool running = false;
    bool fired = false;
};

}

#endif
