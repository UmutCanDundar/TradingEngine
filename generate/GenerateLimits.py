account_limit = {
 "max_notional": ["__int128",0],
 "max_position": ["int64_t",0],
 "max_daily_loss": ["int64_t",0],
 "max_leverage": ["double",0],   
 "max_open_orders": ["uint32_t",0]
}

venues_limit = {
    "BIST": [5,1000000],
    "NYSE": [5,1000000],
    "NASDAQ":[5,1000000]
}

def generate_account(account_limit):
     line = [f"\nstruct alignas(64) AccountLimits \n{{ "]
     for limitname, values in account_limit.items():
         line.append(f"       {values[0]} {limitname} = {values[1]};")
     line.append(f"       uint8_t pad[36];")
     line.append(f"}};")
     return"\n".join(line)

def generate_venue(venues_limit):
     line = [f"\n\nconst std::unordered_map<Venue,std::pair<uint64_t,uint64_t>> VenueLimits \n{{ "]
     for venuename, values in venues_limit.items():
         line.append(f'       {{Venue::{venuename}, {{{values[0]},{values[1]}}} }},')
     line.append(f"}};")
     return"\n".join(line)

with open("../include/GeneratedLimits.h", "w") as f:
      f.write("// Generated automatically. DO NOT EDIT!\n\n")
      f.write("#pragma once\n\n")
      f.write('#include <unordered_map>\n#include <cstdint>\n#include "Protocol-Venue.h"\n\n')
      f.write(generate_account(account_limit))
      f.write(generate_venue(venues_limit))
     
      


