
venue_symbols = [ { "BIST": ["GARAN"]}, {"NASDAQ": ["AAPL"]} ]

def next_pow2(x: int) -> int:
    return 1 if x <= 1 else 1 << (x - 1).bit_length()

with open("../../generated/GeneratedSymbolTables.h", "w") as f:
    f.write("// Generated automatically. DO NOT EDIT!\n\n")
    f.write("#pragma once\n\n")
    f.write("#include <array>\n#include <cstdint>\n#include <span>\n\n")

    f.write("struct SymbolIndex { const char* symbol; uint32_t index; };\n\n")

    for ven_sym in venue_symbols:
      for ven,syms in ven_sym.items():
         size = next_pow2(len(syms))
         f.write(f"constexpr std::array<SymbolIndex, {size}> {ven}_symbols = {{{{\n")
         for syms in ven_sym.values():
            for i, sym in enumerate(syms):
               f.write(f'    {{"{sym}", {i}}},\n')
            for _ in range(size - len(syms)):
               f.write('    {"", UINT32_MAX},\n')
      f.write("}};\n\n")

    f.write("inline auto all_symbol_tables = std::to_array<std::span<const SymbolIndex>>({\n")  
    for ven_sym in venue_symbols:
       for ven in ven_sym.keys():
          f.write(f'    std::span{{{ven}_symbols}},\n')
    f.write("});\n\n")
          
         