#include <string>
#include <cstdint>
#include <iceblock/BlockExchangeProtocol.h>
#include "protocoltype.h"

MemoryType memTypeFromString(const std::string& s);

ProtocolType protocolTypeFromString(const std::string& s);

unsigned int vectorToUint(const std::vector<uint8_t>& v);

unsigned int vectorToUint(const uint8_t* const data, unsigned int numBytes);

