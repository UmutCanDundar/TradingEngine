#pragma once
#include <cstdint>

struct Endian
{
   // 8 bit -> endian farkı yok
   static constexpr uint8_t read_u8(const char* p) noexcept {
      return static_cast<uint8_t>(p[0]);
   }

   // 16 bit big endian read
   static constexpr uint16_t read_u16_be(const char* p) noexcept {
      return (uint16_t(uint8_t(p[0])) << 8) |
             (uint16_t(uint8_t(p[1])));
   }

   // 32 bit big endian read
   static constexpr uint32_t read_u32_be(const char* p) noexcept {
      return (uint32_t(uint8_t(p[0])) << 24) |
             (uint32_t(uint8_t(p[1])) << 16) |
             (uint32_t(uint8_t(p[2])) << 8)  |
             (uint32_t(uint8_t(p[3])));
   }

   // 64 bit big endian read
   static constexpr uint64_t read_u64_be(const char* p) noexcept {
      return (uint64_t(uint8_t(p[0])) << 56) |
             (uint64_t(uint8_t(p[1])) << 48) |
             (uint64_t(uint8_t(p[2])) << 40) |
             (uint64_t(uint8_t(p[3])) << 32) |
             (uint64_t(uint8_t(p[4])) << 24) |
             (uint64_t(uint8_t(p[5])) << 16) |
             (uint64_t(uint8_t(p[6])) << 8)  |
             uint64_t(uint8_t(p[7]));
   }

  // Big-endian yazım (network byte order)
// CPU'daki little endian değeri, gönderim öncesi network formatına çevirir
   static constexpr void write_u16_be(char* p, uint16_t v) noexcept {
      p[0] = static_cast<char>(v >> 8);
      p[1] = static_cast<char>(v);
   }

   static constexpr void write_u32_be(char* p, uint32_t v) noexcept {
      p[0] = static_cast<char>(v >> 24);
      p[1] = static_cast<char>(v >> 16);
      p[2] = static_cast<char>(v >> 8);
      p[3] = static_cast<char>(v);
   }

   static constexpr void write_u64_be(char* p, uint64_t v) noexcept {
      p[0] = static_cast<char>(v >> 56);
      p[1] = static_cast<char>(v >> 48);
      p[2] = static_cast<char>(v >> 40);
      p[3] = static_cast<char>(v >> 32);
      p[4] = static_cast<char>(v >> 24);
      p[5] = static_cast<char>(v >> 16);
      p[6] = static_cast<char>(v >> 8);
      p[7] = static_cast<char>(v);
   }

};
