#pragma once
#include <cstdint>

namespace Endian
{
   // Big to little unsigned read
   static constexpr uint16_t read_u16_be(const char* p) noexcept {
      return (uint16_t(uint8_t(p[0])) << 8) |
             (uint16_t(uint8_t(p[1])));
   }

   static constexpr uint32_t read_u32_be(const char* p) noexcept {
      return (uint32_t(uint8_t(p[0])) << 24) |
             (uint32_t(uint8_t(p[1])) << 16) |
             (uint32_t(uint8_t(p[2])) << 8)  |
             (uint32_t(uint8_t(p[3])));
   }

   static constexpr uint64_t read_u48_be(const char* p) noexcept {
      return (uint64_t(uint8_t(p[0])) << 40) |
             (uint64_t(uint8_t(p[1])) << 32) |
             (uint64_t(uint8_t(p[2])) << 24) |
             (uint64_t(uint8_t(p[3])) << 16) |
             (uint64_t(uint8_t(p[4])) << 8)  |
             uint64_t(uint8_t(p[5]));
   }

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


  // Big to little signed read
   static constexpr int16_t read_i16_be(const char *p) noexcept
   {
      return static_cast<int16_t>
            (
            (uint16_t(uint8_t(p[0])) << 8) |
            (uint16_t(uint8_t(p[1])))
            );
   }

   static constexpr int32_t read_i32_be(const char *p) noexcept
   {
      return static_cast<int32_t>
            (
             (uint32_t(uint8_t(p[0])) << 24) |
             (uint32_t(uint8_t(p[1])) << 16) |
             (uint32_t(uint8_t(p[2])) << 8) |
             (uint32_t(uint8_t(p[3])))
            );
   }

   static constexpr int64_t read_i64_be(const char *p) noexcept
   {
      return static_cast<int64_t>
            (
             (uint64_t(uint8_t(p[0])) << 56) |
             (uint64_t(uint8_t(p[1])) << 48) |
             (uint64_t(uint8_t(p[2])) << 40) |
             (uint64_t(uint8_t(p[3])) << 32) |
             (uint64_t(uint8_t(p[4])) << 24) |
             (uint64_t(uint8_t(p[5])) << 16) |
             (uint64_t(uint8_t(p[6])) << 8) |
             (uint64_t(uint8_t(p[7])))
            );
   }

   // Litte to big unsigned write
   static constexpr void write_u16_be(char *p, uint16_t v) noexcept
   {
      p[0] = char((v >> 8) & 0xFF);
      p[1] = char((v >> 0) & 0xFF);
   }

   static constexpr void write_u32_be(char *p, uint32_t v) noexcept
   {
      p[0] = char((v >> 24) & 0xFF);
      p[1] = char((v >> 16) & 0xFF);
      p[2] = char((v >> 8) & 0xFF);
      p[3] = char((v >> 0) & 0xFF);
   }

   static constexpr void write_u48_be(char *p, uint64_t v) noexcept
   {
      p[0] = char((v >> 40) & 0xFF);
      p[1] = char((v >> 32) & 0xFF);
      p[2] = char((v >> 24) & 0xFF);
      p[3] = char((v >> 16) & 0xFF);
      p[4] = char((v >> 8) & 0xFF);
      p[5] = char((v >> 0) & 0xFF);
   }

   static constexpr void write_u64_be(char *p, uint64_t v) noexcept
   {
      p[0] = char((v >> 56) & 0xFF);
      p[1] = char((v >> 48) & 0xFF);
      p[2] = char((v >> 40) & 0xFF);
      p[3] = char((v >> 32) & 0xFF);
      p[4] = char((v >> 24) & 0xFF);
      p[5] = char((v >> 16) & 0xFF);
      p[6] = char((v >> 8) & 0xFF);
      p[7] = char((v >> 0) & 0xFF);
   }

   
   // Litte to big signed write
   static constexpr void write_i16_be(char *p, int16_t v) noexcept
   {
      uint16_t u = static_cast<uint16_t>(v);
      p[0] = char((u >> 8) & 0xFF);
      p[1] = char((u >> 0) & 0xFF);
   }

   static constexpr void write_i32_be(char *p, int32_t v) noexcept
   {
      uint32_t u = static_cast<uint32_t>(v);
      p[0] = char((u >> 24) & 0xFF);
      p[1] = char((u >> 16) & 0xFF);
      p[2] = char((u >> 8) & 0xFF);
      p[3] = char((u >> 0) & 0xFF);
   }

   static constexpr void write_i64_be(char *p, int64_t v) noexcept
   {
      uint64_t u = static_cast<uint64_t>(v);
      p[0] = char((u >> 56) & 0xFF);
      p[1] = char((u >> 48) & 0xFF);
      p[2] = char((u >> 40) & 0xFF);
      p[3] = char((u >> 32) & 0xFF);
      p[4] = char((u >> 24) & 0xFF);
      p[5] = char((u >> 16) & 0xFF);
      p[6] = char((u >> 8) & 0xFF);
      p[7] = char((u >> 0) & 0xFF);
   }
};
