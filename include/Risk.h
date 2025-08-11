#pragma once
#include <cstdint>

enum class RejectReason : uint8_t
{
    MaxPositionLimitExceeded = 1,  // Belirlenen pozisyon limitinin aşılması
    MaxOrderSizeExceeded = 2,      // Emir büyüklüğü limitinin aşılması
    MaxOpenOrdersReached = 3,      // Toplam açık emir sayısı limit aşımı
    MarketClosed = 4,              // Emir gönderilen zaman aralığında market kapalı
    OrderThrottleLimitReached = 5, // Kısa sürede çok fazla emir gönderimi
    SelfTradeDetected = 6,         // Aynı hesapta karşılıklı emir
    InvalidPriceRange = 7,         // Emir fiyatı market fiyatına göre geçersiz(çok uzak)
    DuplicateOrderID = 8,          // Aynı ID daha önce kullanılmış
    RiskEngineTimeout = 9,         // Risk kontrolleri zaman aşımına uğradı
    ForbiddenInstrument = 10       // Bu sembol trade edilemez olarak işaretli
};

class Risk
{
public:
    Risk();
    ~Risk();

private:
};
