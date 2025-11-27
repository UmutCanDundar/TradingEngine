messages = [

     # ==================== INBOUND MESSAGES ====================

    {
        "name": "OUCHEnterOrderMessage",
        "fields": [
            ("uint64_t quantity", "0", "Order quantity | offset: 20, len: 8"),
            ("uint64_t display_quantity", "0", "Displayed qty (0=unchanged) | offset: 97, len: 8"),
            ("uint32_t order_book_id", "0", "Order book identifier | offset: 15, len: 4"),
            ("int32_t price", "0", "Signed price | offset: 28, len: 4"),
            ("char message_type", "'O'", "Enter Order Message | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Client-generated order identifier | offset: 1, len: 14"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 19, len: 1"),
            ("uint8_t time_in_force", "0", "0=Day, 3=IOC, 4=FOK | offset: 32, len: 1"),
            ("uint8_t open_close", "0", "0=Default, 1=Open, 2=Close | offset: 33, len: 1"),
            ("char client_account[16]", "{}", "Client/Account (AFK code) for DM | offset: 34, len: 16"),
            ("uint8_t client_category", "0", "1=Client,2=House,7=Fund for EQM | offset: 105, len: 1"),
            ("uint8_t offhours", "0", "0=normal,1=off for DM | offset: 106, len: 1"),
            ("uint8_t smp_level", "0", "scope of SMP | offset: 107, len: 1"),
            ("uint8_t smp_method", "0", "the action being undertaken by the trading system in case of a potential prevention | offset: 108, len: 1"),
            ("char reserverd[2]", None, "Reserved | offset: 112, len: 2"),
            ("char customer_info[15]", "{}", "Client reference | offset: 50, len: 15"),
            ("char exchange_info[32]", "{}", "Client account number/only first 16 bytes for EQM | offset: 65, len: 32"),            
            ("char smp_ID[3]", "{}", "the order is eligible for self-match prevention | offset: 109, len: 3"),
            ("char pad[2]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHReplaceOrderMessage",
        "fields": [
            ("uint64_t quantity", "0", "Desired open quantity | offset: 29, len: 8"),
            ("uint64_t display_quantity", "0", "Displayed qty (0=unchanged) | offset: 105, len: 8"),
            ("int32_t price", "0", "New price (0=no change) | offset: 37, len: 4"),
            ("char message_type", "'u'", "Replace Order Message | offset: 0, len: 1"),
            ("char existing_order_token[14]", "{}", "Existing order token | offset: 1, len: 14"),
            ("char replacement_order_token[14]", "{}", "Replacement order token | offset: 15, len: 14"),
            ("char customer_info[15]", "{}", "Pass-thru field | offset: 58, len: 15"),
            ("uint8_t open_close", "0", "0=No change,1=Open,2=Close | offset: 41, len: 1"),
            ("char client_account[16]", "{}", "Client/Account for DM | offset: 42, len: 16"),
            ("char exchange_info[32]", "{}", "Account number (first 16 used) for EQM | offset: 73, len: 32"),
            ("uint8_t client_category", "0", "1=Client,2=House,7=Fund for EQM | offset: 113, len: 1"),
            ("char reserved[8]", None, "Reserved | offset: 114, len: 8"),
            ("char pad[8]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHCancelOrderMessage",
        "fields": [
            ("char message_type", "'X'", "Cancel Order Message | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Order token to cancel | offset: 1, len: 14"),
            ("char pad[49]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHCancelByOrderIDMessage",
        "fields": [
            ("uint64_t order_id", "0", "Exchange-assigned ID | offset: 6, len: 8"),
            ("uint32_t order_book_id", "0", "Order book identifier | offset: 1, len: 4"),
            ("char message_type", "'Y'", "Cancel Order Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 5, len: 1"),
            ("char pad[50]", None, "Padding"),
        ],
    },

    # ==================== OUTBOUND MESSAGES ====================

    {
        "name": "OUCHOrderAcceptedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "UNIX ns timestamp | offset: 1, len: 8"),
            ("uint64_t order_id", "0", "Exchange-assigned ID | offset: 28, len: 8"),
            ("uint64_t quantity", "0", "Open quantity | offset: 36, len: 8"),
            ("uint64_t pre_trade_qty", "0", "Pre-trade quantity | offset: 114, len: 8"),
            ("uint64_t display_qty", "0", "Displayed quantity | offset: 122, len: 8"),
            ("uint32_t order_book_id", "0", "Book ID | offset: 23, len: 4"),
            ("int32_t price", "0", "Price | offset: 44, len: 4"),
            ("char message_type", "'A'", "Order Accepted | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Echo of client token | offset: 9, len: 14"),
            ("char side", "'\\0'", "B/S/T | offset: 27, len: 1"),
            ("uint8_t time_in_force", "0", "TIF | offset: 48, len: 1"),
            ("char client_account[16]", "{}", "Account info for DM | offset: 50, len: 16"),
            ("char customer_info[15]", "{}", "Pass-thru | offset: 67, len: 15"),
            ("char exchange_info[32]", "{}", "Account info for EQM | offset: 82, len: 32"),
            ("uint8_t open_close", "0", "Open/Close flag | offset: 49, len: 1"),
            ("uint8_t order_state", "0", "1=OnBook,2=NotOnBook,98=Paused | offset: 66, len: 1"),
            ("uint8_t client_category", "0", "1=Client,2=House,7=Fund for EQM | offset: 130, len: 1"),
            ("uint8_t offhours", "0", "0=normal,1=off for DM | offset: 131, len: 1"),
            ("uint8_t smp_level", "0", "scope of SMP | offset: 132, len: 1"),
            ("uint8_t smp_method", "0", "the action being undertaken by the trading system in case of a potential prevention | offset: 133, len: 1"),
            ("char smp_ID[3]", "{}", "the order is eligible for self-match prevention | offset: 134, len: 3"),
            ("char pad[55]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHOrderRejectedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "UNIX ns timestamp | offset: 1, len: 8"),
            ("int32_t reject_code", "0", "Signed reason code | offset: 23, len: 4"),
            ("char message_type", "'J'", "Order Rejected | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Echo of token | offset: 9, len: 14"),   
            ("char pad[37]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHOrderReplacedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "UNIX ns timestamp | offset: 1, len: 8"),
            ("uint64_t order_id", "0", "New order ID | offset: 42, len: 8"),
            ("uint64_t quantity", "0", "Open quantity | offset: 50, len: 8"),
            ("uint64_t pre_trade_qty", "0", "Pre-trade quantity | offset: 128, len: 8"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 37, len: 4"),
            ("int32_t price", "0", "Signed price | offset: 58, len: 4"),
            ("char message_type", "'U'", "Order Replaced | offset: 0, len: 1"),
            ("char previous_order_token[14]", "{}", "Previous Order Token | offset: 23, len: 14"),
            ("char side", "'\\0'", "Side (B/S) | offset: 41, len: 1"),
            ("uint8_t time_in_force", "0", "Time-in-Force | offset: 62, len: 1"),
            ("uint8_t open_close", "0", "0=No change,1=Open,2=Close,4=Default | offset: 63, len: 1"),
            ("uint8_t order_state", "0", "1=On book,2=Not on book,98=Paused,99=Ownership lost | offset: 80, len: 1"),
            ("uint8_t client_category", "0", "1=Client,2=House,7=Fund for EQM | offset: 144, len: 1"),
            ("char pad1[4]", None, "Padding"),
            ("uint64_t display_qty", "0", "Displayed quantity | offset: 136, len: 8"),
            ("char order_token[14]", "{}", "Replacement Order Token | offset: 9, len: 14"),
            ("char exchange_info[32]", "{}", "Account info for EQM | offset: 96, len: 32"),
            ("char pad2[10]", None, "Padding"),
            ("char client_account[16]", "{}", "Client/Account (Derivatives) | offset: 64, len: 16"),
            ("char customer_info[15]", "{}", "Pass-through | offset: 81, len: 15"),
            ("char pad3[33]", None, "Padding"),
            
        ],
    },
    {
        "name": "OUCHOrderCancelledMessage",
        "fields": [
            ("uint64_t timestamp", "0", "UNIX ns timestamp | offset: 1, len: 8"),
            ("uint64_t order_id", "0", "Canceled order ID | offset: 28, len: 8"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 23, len: 4"),
            ("char message_type", "'C'", "Order Canceled | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Order Token | offset: 9, len: 14"),
            ("char side", "'\\0'", "Side (B/S) | offset: 27, len: 1"),
            ("uint8_t reason", "0", "Cancel reason code | offset: 36, len: 1"),
            ("char pad[27]", None, "Padding"),
        ],
    },
    {
        "name": "OUCHOrderExecutedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "UNIX ns timestamp | offset: 1, len: 8"),
            ("uint64_t traded_quantity", "0", "Traded quantity | offset: 27, len: 8"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 23, len: 4"),
            ("int32_t trade_price", "0", "Trade price | offset: 35, len: 4"),
            ("char message_type", "'E'", "Order Executed | offset: 0, len: 1"),
            ("char order_token[14]", "{}", "Order Token | offset: 9, len: 14"),
            ("char match_id[12]", "{}", "Match ID | offset: 39, len: 12"),
            ("uint8_t client_category", "0", "Client category | offset: 51, len: 1"),
            ("char pad1[12]", None, "Padding"),
            ("char reserved[16]", "{}", "Reserved | offset: 52, len: 16"),
            ("char pad2[48]", None, "Padding"),
        ],
    },
]


def generate_struct(msg):
    struct_lines = [f"struct alignas(64) {msg['name']} {{"]
    
    max_field_len = 0
    for field, default, comment in msg["fields"]:
        if default is not None:
            length = len(field) + len(default) + 3
            if length > max_field_len:
                max_field_len = length
    comment_col = max_field_len + 8  
    
    for field_type, default, comment in msg["fields"]:
        if default is None:
            line = f"    {field_type};"
            spaces = comment_col - len(field_type)
        else:
            line = f"    {field_type} = {default};"
            spaces = comment_col - (len(field_type) + len(default) + 3)
        
        line += " " * spaces + f"// {comment}"
        struct_lines.append(line)
    struct_lines.append("};\n")
    return "\n".join(struct_lines)


def generate_file(messages):
    lines = [
        "// Generated automatically. DO NOT EDIT!",
        "// BIST OUCH Messages Struct Definitions",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace BIST {\n",
        "namespace IN {\n",
    ]

    for i, msg in enumerate(messages):
        lines.append(generate_struct(msg))
        if i == 3:
            lines.append("}\nnamespace OUT\n{\n")
           
    return "\n".join(lines)

def messagetype_index(msg):
    for i, field in enumerate(msg["fields"]):
       if field[0] == "char message_type":
        return i  

def generate_enum(messages):
     line = [f"\nenum class OUCHTypes : uint8_t {{ "]
     for i, msg in enumerate(messages):
         messagetype = messagetype_index(msg)
         default = msg["fields"][messagetype][1]  
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownOUCHtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func(messages):
    line = [f"\n\ninline constexpr OUCHTypes ouchMessageIndex(char type) {{\n"]
    line.append("   switch(type) {")
    for i, msg in enumerate(messages):
        messagetype = messagetype_index(msg)
        default = msg["fields"][messagetype][1]  
        line.append(f"      case {default}: return OUCHTypes::{default.strip("'")};")
    line.append(f"      default: return OUCHTypes::unknownOUCHtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 

file = generate_file(messages)
with open("../include/GeneratedOUCHMessages_BIST.h", "w") as f:
    f.write(file)
    f.write("\n}\n")
    f.write(generate_enum(messages))
    f.write(generate_func(messages))
    f.write("\n\n}\n")  




