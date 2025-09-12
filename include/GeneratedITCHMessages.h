// Generated automatically. DO NOT EDIT!
#pragma once

#include <cstdint>

struct alignas(64) ITCHAddOrderMessage {
    char message_type = 'A';        // Mesaj Türü (A = Yeni Emir Ekle)
    char pad1[7];                   // Doldurma (Padding)
    uint64_t timestamp = 0;         // Mesaj zamanı (nanosanise cinsinden)
    uint64_t order_ref = 0;         // Emir referans numarası (benzersiz)
    char side = '\0';               // Emir yönü ('B' = Alış, 'S' = Satış)
    char pad2[3];                   // Doldurma (Padding)
    uint32_t quantity = 0;          // Emir miktarı
    char stock[8] = "";             // Hisse senedi sembolü (8 karakter)
    uint32_t price = 0;             // Fiyat (birim fiyat cinsinden)
    char pad3[20];                  // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHAddOrderMPIDMessage {
    char message_type = 'F';        // Mesaj Türü (F = MPID ile Yeni Emir Ekle)
    char pad1[7];                   // Doldurma (Padding)
    uint64_t timestamp = 0;         // Mesaj zamanı (nanosanise)
    uint64_t order_ref = 0;         // Emir referans numarası
    char side = '\0';               // Emir yönü ('B' = Alış, 'S' = Satış)
    char pad2[3];                   // Doldurma (Padding)
    uint32_t quantity = 0;          // Emir miktarı
    char stock[8] = "";             // Hisse senedi sembolü
    uint32_t price = 0;             // Fiyat
    char mpid[4] = "";              // Market Katılımcı ID'si (MPID)
    char pad3[16];                  // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHCancelMessage {
    char message_type = 'X';                // Mesaj Türü (X = Emir İptal)
    char pad1[7];                           // Doldurma (Padding)
    uint64_t timestamp = 0;                 // İptal zamanı
    uint64_t order_ref = 0;                 // İptal edilen emir referansı
    uint32_t cancelled_quantity = 0;        // İptal edilen miktar
    char pad2[36];                          // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHExecutedMessage {
    char message_type = 'E';               // Mesaj Türü (E = Emir Gerçekleşti, fiyat yok)
    char pad1[7];                          // Doldurma (Padding)
    uint64_t timestamp = 0;                // Gerçekleşme zamanı
    uint64_t order_ref = 0;                // Gerçekleşen emir referansı
    uint32_t executed_quantity = 0;        // Gerçekleşen miktar
    char pad2[36];                         // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHExecutedWithPriceMessage {
    char message_type = 'C';               // Mesaj Türü (C = Emir Gerçekleşti, fiyat var)
    char pad1[7];                          // Doldurma (Padding)
    uint64_t timestamp = 0;                // Gerçekleşme zamanı
    uint64_t order_ref = 0;                // Gerçekleşen emir referansı
    uint32_t executed_quantity = 0;        // Gerçekleşen miktar
    uint32_t execution_price = 0;          // Gerçekleşme fiyatı
    char pad2[32];                         // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHDeleteMessage {
    char message_type = 'D';        // Mesaj Türü (D = Emir Silindi)
    char pad1[7];                   // Doldurma (Padding)
    uint64_t timestamp = 0;         // Silinme zamanı
    uint64_t order_ref = 0;         // Silinen emir referansı
    char pad2[40];                  // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHTradeMessage {
    char message_type = 'P';        // Mesaj Türü (P = İşlem)
    char pad1[7];                   // Doldurma (Padding)
    uint64_t timestamp = 0;         // İşlem zamanı
    uint64_t order_ref = 0;         // İlgili emir referansı
    char stock[8] = "";             // Hisse senedi sembolü
    uint32_t quantity = 0;          // İşlem miktarı
    uint32_t price = 0;             // İşlem fiyatı
    char match_id[4] = "";          // İşlem eşleşme ID'si
    char pad2[28];                  // Doldurma (64 byte hizalama için)
};

struct alignas(64) ITCHSystemEventMessage {
    char message_type = 'S';        // Mesaj Türü (S = Sistem Olayı)
    char pad1[7];                   // Doldurma (Padding)
    uint64_t timestamp = 0;         // Olay zamanı
    char event_code = '\0';         // Olay kodu (ör: piyasa açılış/kapanış)
    char pad2[47];                  // Doldurma (64 byte hizalama için)
};

enum ITCHTypes : uint8_t {   A = 0,  F = 1,  X = 2,  E = 3,  C = 4,  D = 5,  P = 6,  S = 7,  unknownITCHtype = 99 };

inline constexpr ITCHTypes MessageIndex(char type) {

   switch(type) {
      case 'A': return ITCHTypes::A;
      case 'F': return ITCHTypes::F;
      case 'X': return ITCHTypes::X;
      case 'E': return ITCHTypes::E;
      case 'C': return ITCHTypes::C;
      case 'D': return ITCHTypes::D;
      case 'P': return ITCHTypes::P;
      case 'S': return ITCHTypes::S;
      default: return ITCHTypes::unknownITCHtype;
   }
}