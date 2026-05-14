messages = [
   {
        "name": "ITCHAddOrderMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Message time (nanoseconds)  offset: 5 | len: 6"),
            ("uint64_t order_ref", "0", "Order reference number (unique)  offset: 11 | len: 8"),
            ("uint32_t shares", "0", "Order quantity (shares)  offset: 20 | len: 4"),
            ("uint32_t price", "0", "Price (in 1/10000 USD, integer)  offset: 32 | len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID (Stock Locate)  offset: 1 | len: 2"),
            ("uint16_t tracking_number", "0", "Message tracking number  offset: 3 | len: 2"),
            ("char message_type", "'A'", "Message Type (A = Add Order)  offset: 0 | len: 1"),
            ("char side", "'\\0'", "Order side ('B' = Buy, 'S' = Sell)  offset: 19 | len: 1"),
            ("char stock[8]", "{}", "Stock symbol (8 characters, space-padded)  offset: 24 | len: 8"),
        ],
    },
    {
        "name": "ITCHAddOrderMPIDMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Message time (nanoseconds) | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Order reference number | offset: 11, len: 8"),
            ("uint32_t shares", "0", "Order quantity | offset: 20, len: 4"),         
            ("uint32_t price", "0", "Price | offset: 32, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'F'", "Message Type (F = Add Order with MPID) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Order side ('B'=Buy, 'S'=Sell) | offset: 19, len: 1"),
            ("char stock[8]", "{}", "Stock symbol | offset: 24, len: 8"),
            ("char mpid[4]", "{}", "Market Participant ID (MPID) | offset: 36, len: 4"),
        ],
    },
    {
        "name": "ITCHCancelMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Cancel time | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Cancelled order reference | offset: 11, len: 8"),
            ("uint32_t cancelled_shares", "0", "Cancelled quantity | offset: 19, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'X'", "Message Type (X = Order Cancel) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHExecutedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Execution time | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Executed order reference | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Match number | offset: 23, len: 8"),
            ("uint32_t executed_shares", "0", "Executed quantity | offset: 19, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'E'", "Message Type (E = Order Executed, no price) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHExecutedWithPriceMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Execution time | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Executed order reference | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Match number | offset: 23, len: 8"),
            ("uint32_t executed_shares", "0", "Executed quantity | offset: 19, len: 4"),
            ("uint32_t execution_price", "0", "Execution price | offset: 32, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'C'", "Message Type (C = Order Executed with price) | offset: 0, len: 1"),
            ("char printable", "'\\0'", "Price printable status (Y/N) | offset: 31, len: 1"),
        ],
    },
    {
        "name": "ITCHDeleteMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Delete time | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Deleted order reference | offset: 11, len: 8"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'D'", "Message Type (D = Order Deleted) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHReplaceMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Message time (nanoseconds)  offset: 5 | len: 6"),
            ("uint64_t order_ref", "0", "Order reference number (unique)  offset: 11 | len: 8"),
            ("uint64_t new_order_ref", "0", "New order reference number (unique)  offset: 19 | len: 8"),
            ("uint32_t shares", "0", "Order quantity (shares)  offset: 27 | len: 4"),
            ("uint32_t price", "0", "Price (in 1/10000 USD, integer)  offset: 31 | len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID (Stock Locate)  offset: 1 | len: 2"),
            ("uint16_t tracking_number", "0", "Message tracking number  offset: 3 | len: 2"),
            ("char message_type", "'U'", "Message Type (U = Order Replace)  offset: 0 | len: 1"),
        ],
    },                      
    {
        "name": "ITCHTradeMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Trade time | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Related order reference | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Match number | offset: 28, len: 8"),
            ("uint32_t shares", "0", "Trade quantity | offset: 20, len: 4"),
            ("uint32_t price", "0", "Trade price | offset: 24, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'P'", "Message Type (P = Trade) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Trade side ('B'/'S') | offset: 19, len: 1"),
            ("char cross_type", "'\\0'", "Cross type | offset: 36, len: 1"),
        ],
    },
    {
        "name": "ITCHSystemEventMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Event time | offset: 5, len: 6"),
            ("uint16_t stock_locate", "0", "Stock ID (always 0) | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'S'", "Message Type (S = System Event) | offset: 0, len: 1"),
            ("char event_code", "'\\0'", "Event code | offset: 11, len: 1"),
        ],
    },
    {
        "name": "ITCHStockDirectoryMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Event time | offset: 5, len: 6"),
            ("uint32_t round_lot_size", "0", "Round lot size | offset: 21, len: 4"),
            ("uint32_t etp_leverage_factor", "0", "ETP leverage | offset: 34, len: 4"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'R'", "Message Type (R = Stock Directory) | offset: 0, len: 1"),
            ("char stock[8]", "{}", "Stock symbol | offset: 11, len: 8"),      
            ("char market_category", "'\\0'", "Market category | offset: 19, len: 1"),
            ("char financial_status_indicator", "'\\0'", "Financial status code | offset: 20, len: 1"),
            ("char round_lots_only", "'\\0'", "Round lots only | offset: 25, len: 1"),
            ("char issue_classification", "'\\0'", "Issue classification | offset: 26, len: 1"),
            ("char issue_sub_type[2]", "{}", "Issue sub-type | offset: 27, len: 2"),
            ("char authenticity", "'\\0'", "Authenticity code | offset: 29, len: 1"),
            ("char short_sale_threshold_indicator", "'\\0'", "Short sale threshold | offset: 30, len: 1"),
            ("char ipo_flag", "'\\0'", "IPO flag | offset: 31, len: 1"),
            ("char luld_reference_price_tier", "'\\0'", "LULD price tier | offset: 32, len: 1"),
            ("char etp_flag", "'\\0'", "ETP flag | offset: 33, len: 1"),
            ("char inverse_indicator", "'\\0'", "Inverse indicator | offset: 38, len: 1"),

        ],
    },
    {
        "name": "ITCHTradingStateMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Event time | offset: 5, len: 6"),
            ("uint16_t stock_locate", "0", "Stock ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'H'", "Message Type (H = Trading State) | offset: 0, len: 1"),
            ("char stock[8]", "{}", "Stock symbol | offset: 11, len: 8"),
            ("char trading_state", "'T'", "T=Trading, H=Halted, P=Paused, Q=Quotation | offset: 19, len: 1"),
            ("char reserved", "'\\0'", "Reserved | offset: 20, len: 1"),
            ("char reason[4]", "{}", "Reason | offset: 21, len: 4"),
        ],
    },
]

