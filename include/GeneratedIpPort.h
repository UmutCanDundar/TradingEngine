// Generated automatically. DO NOT EDIT!

#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "Protocol-Venue.h"

inline constexpr size_t PORTS_COUNT = 1;

inline constexpr size_t IPs_COUNT = 2;

inline constexpr std::array<uint16_t, 1> Ports {
    1304,
};

inline const std::array<std::vector<uint32_t>, PORTS_COUNT> IPs {
    std::vector<uint32_t>{0x0B0B057A, 0x0C0C037B}, // Port1304: {'122.5.11.11', '123.3.12.12'}
};

inline constexpr std::array<Protocol, PORTS_COUNT> PortProtocol {
    Protocol::FIX, // Port 1304
};
inline constexpr std::array<Venue, PORTS_COUNT> PortVenue {
    Venue::BIST, // Port 1304
};
