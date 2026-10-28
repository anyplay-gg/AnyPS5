#ifndef CORE_LIBS_PRX_LIBSCEPAD_SHAREPADMAPPING_HPP
#define CORE_LIBS_PRX_LIBSCEPAD_SHAREPADMAPPING_HPP

#include <cstdint>

#include "prx/libSceNetCtl/include/NetBridge.hpp"
#include "prx/libScePad/include/PadState.hpp"

namespace Pad::Share {

constexpr std::uint32_t DpadUp = 1u << 0;
constexpr std::uint32_t DpadDown = 1u << 1;
constexpr std::uint32_t DpadLeft = 1u << 2;
constexpr std::uint32_t DpadRight = 1u << 3;
constexpr std::uint32_t South = 1u << 4;
constexpr std::uint32_t East = 1u << 5;
constexpr std::uint32_t West = 1u << 6;
constexpr std::uint32_t North = 1u << 7;
constexpr std::uint32_t L1 = 1u << 8;
constexpr std::uint32_t R1 = 1u << 9;
constexpr std::uint32_t L3 = 1u << 10;
constexpr std::uint32_t R3 = 1u << 11;
constexpr std::uint32_t Start = 1u << 12;
constexpr std::uint32_t Select = 1u << 13;
constexpr std::uint32_t PadClick = 1u << 14;

constexpr std::uint8_t ModeHandOver = 1;
constexpr std::uint8_t ModeTogether = 2;

constexpr int PortOf(std::uint8_t mode) {
    return mode == ModeTogether ? 1 : 0;
}

inline std::uint32_t EngineButtons(std::uint32_t neutral) {
    struct Entry {
        std::uint32_t neutral;
        std::uint32_t engine;
    };
    static constexpr Entry Table[] = {
        {DpadUp, 0x0010}, {DpadDown, 0x0040}, {DpadLeft, 0x0080}, {DpadRight, 0x0020}, {South, 0x4000}, {East, 0x2000}, {West, 0x8000}, {North, 0x1000},
        {L1, 0x0400}, {R1, 0x0800}, {L3, 0x0002}, {R3, 0x0004}, {Start, 0x0008}, {PadClick, 0x100000},
    };
    std::uint32_t engine = 0;
    for (const auto& entry : Table) {
        if ((neutral & entry.neutral) != 0) engine |= entry.engine;
    }
    return engine;
}

inline PadInputState ToInputState(const NetBridgeSharePadView& view) {
    PadInputState state;
    state.buttons = EngineButtons(view.buttons);
    for (int axis = 0; axis < 4; ++axis) state.sticks[static_cast<std::size_t>(axis)] = view.sticks[axis];
    state.analogButtonsL2 = view.l2;
    state.analogButtonsR2 = view.r2;
    if (view.l2 != 0) state.buttons |= 0x0100;
    if (view.r2 != 0) state.buttons |= 0x0200;
    state.deviceKind = 1;
    return state;
}

}

#endif