def generate_struct(msg):
    struct_lines = [f"struct {msg['name']} {{"]
    
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
        "// NASDAQ ITCH Messages Struct Definitions",
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <cstddef>",
        "#include <array>",
        "",
        "namespace NASDAQ {\n",
    ]
    for msg in messages:
        lines.append(generate_struct(msg))
    return "\n".join(lines)

def messagetype_index(msg):
    for i, field in enumerate(msg["fields"]):
       if field[0] == "char message_type":
        return i  

def generate_enum(messages):
     line = [f"\nenum class ITCHTypes : size_t {{ "]
     for i, msg in enumerate(messages):
         messagetype = messagetype_index(msg)
         default = msg["fields"][messagetype][1]  
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownITCHtype = 99")
     line.append(f"}};")
     return" ".join(line)

def msg_size(msg):
    size = 0
    for field in msg["fields"]:
        
        if "t64" in field[0]:
            size += 8
        elif "t32" in field[0]:
            size += 4
        elif "t16" in field[0]:
            size += 2
        elif '[' in field[0]:
            result = 0
            idx = field[0].find('[')
            idx += 1
            while field[0][idx] != ']':
                result = (result * 10) + (int)(field[0][idx])
                idx += 1
           
            size += result
        else:
            size += 1
                    
    return size - 2
    
def generate_arr(messages):
     all_msg_count = len(messages)
     line = [f"\n\ninline std::array<size_t, {all_msg_count}> ITCHSizeOfs = {{ "]
     for msg in messages:
         msg_bytes = msg_size(msg)
         line.append(f"{msg_bytes},")
     line.append(f"}};")
     return" ".join(line)

def generate_func(messages):
    line = [f"\n\ninline constexpr size_t itchMessageIndex(char type) {{\n"]
    line.append("   switch(type) {")
    for i, msg in enumerate(messages):
        messagetype = messagetype_index(msg)
        default = msg["fields"][messagetype][1]  
        line.append(f"      case {default}: return static_cast<size_t>(ITCHTypes::{default.strip("'")});")
    line.append(f"      default: return static_cast<size_t>(ITCHTypes::unknownITCHtype);")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 

file = generate_file(messages)
with open("../../generated/GeneratedITCHMessages_NASDAQ.h", "w") as f:
    f.write(file)
    f.write(generate_enum(messages))
    f.write(generate_arr(messages))
    f.write(generate_func(messages))
    f.write("\n\n}\n")  
   



