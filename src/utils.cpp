#include "utils.h"
#include <endian.h>

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

ProtocolType protocolTypeFromString(const std::string& s)
{
  std::string ls = s;
  std::transform(ls.begin(), ls.end(), ls.begin(),
    [](unsigned char c){ return std::tolower(c); });
  ProtocolType type = ProtocolType::None;

  if (ls == "kwp71")
  {
    type = ProtocolType::KWP71;
  }
  else if (ls == "fiat9141")
  {
    type = ProtocolType::FIAT9141;
  }
  else if (ls == "marelli1af")
  {
    type = ProtocolType::Marelli1AF;
  }

  return type;
}

unsigned int vectorToUint(const std::vector<uint8_t>& v)
{
  return vectorToUint(v.data(), v.size());

}

unsigned int vectorToUint(const uint8_t* const data, unsigned int numBytes)
{
  unsigned int result = 0;

  if (numBytes == sizeof(uint32_t))
  {
    result = be32toh(*(reinterpret_cast<const uint32_t*>(data)));
  }
  else if (numBytes == sizeof(uint16_t))
  {
    result = be16toh(*(reinterpret_cast<const uint16_t*>(data)));
  }
  else if (numBytes == sizeof(uint8_t))
  {
    result = data[0];
  }

  return result;
}

