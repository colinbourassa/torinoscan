#pragma once

#include <iceblock/KWP71.h>
#include <iceblock/Fiat9141.h>
#include <iceblock/Marelli1AF.h>

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

  BlockExchangeProtocol* m_baseIface = nullptr;
  std::shared_ptr<KWP71> m_kwp71 = nullptr;
  std::shared_ptr<Fiat9141> m_fiat9141 = nullptr;
  std::shared_ptr<Marelli1AF> m_marelli1AF = nullptr;
};

