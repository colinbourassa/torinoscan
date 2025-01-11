#include "protocoliface.h"
#include "iceblock/KWP71.h"
#include "iceblock/Fiat9141.h"
#include "iceblock/Marelli1AF.h"

ProtocolIface::ProtocolIface()
{
}

void ProtocolIface::setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant)
{
  // TODO: Ensure that the protocol is not changed while a connection is active.

  if (m_currentType != type)
  {
    m_currentType = type;
    if (m_currentType == ProtocolType::KWP71)
    {
      m_iface = std::make_shared<KWP71>(baud, initLine, false);
    }
    else if (m_currentType == ProtocolType::FIAT9141)
    {
      m_iface = std::make_shared<Fiat9141>(baud, initLine, false);
    }
    else if (m_currentType == ProtocolType::Marelli1AF)
    {
      m_iface = std::make_shared<Marelli1AF>(baud, initLine, false);
    }
    else
    {
      m_iface = nullptr;
    }
  }

  m_currentVariant = variant;
}

bool ProtocolIface::connect(uint8_t bus, uint8_t addr, uint8_t ecuAddr)
{
  bool status = false;
  if (m_iface)
  {
    status = m_iface->connectByBusAddr(bus, addr, ecuAddr);
  }
  return status;
}

void ProtocolIface::disconnect()
{
  if (m_iface)
  {
    m_iface->disconnect();
  }
}

