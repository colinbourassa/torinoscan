#pragma once

#include <iceblock/BlockExchangeProtocol.h>

enum class ProtocolType
{
  None,
  KWP71,
  FIAT9141,
  Marelli1AF
};

class ProtocolIface
{
public:
  ProtocolIface();
  void setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant = "");
  bool connect(uint8_t bus, uint8_t addr, uint8_t ecuAddr);
  void disconnect();

private:
  ProtocolType m_currentType = ProtocolType::None;
  std::string m_currentVariant;
  std::shared_ptr<BlockExchangeProtocol> m_iface = nullptr;
};

