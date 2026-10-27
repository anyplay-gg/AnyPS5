#include "prx/libScePad/include/ShareOverrideChord.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace {

using Clock = Pad::ShareOverrideChord::Clock;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "share override chord test: %s\n", message);
        std::abort();
    }
}

Clock::time_point At(int milliseconds) {
    return Clock::time_point{} + std::chrono::milliseconds(milliseconds);
}

}

int main() {
    Require(Pad::ShareOverrideChord::Held(0x100000 | 0x0008), "select and start are the chord");
    Require(!Pad::ShareOverrideChord::Held(0x100000), "select alone is not the chord");
    Require(!Pad::ShareOverrideChord::Held(0x0008 | 0x4000), "start with another button is not the chord");
    Require(Pad::ShareOverrideChord::Held(0x100000 | 0x0008 | 0x4000), "other buttons held with the chord do not break it");

    Pad::ShareOverrideChord wired;
    const std::uint32_t both = Pad::ShareOverrideChord::SelectBit | Pad::ShareOverrideChord::StartBit;
    Require(!wired.Advance(false, true, both, At(0)) && !wired.Advance(false, true, both, At(2000)), "no share running: the chord does nothing");
    Require(!wired.Advance(true, false, both, At(3000)) && !wired.Advance(true, false, both, At(5000)), "no controller in slot 0: the chord does nothing");
    Require(!wired.Advance(true, true, both, At(6000)) && wired.Advance(true, true, both, At(7000)), "share, controller and chord: fires after a second");

    Pad::ShareOverrideChord chord;
    Require(!chord.Advance(false, At(0)), "nothing held does nothing");
    Require(!chord.Advance(true, At(0)), "the chord has just been pressed");
    Require(!chord.Advance(true, At(500)), "half a second is not enough");
    Require(!chord.Advance(true, At(999)), "one millisecond short");
    Require(chord.Advance(true, At(1000)), "one second fires");
    Require(!chord.Advance(true, At(1500)), "holding on fires only once");
    Require(!chord.Advance(true, At(5000)), "still only once");
    Require(!chord.Advance(false, At(5100)), "release");
    Require(!chord.Advance(true, At(5200)), "a new press starts over");
    Require(chord.Advance(true, At(6200)), "and fires again after a second");

    Pad::ShareOverrideChord interrupted;
    Require(!interrupted.Advance(true, At(0)), "start");
    Require(!interrupted.Advance(true, At(700)), "before the second");
    Require(!interrupted.Advance(false, At(800)), "let go");
    Require(!interrupted.Advance(true, At(900)), "press again");
    Require(!interrupted.Advance(true, At(1500)), "the first press does not count");
    Require(interrupted.Advance(true, At(1900)), "a second after the second press");

    std::puts("share override chord: ok");
    return 0;
}
