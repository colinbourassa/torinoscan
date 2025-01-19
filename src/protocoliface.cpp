#include <QThread>
#include <iceblock/KWP71.h>
#include <iceblock/Fiat9141.h>
#include <iceblock/Marelli1AF.h>
#include "protocoliface.h"
#include "utils.h"

ProtocolIface::ProtocolIface()
{
}

bool ProtocolIface::isConnected() const
{
  return m_connectionActive;
}

void ProtocolIface::onConnectRequest(uint8_t ecuAddr)
{
  std::lock_guard<std::mutex> lock(m_connectMutex);

  if (m_protocolParamsSet && m_ftdiDeviceSet)
  {
    if (m_iface &&
        !m_connectionActive &&
        m_iface->connectByBusAddr(m_ftdiUSBBusID, m_ftdiUSBDeviceID, ecuAddr))
    {
      m_connectionActive = true;
      emit connected();
    }
  }
  else
  {
    emit protocolParamsNotSet();
  }
}

void ProtocolIface::onDisconnectRequest()
{
  std::lock_guard<std::mutex> lock(m_connectMutex);

  if (m_iface)
  {
    m_iface->disconnect();
  }
  m_connectionActive = false;
}

void ProtocolIface::onStartParamRead()
{
  if (m_connectionActive && m_paramWidgetList)
  {
    m_stopParamRead = false;
    updateParamData();
  }
}

void ProtocolIface::onStopParamRead()
{
  m_stopParamRead = true;
}

void ProtocolIface::onRequestFaultCodes()
{
}

void ProtocolIface::onShutdownRequest()
{
  m_shutdownFlag = true;

  // Wait until updateParamData() finishes
  std::lock_guard<std::mutex> lock(m_shutdownMutex);

  QThread::currentThread()->quit();
}

bool ProtocolIface::setFTDIDevice(uint8_t ftdiUSBBusID, uint8_t ftdiUSBDeviceID)
{
  bool status = false;
  if (m_connectMutex.try_lock())
  {
    if (!m_connectionActive)
    {
      m_ftdiUSBBusID = ftdiUSBBusID;
      m_ftdiUSBDeviceID = ftdiUSBDeviceID;
      m_ftdiDeviceSet = true;
      status = true;
    }
    m_connectMutex.unlock();
  }
  return status;
}

bool ProtocolIface::setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant)
{
  bool status = false;

  if (m_connectMutex.try_lock())
  {
    if (!m_connectionActive)
    {
      if (type == ProtocolType::KWP71)
      {
        m_iface = std::make_shared<KWP71>(baud, initLine, false);
      }
      else if (type == ProtocolType::FIAT9141)
      {
        m_iface = std::make_shared<Fiat9141>(baud, initLine, false);
      }
      else if (type == ProtocolType::Marelli1AF)
      {
        m_iface = std::make_shared<Marelli1AF>(baud, initLine, false);
      }
      else
      {
        m_iface = nullptr;
      }
      status = (m_iface != nullptr);
      m_protocolParamsSet = status;
      m_currentVariant = variant;
    }
    m_connectMutex.unlock();
  }
  return status;
}

void ProtocolIface::setParamWidgetList(const QList<ParamWidgetGroup*>& paramWidgets)
{
  if (m_paramWidgetMutex.try_lock())
  {
    m_paramWidgetList = &paramWidgets;
    m_paramWidgetMutex.unlock();
  }
}

void ProtocolIface::updateParamData()
{
  std::lock_guard<std::mutex> shutdownLock(m_shutdownMutex);
  std::lock_guard<std::mutex> paramWidgetLock(m_paramWidgetMutex);

  std::map<unsigned int,std::vector<uint8_t>> cachedSnapshotPages;
  QList<ParamWidgetGroup*>::const_iterator widget = m_paramWidgetList->begin();

  while (!m_shutdownFlag &&
         !m_stopParamRead &&
         (widget != m_paramWidgetList->end()) &&
         ((*widget)->isChecked()))
  {
    const ParamType pType = (*widget)->paramType();

    if (std::vector<uint8_t> data;
        (pType == ParamType::MemoryAddress) &&
        m_iface->readMemory((*widget)->memoryType(), (*widget)->address(), (*widget)->numBytes(), data))
    {
      (*widget)->setRawValue(vectorToUint(data));
    }
    else if (std::vector<uint8_t> data;
             (pType == ParamType::StoredValue) &&
             m_iface->readStoredValue((*widget)->address(), data))
    {
      (*widget)->setRawValue(vectorToUint(data));
    }
    else if (pType == ParamType::SnapshotLocation)
    {
      const unsigned int pageNumber = (*widget)->snapshotPage();

      if (!cachedSnapshotPages.count(pageNumber))
      {
        std::vector<uint8_t> pageData;
        if (m_iface->readSnapshot(pageNumber, pageData))
        {
          cachedSnapshotPages[pageNumber] = pageData;
        }
      }

      if (cachedSnapshotPages.count(pageNumber) &&
          (((*widget)->address() + (*widget)->numBytes()) < cachedSnapshotPages.at(pageNumber).size()))
      {
        const uint8_t* const dataPtr =
          cachedSnapshotPages.at(pageNumber).data() + (*widget)->numBytes();

        (*widget)->setRawValue(vectorToUint(dataPtr, (*widget)->numBytes()));
      }
    }

    widget++;
  }
}

