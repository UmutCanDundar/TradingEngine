// Generated automatically. DO NOT EDIT!

#pragma once
#include <array>
#include <cstdint>
#include <vector>

inline constexpr size_t PORTS_COUNT = 1;

inline constexpr std::array<uint16_t, 1> Ports {
    1304,
};

inline const std::array<std::vector<uint32_t>, PORTS_COUNT> IPs {
    std::vector<uint32_t>{0x0B0B057A, 0x0C0C007B}, // Port1304: {'122.05.11.11', '123.0.12.12'}
};

enum class Protocol : uint8_t { ITCH, SBE, FIX, };

inline constexpr std::array<Protocol, PORTS_COUNT> PortProtocol {
    Protocol::FIX, // Port 1304
};
