import socket
import struct

PORTS = {1304}
PORTS_COUNT = len(PORTS)

PortProtocol = {
     1304: "FIX"
}

PortVenue = {
     1304: "BIST"
}

IPs = {
    1304: {"123.0.12.12", "122.05.11.11"},
}
IPs_COUNT = sum(len(ips) for ips in IPs.values())

IPs_to_int32_and_nl = {}
for port, ips in IPs.items():
    int32_nl_ips = set()
    for ip in ips:
      ip_binary = socket.inet_aton(ip)
      ip_int32 = struct.unpack("!I",ip_binary)[0]
      ip_nl=socket.htonl(ip_int32)
      int32_nl_ips.add(ip_nl)
    IPs_to_int32_and_nl[port] = int32_nl_ips

with open("../include/GeneratedIpPort.h", "w") as f:
    f.write("// Generated automatically. DO NOT EDIT!\n\n")
    f.write("#pragma once\n")
    f.write("#include <array>\n#include <cstdint>\n#include <vector>\n#include \"Protocol-Venue.h\"\n\n")
    
    f.write(f"inline constexpr size_t PORTS_COUNT = {PORTS_COUNT};\n\n")
    f.write(f"inline constexpr size_t IPs_COUNT = {PORTS_COUNT};\n\n")
   

    f.write(f"inline constexpr std::array<uint16_t, {PORTS_COUNT}> Ports {{\n")
    for port in PORTS:
        f.write(f"    {port},\n")
    f.write("};\n\n")

    f.write(f"inline const std::array<std::vector<uint32_t>, PORTS_COUNT> IPs {{\n")
    for port in IPs.keys():
            ips_hex = [f"0x{ip:08X}" for ip in IPs_to_int32_and_nl[port]]
            ips_str = ", ".join(ips_hex)
            f.write(f"    std::vector<uint32_t>{{{ips_str}}}, // Port{port}: {IPs[port]}\n")
    f.write(f"}};\n\n")

    f.write(f"inline constexpr std::array<Protocol, PORTS_COUNT> PortProtocol {{\n")
    for port in PortProtocol.keys():
            f.write(f"    Protocol::{PortProtocol[port]}, // Port {port}\n")
    f.write(f"}};\n")

    f.write(f"inline constexpr std::array<Venue, PORTS_COUNT> PortVenue {{\n")
    for port in PortVenue.keys():
            f.write(f"    Venue::{PortVenue[port]}, // Port {port}\n")
    f.write(f"}};\n")
