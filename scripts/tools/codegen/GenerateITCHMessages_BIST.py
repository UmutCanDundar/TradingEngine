messages = [
    {
        "name": "ITCHSecondsMessage",
        "fields": [
            ("uint32_t second", "0", "Unix time (seconds) | offset: 1, len: 4"),
            ("char message_type", "'T'", "Message Type (T = Seconds) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHOrderBookDirectoryMessage",
        "fields": [
            ("uint64_t nominal_value", "0", "Nominal value | offset: 105, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 5, len: 4"),
            ("uint32_t odd_lot_size", "0", "Odd lot size | offset: 93, len: 4"),
            ("uint32_t round_lot_size", "0", "Round lot size | offset: 97, len: 4"),
            ("uint32_t block_lot_size", "0", "Block lot size | offset: 101, len: 4"),
            ("uint32_t underlying_order_book_id", "0", "Underlying order book ID | offset: 114, len: 4"),
            ("int32_t strike_price", "0", "Strike price (derivatives) | offset: 118, len: 4"),
            ("uint32_t expiration_date", "0", "Expiration date YYYYMMDD | offset: 122, len: 4"),
            ("uint16_t decimals_in_price", "0", "Number of decimal places in price | offset: 89, len: 2"),
            ("uint16_t decimals_in_nominal", "0", "Number of decimal places in nominal value | offset: 91, len: 2"),
            ("uint16_t decimals_in_strike_price", "0", "Number of decimal places in strike price | offset: 126, len: 2"),
            ("uint8_t financial_product", "0", "Financial product (1=Option, 5=Cash, etc.) | offset: 85, len: 1"),
            ("uint8_t number_of_legs", "0", "Number of legs (for combination) | offset: 113, len: 1"),
            ("uint8_t put_or_call", "0", "Put/Call (1=Call, 2=Put) | offset: 128, len: 1"),
            ("uint8_t ranking_type", "0", "Ranking type (1=Price Time) | offset: 129, len: 1"),
            ("char message_type", "'R'", "Message Type (R = Order Book Directory) | offset: 0, len: 1"),
            ("char isin[12]", "{}", "ISIN code | offset: 73, len: 12"),
            ("char symbol[32]", "{}", "Security short name | offset: 9, len: 32"),
            ("char long_name[32]", "{}", "Long name | offset: 41, len: 32"),
            ("char trading_currency[3]", "{}", "Trading currency | offset: 86, len: 3"),           
        ],
    },
    {
        "name": "ITCHCombinationOrderBookLegMessage",
        "fields": [
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t combination_order_book_id", "0", "Combination order book ID | offset: 5, len: 4"),
            ("uint32_t leg_order_book_id", "0", "Leg order book ID | offset: 9, len: 4"),
            ("uint32_t leg_ratio", "0", "Leg ratio | offset: 14, len: 4"),
            ("char message_type", "'M'", "Message Type (M = Combination Leg) | offset: 0, len: 1"),
            ("char leg_side", "'\\0'", "Leg side (B=As Defined, C=Opposite) | offset: 13, len: 1"),
        ],
    },
    {
        "name": "ITCHTickSizeTableEntryMessage",
        "fields": [
            ("uint64_t tick_size", "0", "Tick size (8 bytes!) | offset: 9, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 5, len: 4"),
            ("int32_t price_from", "0", "Price from | offset: 17, len: 4"),
            ("int32_t price_to", "0", "Price to (0=infinity) | offset: 21, len: 4"),
            ("char message_type", "'L'", "Message Type (L = Tick Size) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHShortSellStatusMessage",
        "fields": [
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID (Equity only) | offset: 5, len: 4"),
            ("uint8_t short_sale_restriction", "0", "Short sell status (0=No restriction, 1=Not allowed, 2=Uptick rule) | offset: 9, len: 1"),
            ("char message_type", "'V'", "Message Type (V = Short Sell Status) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHSystemEventMessage",
        "fields": [
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("char message_type", "'S'", "Message Type (S = System Event) | offset: 0, len: 1"),
            ("char event_code", "'\\0'", "Event code (O=Start, C=End) | offset: 5, len: 1"),
        ],
    },
   
    {
        "name": "ITCHOrderBookStateMessage",
        "fields": [
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 5, len: 4"),
            ("char message_type", "'O'", "Message Type (O = Order Book State) | offset: 0, len: 1"),
            ("char state_name[20]", "{}", "State name | offset: 9, len: 20"),
        ],
    },
    {
        "name": "ITCHAddOrderMessage",
        "fields": [
            ("uint64_t order_id", "0", "Order ID (not unique! together with order_book_id + side) | offset: 5, len: 8"),
            ("uint64_t quantity", "0", "Visible quantity | offset: 22, len: 8"),
            ("uint64_t ranking_time", "0", "Ranking timestamp - nanosecond | offset: 37, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 13, len: 4"),
            ("uint32_t ranking_seq_number", "0", "Ranking sequence number | offset: 18, len: 4"),
            ("int32_t price", "0", "Display price (SIGNED!) | offset: 30, len: 4"),
            ("uint16_t order_attributes", "0", "Order attributes (0=N/A, 8192=Bait) | offset: 34, len: 2"),
            ("uint8_t lot_type", "0", "Lot type (2=Round Lot) | offset: 36, len: 1"),
            ("char message_type", "'A'", "Message Type (A = Add Order) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 17, len: 1"),
        ],
    },
    {
        "name": "ITCHOrderExecutedMessage",
        "fields": [
            ("uint64_t order_id", "0", "Order ID | offset: 5, len: 8"),
            ("uint64_t executed_quantity", "0", "Executed quantity | offset: 18, len: 8"),
            ("uint64_t match_id", "0", "Match ID | offset: 26, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 13, len: 4"),
            ("uint32_t combo_group_id", "0", "Combo group ID | offset: 34, len: 4"),
            ("char message_type", "'E'", "Message Type (E = Order Executed) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 17, len: 1"),
            ("char reserved1[7]", "{}", "Reserved | offset: 38, len: 7"),
            ("char reserved2[7]", "{}", "Reserved | offset: 45, len: 7"),
        ],
    },
    {
        "name": "ITCHOrderExecutedWithPriceMessage",
        "fields": [
            ("uint64_t order_id", "0", "Order ID | offset: 5, len: 8"),
            ("uint64_t executed_quantity", "0", "Executed quantity | offset: 18, len: 8"),
            ("uint64_t match_id", "0", "Match ID | offset: 26, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 13, len: 4"),
            ("uint32_t combo_group_id", "0", "Combo group ID | offset: 34, len: 4"),
            ("int32_t trade_price", "0", "Trade price (SIGNED!) | offset: 52, len: 4"),
            ("char message_type", "'C'", "Message Type (C = Order Executed with Price) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 17, len: 1"),
            ("char reserved1[7]", "{}", "Reserved | offset: 38, len: 7"),
            ("char reserved2[7]", "{}", "Reserved | offset: 45, len: 7"),
            ("char occurred_at_cross", "'\\0'", "Occurred at cross (Y/N) | offset: 56, len: 1"),
            ("char printable", "'\\0'", "Printable (Y/N) | offset: 57, len: 1"),
        ],
    },
    {
        "name": "ITCHOrderDeleteMessage",
        "fields": [
            ("uint64_t order_id", "0", "Order ID | offset: 5, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 13, len: 4"),
            ("char message_type", "'D'", "Message Type (D = Order Delete) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B=Buy, S=Sell) | offset: 17, len: 1"),
        ],
    },
     {
        "name": "ITCHOrderBookFlushMessage",
        "fields": [
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 5, len: 4"),
            ("char message_type", "'Y'", "Message Type (Y = Order Book Flush) | offset: 0, len: 1"),
        ],
    },
    {
        "name": "ITCHTradeMessage",
        "fields": [
            ("uint64_t match_id", "0", "Match ID | offset: 5, len: 8"),
            ("uint64_t quantity", "0", "Quantity | offset: 18, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t combo_group_id", "0", "Combo group ID | offset: 13, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 26, len: 4"),
            ("int32_t trade_price", "0", "Trade price | offset: 30, len: 4"),
            ("char message_type", "'P'", "Message Type (P = Trade) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Side (B/S/Space) | offset: 17, len: 1"),
            ("char reserved1[7]", "{}", "Reserved | offset: 34, len: 7"),
            ("char reserved2[7]", "{}", "Reserved | offset: 41, len: 7"),
            ("char printable", "'\\0'", "Printable (Y/N) | offset: 48, len: 1"),
            ("char occurred_at_cross", "'\\0'", "Occurred at cross (Y/N) | offset: 49, len: 1"),
            ("uint8_t order_id", "0", "dummy: enables requires{msg->order_id} branch in OrderManager, never read"),
        ],
    },
    {
        "name": "ITCHEquilibriumPriceUpdateMessage",
        "fields": [
            ("uint64_t available_bid_quantity", "0", "Bid quantity at equilibrium | offset: 9, len: 8"),
            ("uint64_t available_ask_quantity", "0", "Ask quantity at equilibrium | offset: 17, len: 8"),
            ("uint64_t best_bid_quantity", "0", "Best bid quantity | offset: 37, len: 8"),
            ("uint64_t best_ask_quantity", "0", "Best ask quantity | offset: 45, len: 8"),
            ("uint32_t timestamp_ns", "0", "Nanoseconds part | offset: 1, len: 4"),
            ("uint32_t order_book_id", "0", "Order book ID | offset: 5, len: 4"),
            ("int32_t equilibrium_price", "0", "Equilibrium price | offset: 25, len: 4"),
            ("int32_t best_bid_price", "0", "Best bid price | offset: 29, len: 4"),
            ("int32_t best_ask_price", "0", "Best ask price | offset: 33, len: 4"),
            ("char message_type", "'Z'", "Message Type (Z = Equilibrium Price) | offset: 0, len: 1"),
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
        "// BIST ITCH Messages Struct Definitions",
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <array>",
        "#include <cstddef>",
        "",
        "namespace BIST {\n",
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

def msg_size(msg, msg_idx):
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

    if (msg_idx == 12):
        size -= 1

    return size
       
def generate_arr(messages):
     all_msg_count = len(messages)
     line = [f"\n\ninline std::array<size_t, {all_msg_count}> ITCHSizeOfs = {{ "]
     for i, msg in enumerate(messages):
         msg_bytes = msg_size(msg, i)
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
with open("../../generated/GeneratedITCHMessages_BIST.h", "w") as f:
    f.write(file)
    f.write(generate_enum(messages))
    f.write(generate_arr(messages))
    f.write(generate_func(messages))
    f.write("\n\n}\n")  




