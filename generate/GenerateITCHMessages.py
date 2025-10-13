messages = [
    {
        "name": "ITCHAddOrderMessage",
        "fields": [
            ("char message_type", "'A'", "Mesaj Türü (A = Yeni Emir Ekle)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Mesaj zamanı (nanosanise cinsinden)"),
            ("uint64_t order_ref", "0", "Emir referans numarası (benzersiz)"),
            ("uint64_t stock_locate", "0", "Hisse senedi id"),
            ("char side", "'\\0'", "Emir yönü ('B' = Alış, 'S' = Satış)"),
            ("char pad2[3]", None, "Doldurma (Padding)"),
            ("uint32_t quantity", "0", "Emir miktarı"),
            ("uint32_t price", "0", "Fiyat (birim fiyat cinsinden)"),
            ("char pad3[20]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHAddOrderMPIDMessage",
        "fields": [
            ("char message_type", "'F'", "Mesaj Türü (F = MPID ile Yeni Emir Ekle)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Mesaj zamanı (nanosanise)"),
            ("uint64_t order_ref", "0", "Emir referans numarası"),
            ("uint64_t stock_locate", "0", "Hisse senedi id"),
            ("char side", "'\\0'", "Emir yönü ('B' = Alış, 'S' = Satış)"),
            ("char pad2[3]", None, "Doldurma (Padding)"),
            ("uint32_t quantity", "0", "Emir miktarı"),
            ("uint32_t price", "0", "Fiyat"),
            ("char mpid[4]", "{}", "Market Katılımcı ID'si (MPID)"),
            ("char pad3[16]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHCancelMessage",
        "fields": [
            ("char message_type", "'X'", "Mesaj Türü (X = Emir İptal)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "İptal zamanı"),
            ("uint64_t order_ref", "0", "İptal edilen emir referansı"),
            ("uint32_t cancelled_quantity", "0", "İptal edilen miktar"),
            ("char pad2[36]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHExecutedMessage",
        "fields": [
            ("char message_type", "'E'", "Mesaj Türü (E = Emir Gerçekleşti, fiyat yok)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Gerçekleşme zamanı"),
            ("uint64_t order_ref", "0", "Gerçekleşen emir referansı"),
            ("uint32_t executed_quantity", "0", "Gerçekleşen miktar"),
            ("char pad2[36]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHExecutedWithPriceMessage",
        "fields": [
            ("char message_type", "'C'", "Mesaj Türü (C = Emir Gerçekleşti, fiyat var)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Gerçekleşme zamanı"),
            ("uint64_t order_ref", "0", "Gerçekleşen emir referansı"),
            ("uint32_t executed_quantity", "0", "Gerçekleşen miktar"),
            ("uint32_t execution_price", "0", "Gerçekleşme fiyatı"),
            ("char pad2[32]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHDeleteMessage",
        "fields": [
            ("char message_type", "'D'", "Mesaj Türü (D = Emir Silindi)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Silinme zamanı"),
            ("uint64_t order_ref", "0", "Silinen emir referansı"),
            ("char pad2[40]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHTradeMessage",
        "fields": [
            ("char message_type", "'P'", "Mesaj Türü (P = İşlem)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "İşlem zamanı"),
            ("uint64_t order_ref", "0", "İlgili emir referansı"),
            ("uint64_t stock_locate", "0", "Hisse senedi id"),
            ("uint32_t quantity", "0", "İşlem miktarı"),
            ("uint32_t price", "0", "İşlem fiyatı"),
            ("char match_id[4]", "{}", "İşlem eşleşme ID'si"),
            ("char pad2[28]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHSystemEventMessage",
        "fields": [
            ("char message_type", "'S'", "Mesaj Türü (S = Sistem Olayı)"),
            ("char pad1[7]", None, "Doldurma (Padding)"),
            ("uint64_t timestamp", "0", "Olay zamanı"),
            ("char event_code", "'\\0'", "Olay kodu (ör: piyasa açılış/kapanış)"),
            ("char pad2[47]", None, "Doldurma (64 byte hizalama için)"),
        ],
    },
    {
        "name": "ITCHStockDirectoryMessage",
        "fields": [
            ("char message_type", "'R'", "Mesaj Türü (R = Stock Directory) | offset: 0, len: 1"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("char pad1[5]", None, "Padding | offset: 3, len: 5"),
            ("uint64_t timestamp", "0", "Olay zamanı | offset: 8, len: 8"),
            ("char stock[8]", "{}", "Hisse senedi sembolü | offset: 16, len: 8"),
            ("char pad2[40]", None, "Padding | offset: 24, len: 40"),
        ],
    },
    {
        "name": "ITCHTradingStateMessage",
        "fields": [
            ("char message_type", "'H'", "Mesaj Türü (H = Trading State) | offset: 0, len: 1"),
            ("uint16_t stock_locate", "0", "Hisse senedi ID | offset: 1, len: 2"),
            ("char pad1[5]", None, "Padding | offset: 3, len: 5"),
            ("uint64_t timestamp", "0", "Olay zamanı | offset: 8, len: 8"),
            ("char trading_state", "'T'", "T=Trading, H=Halted, P=Paused, Q=Quotation | offset: 24, len: 1"),
            ("char pad2[39]", None, "Padding | offset: 25, len: 39"),
        ],
    }
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
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
    ]
    for msg in messages:
        lines.append(generate_struct(msg))
    return "\n".join(lines)

def generate_enum(messages):
     line = [f"\nenum ITCHTypes : uint8_t {{ "]
     for i, msg in enumerate(messages):
         default = msg["fields"][0][1]  
         types = f" {default.strip("'")} = {i},"
         line.append(types)
     line.append(f" unknownITCHtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func(messages):
    line = [f"\n\ninline constexpr ITCHTypes MessageIndex(char type) {{\n"]
    line.append("   switch(type) {")
    for i, msg in enumerate(messages):
        default = msg["fields"][0][1]  
        line.append(f"      case {default}: return ITCHTypes::{default.strip("'")};")
    line.append(f"      default: return ITCHTypes::unknownITCHtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line) 

file = generate_file(messages)
with open("../include/GeneratedITCHMessages.h", "w") as f:
    f.write(file)
    f.write(generate_enum(messages))
    f.write(generate_func(messages))




