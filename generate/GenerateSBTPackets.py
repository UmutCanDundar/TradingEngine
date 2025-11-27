packets = [
    # ==================== OUTBOUND MESSAGES ====================
    {
        "name": "DebugPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'P'", "Debug Packet (Type +, P:PLUS) | offset: 2, len: 1"),
            ("char text[]", None, "Free form human readable text | offset: 3, len: "),
        ],
    },
    {
        "name": "LoginAcceptedPacket",
        "fields": [
            ("uint16_t packet_length", "31", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'A'", "Login Accepted Packet | offset: 2, len: 1"),
            ("char session[10]", "{}", "Session ID (left padded with spaces) | offset: 3, len: 10"),
            ("char sequence_number[20]", "{}", "Sequence number in ASCII (left padded with spaces) | offset: 13, len: 20"),
        ],
    },
    {
        "name": "LoginRejectedPacket",
        "fields": [
            ("uint16_t packet_length", "2", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'J'", "Login Rejected Packet | offset: 2, len: 1"),
            ("char reject_reason_code", "'\\0'", "Reject reason (A=Not Authorized, S=Session not available) | offset: 3, len: 1"),
        ],
    },
    {
        "name": "SequencedDataPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'S'", "Sequenced Data Packet | offset: 2, len: 1"),
            ("char message[]", None, "Defined by higher-level protocol | offset: 3, len: "),
        ],
    },
    {
        "name": "UnsequencedDataPacketFromServer",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'U'", "Unsequenced Data Packet | offset: 2, len: 1"),
            ("char message[]", None, "Defined by higher-level protocol | offset: 3, len: "),
        ],
    },
    {
        "name": "ServerHeartbeatPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'H'", "Server Heartbeat Packet | offset: 2, len: 1"),
        ],
    },
    {
        "name": "EndOfSessionPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'Z'", "End of Session Packet | offset: 2, len: 1"),
        ],
    },

    # ==================== INBOUND MESSAGES ====================
    {
        "name": "LoginRequestPacket",
        "fields": [
            ("uint16_t packet_length", "47", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'L'", "Login Request Packet | offset: 2, len: 1"),
            ("char username[6]", "{}", "Username (right padded with spaces) | offset: 3, len: 6"),
            ("char password[10]", "{}", "Password (right padded with spaces) | offset: 9, len: 10"),
            ("char requested_session[10]", "{}", "Session ID or blanks for current session | offset: 19, len: 10"),
            ("char requested_sequence_number[20]", "{}", "Next sequence number in ASCII or 0 for most recent | offset: 29, len: 20"),
        ],
    },
    {
        "name": "UnsequencedDataPacketFromClient",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'u'", "Unsequenced Data Packet | offset: 2, len: 1"),
            ("char message[]", None, "Defined by higher-level protocol | offset: 3, len: "),
        ],
    },
    {
        "name": "ClientHeartbeatPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'R'", "Client Heartbeat Packet | offset: 2, len: 1"),
        ],
    },
    {
        "name": "LogoutRequestPacket",
        "fields": [
            ("uint16_t packet_length", "1", "Number of bytes after this field until the next packet | offset: 0, len: 2"),
            ("char packet_type", "'O'", "Logout Request Packet | offset: 2, len: 1"),
        ],
    },
]

def generate_struct(pkt):
    struct_lines = [f"struct {pkt['name']} {{"]
    
    max_field_len = 0
    for field, default, comment in pkt["fields"]:
        if default is not None:
            length = len(field) + len(default) + 3
            if length > max_field_len:
                max_field_len = length
    comment_col = max_field_len + 8  
    
    for field_type, default, comment in pkt["fields"]:
        if default is None:
            line = f"    {field_type};"
            spaces = comment_col - len(field_type)
        else:
            line = f"    {field_type} = {default};"
            spaces = comment_col - (len(field_type) + len(default) + 3)
        
        line += " " * spaces + f"// {comment}"
        struct_lines.append(line)
    struct_lines.append("} __attribute__((packed));\n")
    return "\n".join(struct_lines)


def generate_file(packets):
    lines = [
        "// Generated automatically. DO NOT EDIT!",
        "// SoupBinTCP Packet Struct Definitions",
        "#pragma once",
        "",
        "#include \"common.h\"",
        "",
        "#include <cstdint>",
        "/*",
    ]
    for pkt in packets:
        lines.append(generate_struct(pkt))
    return "\n".join(lines)

def packettype_index(pkt):
    for i, field in enumerate(pkt["fields"]):
       if field[0] == "char packet_type":
        return i  

def generate_enum(packets):
     line = [f"\nenum class SBTTypes : uint8_t {{ "]
     for i, pkt in enumerate(packets):
         packettype = packettype_index(pkt)
         default = pkt["fields"][packettype][1]
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownSBTtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func(packets):
    line = [f"\n\ninline constexpr SBTTypes PacketIndex(char type) {{\n"]
    line.append("   if(UNLIKELY(type == '+')) type = 'P'; \n")
    line.append("   switch(type) {")
    for i, pkt in enumerate(packets):
        packettype = packettype_index(pkt)
        default = pkt["fields"][packettype][1]  
        line.append(f"      case {default}: return SBTTypes::{default.strip("'")};")
    line.append(f"      default: return SBTTypes::unknownSBTtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 

file = generate_file(packets)
with open("../include/GeneratedSBTPackets.h", "w") as f:
    f.write(file)
    f.write("*/")
    f.write(generate_enum(packets))
    f.write(generate_func(packets))
    




