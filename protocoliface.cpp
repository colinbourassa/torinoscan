#include "protocoliface.h"

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
      m_fiat9141 = nullptr;
      m_marelli1AF = nullptr;
      m_kwp71 = std::make_shared<KWP71>(baud, initLine, false);
      m_baseIface = m_kwp71.get();
    }
    else if (m_currentType == ProtocolType::FIAT9141)
    {
      m_kwp71 = nullptr;
      m_marelli1AF = nullptr;
      m_fiat9141 = std::make_shared<Fiat9141>(baud, initLine, false);
      m_baseIface = m_fiat9141.get();
    }
    else if (m_currentType == ProtocolType::Marelli1AF)
    {
      m_kwp71 = nullptr;
      m_fiat9141 = nullptr;
      m_marelli1AF = std::make_shared<Marelli1AF>(baud, initLine, false);
      m_baseIface = m_marelli1AF.get();
    }
    else
    {
      m_kwp71 = nullptr;
      m_fiat9141 = nullptr;
      m_marelli1AF = nullptr;
      m_baseIface = nullptr;
    }
  }

  m_currentVariant = variant;
}

bool ProtocolIface::connect(uint8_t bus, uint8_t addr, uint8_t ecuAddr)
{
  bool status = false;
  if (m_baseIface)
  {
    status = m_baseIface->connectByBusAddr(bus, addr, ecuAddr);
  }
  return status;
}

void ProtocolIface::disconnect()
{
  if (m_baseIface)
  {
    m_baseIface->disconnect();
  }
}

