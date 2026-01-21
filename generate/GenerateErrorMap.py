# generate_strategies.py
import os
import errno

# Stratejileri sayısal enum gibi tanımlayalım
ERROR_STRATEGIES = ["Ignore", "Retry", "Abort", "Custom"]

errno_map = {
    "EAGAIN": "Retry",
    "EPIPE": "Abort",
    "ECONNRESET": "Retry",
    "EINTR": "Retry",
    "ENOMEM": "Abort",
    "EIO": "Abort",
    "EINVAL": "Abort",
    "EACCES": "Abort",
    "ENOSPC": "Abort",
    "EPERM": "Abort",
    "ESRCH": "Abort"
}
    
error_map = {
    "InvalidIP" : "Abort",
    "CouldNotOpenFile" : "Abort"
    }

errno_values = {}
for errname, errstrategy in errno_map.items():
    errnum = getattr(errno, errname, None)
    if errnum is not None:
        errno_values[errnum] = errstrategy

used_codes = sorted(errno_values.keys())
max_errno_used = max(used_codes)

def align_up(x, align):
    return ((x + align - 1) // align) * align
aligned_size = align_up(max_errno_used + 1, 64)

# Çıktı dosyasını üret
with open("../include/GeneratedErrorMap.h", "w") as f:
    f.write("// Generated automatically. DO NOT EDIT!\n\n")
    f.write("#pragma once\n")
    f.write("#include <array>\n#include <cstdint>\n\n")

    # Enum tanımı
    f.write("enum class ErrorStrategy : uint8_t {\n")
    for i, strname in enumerate(ERROR_STRATEGIES):
        f.write(f"    {strname} = {i},\n")
    f.write("};\n\n")

    f.write("enum class ErrorName : uint8_t {\n")
    for i, strname in enumerate(error_map.keys()):
        f.write(f"    {strname} = {i},\n")
    f.write("};\n\n")

    f.write(f" inline constexpr std::array<ErrorStrategy, {aligned_size}> errno_strategies = {{\n")

    # Index 0-255 için strateji ataması
    for i in range(aligned_size):
        if i in errno_values:
            f.write(f"    ErrorStrategy::{errno_values[i]}, // {i}: {os.strerror(i)} \n")
        else:    
            f.write(f"    ErrorStrategy::Ignore, // {i}: {os.strerror(i)}\n") 
    f.write(f"}};\n")

    f.write(f" alignas(64) inline constexpr std::array<ErrorStrategy, {len(error_map.keys())}> error_strategies = {{\n")

    # Index 0-255 için strateji ataması
    for errname in error_map.keys():
        errstrategy = error_map.get(errname,"Ignore")
        f.write(f"    ErrorStrategy::{errstrategy},\n")
    f.write(f"}};\n")
