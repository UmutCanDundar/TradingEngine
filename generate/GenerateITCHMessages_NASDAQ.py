messages = [
   {
        "name": "ITCHAddOrderMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Mesaj zamanı (nanosecond)  offset: 5 | len: 6"),
            ("uint64_t order_ref", "0", "Emir referans numarası (benzersiz)  offset: 11 | len: 8"),
            ("uint32_t shares", "0", "Emir miktarı (adet)  offset: 20 | len: 4"),
            ("uint32_t price", "0", "Fiyat (1/10000 dolar cinsinden, integer)  offset: 32 | len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID (Stock Locate)  offset: 1 | len: 2"),
            ("uint16_t tracking_number", "0", "Mesaj izleme numarası (Tracking Number)  offset: 3 | len: 2"),
            ("char message_type", "'A'", "Mesaj Türü (A = Yeni Emir Ekle)  offset: 0 | len: 1"),
            ("char side", "'\\0'", "Emir yönü ('B' = Alış, 'S' = Satış)  offset: 19 | len: 1"),
            ("char stock[8]", "{}", "Hisse sembolü (8 karakter, boşlukla doldurulmuş)  offset: 24 | len: 8"),
            ("char pad[26]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHAddOrderMPIDMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Mesaj zamanı (nanosecond) | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Emir referans numarası | offset: 11, len: 8"),
            ("uint32_t shares", "0", "Emir miktarı | offset: 20, len: 4"),         
            ("uint32_t price", "0", "Fiyat | offset: 32, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'F'", "Mesaj Türü (F = MPID ile Yeni Emir Ekle) | offset: 0, len: 1"),
            ("char side", "'\\0'", "Emir yönü ('B'=Alış, 'S'=Satış) | offset: 19, len: 1"),
            ("char stock[8]", "{}", "Hisse sembolü | offset: 24, len: 8"),
            ("char mpid[4]", "{}", "Market Katılımcı ID'si (MPID) | offset: 36, len: 4"),
            ("char pad[22]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHCancelMessage",
        "fields": [
            ("uint64_t timestamp", "0", "İptal zamanı | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "İptal edilen emir referansı | offset: 11, len: 8"),
            ("uint32_t cancelled_shares", "0", "İptal edilen miktar | offset: 19, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'X'", "Mesaj Türü (X = Emir İptal) | offset: 0, len: 1"),
            ("char pad[39]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHExecutedMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Gerçekleşme zamanı | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Gerçekleşen emir referansı | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Eşleşme numarası | offset: 23, len: 8"),
            ("uint32_t executed_shares", "0", "Gerçekleşen miktar | offset: 19, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'E'", "Mesaj Türü (E = Emir Gerçekleşti, fiyat yok) | offset: 0, len: 1"),
            ("char pad[31]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHExecutedWithPriceMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Gerçekleşme zamanı | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Gerçekleşen emir referansı | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Eşleşme numarası | offset: 23, len: 8"),
            ("uint32_t executed_shares", "0", "Gerçekleşen miktar | offset: 19, len: 4"),
            ("uint32_t execution_price", "0", "Gerçekleşme fiyatı | offset: 32, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'C'", "Mesaj Türü (C = Emir Gerçekleşti, fiyat var) | offset: 0, len: 1"),
            ("char printable", "'\\0'", "Fiyatın baskı durumu (Y/N) | offset: 31, len: 1"),
            ("char pad[26]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHDeleteMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Silinme zamanı | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "Silinen emir referansı | offset: 11, len: 8"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'D'", "Mesaj Türü (D = Emir Silindi) | offset: 0, len: 1"),
            ("char pad[43]", None, "Doldurma (Padding)"), 
        ],
    },
    {
        "name": "ITCHReplaceMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Mesaj zamanı (nanosecond)  offset: 5 | len: 6"),
            ("uint64_t order_ref", "0", "Emir referans numarası (benzersiz)  offset: 11 | len: 8"),
            ("uint64_t new_order_ref", "0", "Emir referans numarası (benzersiz)  offset: 19 | len: 8"),
            ("uint32_t shares", "0", "Emir miktarı (adet)  offset: 27 | len: 4"),
            ("uint32_t price", "0", "Fiyat (1/10000 dolar cinsinden, integer)  offset: 31 | len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID (Stock Locate)  offset: 1 | len: 2"),
            ("uint16_t tracking_number", "0", "Mesaj izleme numarası (Tracking Number)  offset: 3 | len: 2"),
            ("char message_type", "'U'", "Mesaj Türü (A = Yeni Emir Ekle)  offset: 0 | len: 1"),
            ("char pad[27]", None, "Doldurma (Padding)"),
        ],
    },                      
    {
        "name": "ITCHTradeMessage",
        "fields": [
            ("uint64_t timestamp", "0", "İşlem zamanı | offset: 5, len: 6"),
            ("uint64_t order_ref", "0", "İlgili emir referansı | offset: 11, len: 8"),
            ("uint64_t match_number", "0", "Eşleşme numarası | offset: 28, len: 8"),
            ("uint32_t shares", "0", "İşlem miktarı | offset: 20, len: 4"),
            ("uint32_t price", "0", "İşlem fiyatı | offset: 24, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'P'", "Mesaj Türü (P = İşlem) | offset: 0, len: 1"),
            ("char side", "'\\0'", "İşlem yönü ('B'/'S') | offset: 19, len: 1"),
            ("char cross_type", "'\\0'", "Cross türü | offset: 36, len: 1"),
            ("char pad[25]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHSystemEventMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Olay zamanı | offset: 5, len: 6"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID (her zaman 0) | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'S'", "Mesaj Türü (S = Sistem Olayı) | offset: 0, len: 1"),
            ("char event_code", "'\\0'", "Olay kodu | offset: 11, len: 1"),
            ("char pad[50]", None, "Doldurma (Padding)"),
        ],
    },
    {
        "name": "ITCHStockDirectoryMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Olay zamanı | offset: 5, len: 6"),
            ("uint32_t round_lot_size", "0", "Round lot size | offset: 21, len: 4"),
            ("uint32_t etp_leverage_factor", "0", "ETP leverage | offset: 34, len: 4"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'R'", "Mesaj Türü (R = Stock Directory) | offset: 0, len: 1"),
            ("char stock[8]", "{}", "Hisse sembolü | offset: 11, len: 8"),      
            ("char market_category", "'\\0'", "Market kategorisi | offset: 19, len: 1"),
            ("char financial_status_indicator", "'\\0'", "Finansal durum kodu | offset: 20, len: 1"),
            ("char round_lots_only", "'\\0'", "Sadece round lot mu | offset: 25, len: 1"),
            ("char issue_classification", "'\\0'", "Issue sınıfı | offset: 26, len: 1"),
            ("char issue_sub_type[2]", "{}", "Issue alt türü | offset: 27, len: 2"),
            ("char authenticity", "'\\0'", "Authenticity kodu | offset: 29, len: 1"),
            ("char short_sale_threshold_indicator", "'\\0'", "Short sale threshold | offset: 30, len: 1"),
            ("char ipo_flag", "'\\0'", "IPO flag | offset: 31, len: 1"),
            ("char luld_reference_price_tier", "'\\0'", "LULD price tier | offset: 32, len: 1"),
            ("char etp_flag", "'\\0'", "ETP flag | offset: 33, len: 1"),
            ("char inverse_indicator", "'\\0'", "Inverse indicator | offset: 38, len: 1"),
            ("char pad[23]", None, "Doldurma (Padding)"),

        ],
    },
    {
        "name": "ITCHTradingStateMessage",
        "fields": [
            ("uint64_t timestamp", "0", "Olay zamanı | offset: 5, len: 6"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("uint16_t tracking_number", "0", "Tracking number | offset: 3, len: 2"),
            ("char message_type", "'H'", "Mesaj Türü (H = Trading State) | offset: 0, len: 1"),
            ("char stock[8]", "{}", "Hisse sembolü | offset: 11, len: 8"),
            ("char trading_state", "'T'", "T=Trading, H=Halted, P=Paused, Q=Quotation | offset: 19, len: 1"),
            ("char reserved", "'\\0'", "Reserved | offset: 20, len: 1"),
            ("char reason[4]", "{}", "Reason | offset: 21, len: 4"),
            ("char pad[37]", None, "Doldurma (Padding)"),
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
        "// NASDAQ ITCH Messages Struct Definitions",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace NASDAQ {\n",
    ]
    for msg in messages:
        lines.append(generate_struct(msg))
    return "\n".join(lines)

def messagetype_index(msg):
    for i, field in enumerate(msg["fields"]):
       if field[0] is "char message_type":
        return i  

def generate_enum(messages):
     line = [f"\nenum class ITCHTypes : uint8_t {{ "]
     for i, msg in enumerate(messages):
         messagetype = messagetype_index(msg)
         default = msg["fields"][messagetype][1]  
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownITCHtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func(messages):
    line = [f"\n\ninline constexpr ITCHTypes itchMessageIndex(char type) {{\n"]
    line.append("   switch(type) {")
    for i, msg in enumerate(messages):
        messagetype = messagetype_index(msg)
        default = msg["fields"][messagetype][1]  
        line.append(f"      case {default}: return ITCHTypes::{default.strip("'")};")
    line.append(f"      default: return ITCHTypes::unknownITCHtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 

file = generate_file(messages)
with open("../include/GeneratedITCHMessages_NASDAQ.h", "w") as f:
    f.write(file)
    f.write(generate_enum(messages))
    f.write(generate_func(messages))
    f.write("\n\n}\n")  
   



