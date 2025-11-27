// Generated automatically. DO NOT EDIT!
// SoupBinTCP Packet Struct Definitions
#pragma once

#include "common.h"

#include <cstdint>
/*
struct DebugPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'P';            // Debug Packet (Type +, P:PLUS) | offset: 2, len: 1
    char text[];                       // Free form human readable text | offset: 3, len: 
} __attribute__((packed));

struct LoginAcceptedPacket {
    uint16_t packet_length = 31;          // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'A';               // Login Accepted Packet | offset: 2, len: 1
    char session[10] = {};                // Session ID (left padded with spaces) | offset: 3, len: 10
    char sequence_number[20] = {};        // Sequence number in ASCII (left padded with spaces) | offset: 13, len: 20
} __attribute__((packed));

struct LoginRejectedPacket {
    uint16_t packet_length = 2;            // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'J';                // Login Rejected Packet | offset: 2, len: 1
    char reject_reason_code = '\0';        // Reject reason (A=Not Authorized, S=Session not available) | offset: 3, len: 1
} __attribute__((packed));

struct SequencedDataPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'S';            // Sequenced Data Packet | offset: 2, len: 1
    char message[];                    // Defined by higher-level protocol | offset: 3, len: 
} __attribute__((packed));

struct UnsequencedDataPacketFromServer {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'U';            // Unsequenced Data Packet | offset: 2, len: 1
    char message[];                    // Defined by higher-level protocol | offset: 3, len: 
} __attribute__((packed));

struct ServerHeartbeatPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'H';            // Server Heartbeat Packet | offset: 2, len: 1
} __attribute__((packed));

struct EndOfSessionPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'Z';            // End of Session Packet | offset: 2, len: 1
} __attribute__((packed));

struct LoginRequestPacket {
    uint16_t packet_length = 47;                    // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'L';                         // Login Request Packet | offset: 2, len: 1
    char username[6] = {};                          // Username (right padded with spaces) | offset: 3, len: 6
    char password[10] = {};                         // Password (right padded with spaces) | offset: 9, len: 10
    char requested_session[10] = {};                // Session ID or blanks for current session | offset: 19, len: 10
    char requested_sequence_number[20] = {};        // Next sequence number in ASCII or 0 for most recent | offset: 29, len: 20
} __attribute__((packed));

struct UnsequencedDataPacketFromClient {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'u';            // Unsequenced Data Packet | offset: 2, len: 1
    char message[];                    // Defined by higher-level protocol | offset: 3, len: 
} __attribute__((packed));

struct ClientHeartbeatPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'R';            // Client Heartbeat Packet | offset: 2, len: 1
} __attribute__((packed));

struct LogoutRequestPacket {
    uint16_t packet_length = 1;        // Number of bytes after this field until the next packet | offset: 0, len: 2
    char packet_type = 'O';            // Logout Request Packet | offset: 2, len: 1
} __attribute__((packed));
*/
enum class SBTTypes : uint8_t {   P = 0,  A = 1,  J = 2,  S = 3,  U = 4,  H = 5,  Z = 6,  L = 7,  u = 8,  R = 9,  O = 10,  unknownSBTtype = 99 };

inline constexpr SBTTypes PacketIndex(char type) {

   if(UNLIKELY(type == '+')) type = 'P'; 

   switch(type) {
      case 'P': return SBTTypes::P;
      case 'A': return SBTTypes::A;
      case 'J': return SBTTypes::J;
      case 'S': return SBTTypes::S;
      case 'U': return SBTTypes::U;
      case 'H': return SBTTypes::H;
      case 'Z': return SBTTypes::Z;
      case 'L': return SBTTypes::L;
      case 'u': return SBTTypes::u;
      case 'R': return SBTTypes::R;
      case 'O': return SBTTypes::O;
      default: return SBTTypes::unknownSBTtype;
   }
}