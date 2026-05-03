messages = [
    
    # ==================== INBOUND MESSAGES ====================
    
    {
        "name": "OUCHEnterOrderMessage",  
        "fields": [
            ("uint64_t price", "0", "Price with 4 implied decimals | offset: 18, len: 8"),
            ("uint32_t user_ref_num", "0", "Day-unique, strictly increasing ID | offset: 1, len: 4"),
            ("uint32_t quantity", "0", "Total shares (must be >0 and <1,000,000) | offset: 6, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 45, len: 2"),
            ("char message_type", "'O'", "Enter Order Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "B=Buy, S=Sell, T=SellShort, E=SellShortExempt | offset: 5, len: 1"),
            ("char symbol[8]", "{}", "Stock symbol | offset: 10, len: 8"),
            ("char time_in_force", "'0'", "0=Day, 3=IOC, 5=GTX, 6=GTT, E=AfterHours | offset: 26, len: 1"),
            ("char display", "'Y'", "Y=Visible, N=Hidden, A=Attributable | offset: 27, len: 1"),
            ("char capacity", "'A'", "A=Agency, P=Principal, R=Riskless, O=Other | offset: 28, len: 1"),
            ("char intermarket_sweep_eligibility", "'N'", "Y=Eligible, N=NotEligible | offset: 29, len: 1"),
            ("char cross_type", "'N'", "N=Continuous, O=Opening, C=Closing, H=Halt/IPO, S=Supplemental, R=Retail, E=ExtendedLife, A=AfterHoursClose | offset: 30, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier | offset: 31, len: 14"),
            ("//char optional_appendage[0]", "{}", "Optional fields (Firm, MinQty, CustomerType, MaxFloor, PriceType, PegOffset, etc.) | offset: 47, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHReplaceOrderMessage",  
        "fields": [
            ("uint64_t price", "0", "Replacement price with 4 implied decimals | offset: 13, len: 8"),
            ("uint32_t orig_user_ref_num", "0", "Original order UserRefNum | offset: 1, len: 4"),
            ("uint32_t user_ref_num", "0", "New replacement UserRefNum (must be unique) | offset: 5, len: 4"),
            ("uint32_t quantity", "0", "Total shares liable (inclusive of previous executions) | offset: 9, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 38, len: 2"),
            ("char message_type", "'u'", "Replace Order Message | offset: 0, len: 1"),
            ("char time_in_force", "'0'", "0=Day, 3=IOC, 5=GTX, 6=GTT | offset: 21, len: 1"),
            ("char display", "'Y'", "Y=Visible, N=Hidden, A=Attributable | offset: 22, len: 1"),
            ("char intermarket_sweep_eligibility", "'N'", "Y=Eligible, N=NotEligible | offset: 23, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier for replacement | offset: 24, len: 14"),
            ("//char optional_appendage[0]", "{}", "Optional fields (MinQty, MaxFloor, PriceType, PostOnly, ExpireTime, TradeNow, HandleInst, etc.) | offset: 40, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHCancelOrderMessage",  
        "fields": [
            ("uint32_t user_ref_num", "0", "Order UserRefNum to cancel | offset: 1, len: 4"),
            ("uint32_t quantity", "0", "New intended order size (0=cancel all remaining) | offset: 5, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 9, len: 2"),          
            ("char message_type", "'X'", "Cancel Order Message | offset: 0, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 11, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHModifyOrderMessage", 
        "fields": [
            ("uint32_t user_ref_num", "0", "Order UserRefNum to modify | offset: 1, len: 4"),
            ("uint32_t quantity", "0", "New intended order size | offset: 6, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 10, len: 2"),
            ("char message_type", "'m'", "Modify Order Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "New side: B=Buy, S=Sell, T=SellShort, E=SellShortExempt (only S<->E, S<->T, E<->T allowed) | offset: 5, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx, SharesLocated, LocateBroker) | offset: 12, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHAccountQueryRequest",  
        "fields": [
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 1, len: 2"),
            ("char message_type", "'q'", "Account Query Request | offset: 0, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 3, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    
    # ==================== OUTBOUND MESSAGES ====================
    
    {
        "name": "OUCHSystemEventMessage", 
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("char message_type", "'S'", "System Event Message | offset: 0, len: 1"),
            ("char event_code", "'\\0'", "S=StartOfDay, E=EndOfDay | offset: 9, len: 1"),
        ],
    },
    {
        "name": "OUCHOrderAcceptedMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint64_t price", "0", "Accepted price (may differ from entered) | offset: 26, len: 8"),
            ("uint64_t order_reference_number", "0", "Day-unique order reference assigned by NASDAQ | offset: 36, len: 8"),
            ("uint32_t user_ref_num", "0", "Echo of UserRefNum | offset: 9, len: 4"),
            ("uint32_t quantity", "0", "Total shares accepted | offset: 14, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 62, len: 2"),
            ("char message_type", "'A'", "Order Accepted Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "B/S/T/E | offset: 13, len: 1"),
            ("char symbol[8]", "{}", "Stock symbol | offset: 18, len: 8"),
            ("char time_in_force", "'0'", "Accepted TIF | offset: 34, len: 1"),
            ("char display", "'\\0'", "Y=Visible, N=Hidden, A=Attributable, Z=Conformant | offset: 35, len: 1"),
            ("char capacity", "'\\0'", "A/P/R/O | offset: 44, len: 1"),
            ("char intermarket_sweep_eligibility", "'\\0'", "Y/N | offset: 45, len: 1"),
            ("char cross_type", "'\\0'", "N/O/C/H/S/R/E/A | offset: 46, len: 1"),
            ("char order_state", "'\\0'", "L=OrderLive, D=OrderDead | offset: 47, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier | offset: 48, len: 14"),
            ("//char optional_appendage[0]", "{}", "Optional fields (Firm, MinQty, CustomerType, MaxFloor, PriceType, etc.) | offset: 64, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHOrderReplacedMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint64_t price", "0", "Accepted replacement price | offset: 30, len: 8"),
            ("uint64_t order_reference_number", "0", "Day-unique order reference | offset: 40, len: 8"),
            ("uint32_t orig_user_ref_num", "0", "Original UserRefNum | offset: 9, len: 4"),
            ("uint32_t user_ref_num", "0", "Replacement UserRefNum | offset: 13, len: 4"),
            ("uint32_t quantity", "0", "Total shares outstanding | offset: 18, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 66, len: 2"),
            ("char message_type", "'U'", "Order Replaced Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "B/S/T/E | offset: 17, len: 1"),
            ("char symbol[8]", "{}", "Stock symbol | offset: 22, len: 8"),
            ("char time_in_force", "'0'", "Accepted TIF | offset: 38, len: 1"),
            ("char order_state", "'\\0'", "L=OrderLive, D=OrderDead | offset: 51, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier | offset: 52, len: 14"),
            ("char display", "'\\0'", "Y=Visible, N=Hidden, A=Attributable, Z=Conformant | offset: 39, len: 1"),
            ("char capacity", "'\\0'", "A/P/R/O | offset: 48, len: 1"),
            ("char intermarket_sweep_eligibility", "'\\0'", "Y/N | offset: 49, len: 1"),
            ("char cross_type", "'\\0'", "N/O/C/H/S/R/E/A | offset: 50, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (Firm, MinQty, MaxFloor, PriceType, etc.) | offset: 68, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHOrderCancelledMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t user_ref_num", "0", "Order UserRefNum | offset: 9, len: 4"),
            ("uint32_t quantity", "0", "Incremental shares canceled | offset: 13, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 18, len: 2"),
            ("char message_type", "'C'", "Order Canceled Message | offset: 0, len: 1"),
            ("char reason", "'\\0'", "Cancel reason (see Appendix B) | offset: 17, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 20, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHOrderExecutedMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint64_t price", "0", "Execution price | offset: 17, len: 8"),
            ("uint64_t match_number", "0", "Exchange-assigned match ID | offset: 26, len: 8"),
            ("uint32_t user_ref_num", "0", "Order UserRefNum | offset: 9, len: 4"),
            ("uint32_t quantity", "0", "Incremental shares executed | offset: 13, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 34, len: 2"),
            ("char message_type", "'E'", "Order Executed Message | offset: 0, len: 1"),
            ("char liquidity_flag", "'\\0'", "Liquidity flag (see Appendix D) | offset: 25, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 36, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHOrderRejectedMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t user_ref_num", "0", "Rejected UserRefNum (cannot be reused) | offset: 9, len: 4"),
            ("uint16_t reason", "0", "Reject reason code (see Appendix C) | offset: 13, len: 2"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 29, len: 2"),
            ("char message_type", "'J'", "Order Rejected Message | offset: 0, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier | offset: 15, len: 14"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 31, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHBrokenTradeMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint64_t match_number", "0", "Match number from original execution | offset: 13, len: 8"),
            ("uint32_t user_ref_num", "0", "Order UserRefNum | offset: 9, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 36, len: 2"),
            ("char message_type", "'B'", "Broken Trade Message | offset: 0, len: 1"),
            ("char reason", "'\\0'", "E=Erroneous, C=Consent, S=Supervisory, X=External | offset: 21, len: 1"),
            ("char cl_ord_id[14]", "{}", "Customer order identifier | offset: 22, len: 14"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 38, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHOrderModifiedMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t user_ref_num", "0", "Modified order UserRefNum | offset: 9, len: 4"),
            ("uint32_t quantity", "0", "Total shares outstanding | offset: 14, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 18, len: 2"),
            ("char message_type", "'M'", "Order Modified Message | offset: 0, len: 1"),
            ("char side", "'\\0'", "New side (B/S/T/E) | offset: 13, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx, SharesLocated, LocateBroker) | offset: 20, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHCancelPendingMessage",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t user_ref_num", "0", "Order UserRefNum | offset: 9, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 13, len: 2"),
            ("char message_type", "'P'", "Cancel Pending Message | offset: 0, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHCancelRejectMessage", 
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t user_ref_num", "0", "Order UserRefNum | offset: 9, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 13, len: 2"),
            ("char message_type", "'I'", "Cancel Reject Message | offset: 0, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
        ],
    },
    {
        "name": "OUCHAccountQueryResponse",  
        "fields": [
            ("uint64_t timestamp", "0", "Nanoseconds since midnight | offset: 1, len: 8"),
            ("uint32_t next_user_ref_num", "0", "Next available UserRefNum | offset: 9, len: 4"),
            ("uint16_t appendage_length", "0", "Length of optional appendage | offset: 13, len: 2"),
            ("char message_type", "'Q'", "Account Query Response | offset: 0, len: 1"),
            ("//char optional_appendage[0]", "{}", "Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)"),
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
        "// NASDAQ OUCH Messages Struct Definitions",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace NASDAQ {\n",
        "/* namespace IN {\n",
    ]

    for i, msg in enumerate(messages):
        lines.append(generate_struct(msg))
        if i == 4:
            lines.append("} */ \nnamespace OUT\n{\n")
           
    return "\n".join(lines)

def messagetype_index(msg):
    for i, field in enumerate(msg["fields"]):
       if field[0] == "char message_type":
        return i  

def generate_enum(messages):
     line = [f"\nenum class OUCHTypes : uint8_t {{ "]
     for i, msg in enumerate(messages[5:]):
         messagetype = messagetype_index(msg)
         default = msg["fields"][messagetype][1]  
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownOUCHtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func_ouctypes(messages):
    line = [f"\n\ninline constexpr OUCHTypes ouchMessageIndex(char type) {{\n"]
    line.append("   switch(type) {")
    for i, msg in enumerate(messages[5:]):
        messagetype = messagetype_index(msg)
        default = msg["fields"][messagetype][1]  
        line.append(f"      case {default}: return OUCHTypes::{default.strip("'")};")
    line.append(f"      default: return OUCHTypes::unknownOUCHtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 


file = generate_file(messages)
with open("../../generated/GeneratedOUCHMessages_NASDAQ.h", "w") as f:
    f.write(file)
    f.write("\n}\n")
    f.write(generate_enum(messages))
    f.write(generate_func_ouctypes(messages))
    f.write("\n\n}\n")  


# CancelReasons = {
#      'D': "This order cannot be executed because of a regulatory restriction (e.g.: trade through restrictions).",
#      'E': "Closed. Any DAY order that was received after the closing cross is complete in a given symbol will receive this cancel reason",
#      'F': "Post Only Cancel. This Post Only order was cancelled because it would have been price slid for NMS.",
#      'G': "Post Only Cancel. This Post Only order was cancelled because it would have been price slid due to a contra side displayed order on the book",
#      'H': "Halted. The on-open order was canceled because the symbol remained halted after the opening cross completed.",
#      'I': "Immediate or Cancel Order.",
#      'K': "This order cannot be executed because of Market Collars",
#      'Q': "Self Match Prevention. The order was cancelled because it would have executed with an existing order entered by the same MPID.",
#      'S': "Supervisory. The order was manually canceled or reduced by an NASDAQ supervisory terminal.",
#      'T': "Timeout. The Time In Force for this order has expired",
#      'U': "User requested cancel. Sent in response to a Cancel Request Message.",
#      'X': "Open Protection. Orders that are cancelled as a result of the Opening Price Protection Threshold.",
#      'Z': "System cancel. This order was cancelled by the system.",
#      'e': "Company Direct Listing Capital Raise order exceeds allowable shares offered",
# }

# RejectReasons = {
#     0x0001: "Quote Unavailable",
#     0x0002: "Destination Closed",
#     0x0003: "Invalid Display",
#     0x0004: "Invalid Max Floor",
#     0x0005: "Invalid Peg Type",
#     0x0006: "Fat Finger",
#     0x0007: "Halted",
#     0x0008: "ISO Not Allowed",
#     0x0009: "Invalid Side",
#     0x000A: "Processing Error",
#     0x000B: "Cancel Pending",
#     0x000C: "Firm Not Authorized",
#     0x000D: "Invalid Min Quantity",
#     0x000E: "No Closing Reference Price",
#     0x000F: "Other",
#     0x0010: "Cancel Not Allowed",
#     0x0011: "Pegging Not Allowed",
#     0x0012: "Crossed Market",
#     0x0013: "Invalid Quantity",
#     0x0014: "Invalid Cross Order",
#     0x0015: "Replace Not Allowed",
#     0x0016: "Routing Not Allowed",
#     0x0017: "Invalid Symbol",
#     0x0018: "Test",
#     0x0019: "Late LOC Too Aggressive",
#     0x001A: "Retail Not Allowed",
#     0x001B: "Invalid Midpoint Post Only Price",
#     0x001C: "Invalid Destination",
#     0x001D: "Invalid Price",
#     0x001E: "Shares Exceed Threshold",
#     0x001F: "Exceeds Maximum Allowed Notional Value",
#     0x0020: "Risk: Aggregate Exposure Exceeded",
#     0x0021: "Risk: Market Impact",
#     0x0022: "Risk: Restricted Stock",
#     0x0023: "Risk: Short Sell Restricted",
#     0x0024: "Risk: ISO Not Allowed",
#     0x0025: "Risk: Exceeds ADV Limit",
#     0x0026: "Risk: Fat Finger",
#     0x0027: "Risk: Locate Required",
#     0x0028: "Risk: Symbol Message Rate Restriction",
#     0x0029: "Risk: Port Message Rate Restriction",
#     0x002A: "Risk: Duplicate Message Rate Restriction",
#     0x002B: "Risk: Short Sell Not Allowed",
#     0x002C: "Risk: Market Order Not Allowed",
#     0x002D: "Risk: Pre-Market Not Allowed",
#     0x002E: "Risk: Post-Market Not Allowed",
#     0x002F: "Risk: Short Sell Exempt Not Allowed",
#     0x0030: "Risk: Single Order Notional Exceeded",
#     0x0031: "Risk: Max Quantity Exceeded",
#     0x0032: "Reg SHO State Not Available",
#     0x0033: "Risk: IPO Market Buy Not Allowed",
# }

# def generate_func_c_reason(CancelReasons):
#     line = [f"\n\ninline constexpr std::string_view to_string (char cancel_reason) {{\n"]
#     line.append("   switch(cancel_reason) {")
#     for sym, rea in CancelReasons.items():
#         line.append(f"      case '{sym}': return \"{rea}\";")
#     line.append(f"      default: return \"\"; ")
#     line.append("   }")
#     line.append("}")
#     return "\n".join(line)

# def generate_func_r_reason(RejectReasons):
#     line = [f"\n\ninline constexpr std::string_view to_string (uint16_t reject_reason) {{\n"]
#     line.append("   switch(reject_reason) {")
#     for val, rea in RejectReasons.items():
#         line.append(f"      case {val}: return \"{rea}\";")
#     line.append(f"      default: return \"\"; ")
#     line.append("   }")
#     line.append("}")
#     return "\n".join(line)  