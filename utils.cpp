#include "utils.h"

MemoryType memTypeFromString(const std::string& s)
{
  std::string ls = s;
  std::transform(ls.begin(), ls.end(), ls.begin(),
    [](unsigned char c){ return std::tolower(c); });
  MemoryType type = MemoryType::Unspecified;

  if (ls == "ram")
  {
    type = MemoryType::RAM;
  }
  else if (ls == "rom")
  {
    type = MemoryType::ROM;
  }
  else if (ls == "eeprom")
  {
    type = MemoryType::EEPROM;
  }
  else if (ls == "fault")
  {
    type = MemoryType::Fault;
  }

  return type;
}

