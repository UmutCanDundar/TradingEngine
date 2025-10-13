messages = [
    {
        "name": "SBEHeader",
        "fields": [
            ("uint16_t blockLength", "0", "blok uzunluğu"),
            ("uint16_t templateId", "0", "mesaj tipi ID"),
            ("uint16_t schemaId", "0", "şema versiyonu"),
            ("uint16_t version", "0", "protokol versiyonu"),
        ],
    },
    {
        "name": "SBEAddOrderMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t orderId", "0", "sipariş ID"),
            ("uint32_t price", "0", "fiyat bilgisi"),
            ("uint32_t quantity", "0", "adet bilgisi"),
            ("uint8_t side", "0", "alış/satış yönü (0=buy, 1=sell)"),
            ("uint8_t pad[39]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBEDeleteOrderMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t orderId", "0", "iptal edilecek sipariş ID"),
            ("uint8_t pad[48]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBETradeMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t tradeId", "0", "işlem ID"),
            ("uint64_t orderId", "0", "ilgili sipariş ID"),
            ("uint32_t price", "0", "işlem fiyatı"),
            ("uint32_t quantity", "0", "işlem adedi"),
            ("uint8_t aggressorSide", "0", "işlem yönü (0=buy, 1=sell)"),
            ("uint8_t pad[31]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBEModifyOrderMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t orderId", "0", "değişecek sipariş ID"),
            ("uint32_t newQuantity", "0", "yeni miktar"),
            ("uint8_t pad[44]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBEHeartbeatMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t timestamp", "0", "zaman bilgisi (ns)"),
            ("uint8_t pad[48]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBEMarketStatusMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t instrumentId", "0", "ürün ID"),
            ("uint8_t marketState", "0", "piyasa durumu (0=kapalı, 1=açık)"),
            ("uint8_t pad[47]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBEInstrumentDefinitionMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t instrumentId", "0", "ürün ID"),
            ("char symbol[8]", "{}", "hisse adı"),
            ("uint8_t currencyCode[3]", "{'T','R','Y'}", "para birimi kodu"),
            ("uint8_t pad[37]", None, "padding (64 byte hizalama için)"),
        ],
    },
    {
        "name": "SBESequenceResetMessage",
        "fields": [
            ("SBEHeader header", None, "mesaj başlığı"),
            ("uint64_t newSeqNo", "0", "yeni sıra numarası"),
            ("uint8_t pad[48]", None, "padding (64 byte hizalama için)"),
        ],
    },
]

templateID = {
    "SBEAddOrderMessage": 0,
    "SBEDeleteOrderMessage": 1,
    "SBETradeMessage": 2,
    "SBEModifyOrderMessage": 3,
    "SBEHeartbeatMessage": 4,
    "SBEMarketStatusMessage": 5,
    "SBEInstrumentDefinitionMessage": 6,
    "SBESequenceResetMessage": 7,
}


def generate_struct(msg):
    struct_lines = [f"struct {msg['name']} {{" if msg['name'] == "SBEHeader" else f"struct alignas(64) {msg['name']} {{"]

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
     line = [f"\nenum SBETypes : uint8_t {{ "]
     for i, msg in enumerate(messages[1:]):
            types = f" {msg ["name"].upper()} = {i},"
            line.append(types)
     line.append(f" unknownSBEtype = 99")
     line.append(f"}};")
     return" ".join(line)

def generate_func(messages):
    line = [f"\n\ninline constexpr SBETypes MessageIndex(uint16_t templateId) {{\n"]
    line.append("   switch(templateId) {")
    for msg in messages[1:]:
        line.append(f"      case {templateID[msg["name"]]}: return SBETypes::{msg["name"].upper()}; ")
    line.append(f"      default: return SBETypes::unknownSBEtype;")
    line.append("   }")
    line.append("}")
    return "\n".join(line)

file = generate_file(messages)
with open("../include/GeneratedSBEMessages.h", "w") as f:
    f.write(file)
    f.write(generate_enum(messages))
    f.write(generate_func(messages))