// Generated automatically. DO NOT EDIT!
#pragma once

#include <cstdint>

struct SBEHeader
{
    uint16_t blockLength = 0; // blok uzunluğu
    uint16_t templateId = 0;  // mesaj tipi ID
    uint16_t schemaId = 0;    // şema versiyonu
    uint16_t version = 0;
};

struct alignas(64) SBEAddOrderMessage
{
    SBEHeader header;      // mesaj başlığı
    uint64_t orderId = 0;  // sipariş ID
    uint32_t price = 0;    // fiyat bilgisi
    uint32_t quantity = 0; // adet bilgisi
    uint8_t side = 0;      // alış/satış yönü (0=buy, 1=sell)
    uint8_t pad[39];       // padding (64 byte hizalama için)
};

struct alignas(64) SBEDeleteOrderMessage
{
    SBEHeader header;     // mesaj başlığı
    uint64_t orderId = 0; // iptal edilecek sipariş ID
    uint8_t pad[48];      // padding (64 byte hizalama için)
};

struct alignas(64) SBETradeMessage
{
    SBEHeader header;          // mesaj başlığı
    uint64_t tradeId = 0;      // işlem ID
    uint64_t orderId = 0;      // ilgili sipariş ID
    uint32_t price = 0;        // işlem fiyatı
    uint32_t quantity = 0;     // işlem adedi
    uint8_t aggressorSide = 0; // işlem yönü (0=buy, 1=sell)
    uint8_t pad[31];           // padding (64 byte hizalama için)
};

struct alignas(64) SBEModifyOrderMessage
{
    SBEHeader header;         // mesaj başlığı
    uint64_t orderId = 0;     // değişecek sipariş ID
    uint32_t newQuantity = 0; // yeni miktar
    uint8_t pad[44];          // padding (64 byte hizalama için)
};

struct alignas(64) SBEHeartbeatMessage
{
    SBEHeader header;       // mesaj başlığı
    uint64_t timestamp = 0; // zaman bilgisi (ns)
    uint8_t pad[48];        // padding (64 byte hizalama için)
};

struct alignas(64) SBEMarketStatusMessage
{
    SBEHeader header;          // mesaj başlığı
    uint64_t instrumentId = 0; // ürün ID
    uint8_t marketState = 0;   // piyasa durumu (0=kapalı, 1=açık)
    uint8_t pad[47];           // padding (64 byte hizalama için)
};

struct alignas(64) SBEInstrumentDefinitionMessage
{
    SBEHeader header;                          // mesaj başlığı
    uint64_t instrumentId = 0;                 // ürün ID
    uint32_t lotSize = 0;                      // lot büyüklüğü
    uint8_t currencyCode[3] = {'T', 'R', 'Y'}; // para birimi kodu
    uint8_t pad[41];                           // padding (64 byte hizalama için)
};

struct alignas(64) SBESequenceResetMessage
{
    SBEHeader header;      // mesaj başlığı
    uint64_t newSeqNo = 0; // yeni sıra numarası
    uint8_t pad[48];       // padding (64 byte hizalama için)
};

enum SBETypes : uint8_t
{
    SBEADDORDERMESSAGE = 0,
    SBEDELETEORDERMESSAGE = 1,
    SBETRADEMESSAGE = 2,
    SBEMODIFYORDERMESSAGE = 3,
    SBEHEARTBEATMESSAGE = 4,
    SBEMARKETSTATUSMESSAGE = 5,
    SBEINSTRUMENTDEFINITIONMESSAGE = 6,
    SBESEQUENCERESETMESSAGE = 7,
    unknownSBEtype = 99
};

inline constexpr SBETypes MessageIndex(uint16_t templateId)
{

    switch (templateId)
    {
    case 0:
        return SBETypes::SBEADDORDERMESSAGE;
    case 1:
        return SBETypes::SBEDELETEORDERMESSAGE;
    case 2:
        return SBETypes::SBETRADEMESSAGE;
    case 3:
        return SBETypes::SBEMODIFYORDERMESSAGE;
    case 4:
        return SBETypes::SBEHEARTBEATMESSAGE;
    case 5:
        return SBETypes::SBEMARKETSTATUSMESSAGE;
    case 6:
        return SBETypes::SBEINSTRUMENTDEFINITIONMESSAGE;
    case 7:
        return SBETypes::SBESEQUENCERESETMESSAGE;
    default:
        return SBETypes::unknownSBEtype;
    }
}