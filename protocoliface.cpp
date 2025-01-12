#include "protocoliface.h"
#include "iceblock/KWP71.h"
#include "iceblock/Fiat9141.h"
#include "iceblock/Marelli1AF.h"

ProtocolIface::ProtocolIface()
{
}

void ProtocolIface::setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant)
{
  std::lock_guard<std::mutex> lock(m_connectMutex);

  if (!m_connectionActive)
  {
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
}

bool ProtocolIface::connect(uint8_t bus, uint8_t addr, uint8_t ecuAddr)
{
  std::lock_guard<std::mutex> lock(m_connectMutex);

  bool status = false;
  if (m_iface && !m_connectionActive)
  {
    status = m_iface->connectByBusAddr(bus, addr, ecuAddr);
    m_connectionActive = status;
  }
  return status;
}

void ProtocolIface::disconnect()
{
  std::lock_guard<std::mutex> lock(m_connectMutex);

  if (m_iface)
  {
    m_iface->disconnect();
  }
  m_connectionActive = false;
}

