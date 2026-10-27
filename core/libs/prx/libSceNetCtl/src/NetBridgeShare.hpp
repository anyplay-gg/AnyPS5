#ifndef CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGESHARE_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGESHARE_HPP

#include <cstdint>
#include <functional>
#include <vector>

namespace NetBridgeShare {

using Sender = std::function<void(std::uint16_t kind, const std::vector<std::uint8_t>& payload)>;

constexpr std::uint8_t ModeWatch = 0;
constexpr std::uint8_t ModeHandOver = 1;
constexpr std::uint8_t ModeTogether = 2;

constexpr std::uint8_t OpenMapped = 0;
constexpr std::uint8_t OpenCannot = 1;
constexpr std::uint8_t OpenRunning = 2;
constexpr std::uint8_t OpenNotPresenting = 3;
// Not a reply: a payload too short for a share open is answered with `unsupported`.
constexpr std::uint8_t OpenUnsupported = 0xFF;

constexpr std::uint8_t ActionTookControl = 1;
constexpr std::uint8_t ActionStop = 2;

void Attach(Sender sender);
std::uint8_t Open(const std::vector<std::uint8_t>& payload);
void Close();
void Mode(const std::vector<std::uint8_t>& payload);
void Pad(const std::vector<std::uint8_t>& payload);

}

#endif
