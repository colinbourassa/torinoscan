#pragma once

#include <QObject>
#include <QList>
#include <iceblock/BlockExchangeProtocol.h>
#include "paramwidgetgroup.h"
#include "protocoltype.h"

class ProtocolIface : public QObject
{
  Q_OBJECT

public:
  ProtocolIface();
  bool setFTDIDevice(uint8_t usbBusID, uint8_t usbDeviceID);
  bool setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant = "");
  void updateParamData();
  void setParamWidgetList(const QList<ParamWidgetGroup*>& paramWidgets);

public slots:
  void onShutdownRequest();
  void onConnectRequest(uint8_t ecuAddr);
  void onDisconnectRequest();
  void onStartParamRead();
  void onStopParamRead();
  void onRequestFaultCodes();

signals:
  void connected();
  void connectionFailed();
  void protocolParamsNotSet();
  void disconnected();

private:
  std::shared_ptr<BlockExchangeProtocol> m_iface = nullptr;
  const QList<ParamWidgetGroup*>* m_paramWidgetList = nullptr;
  int m_baud = 0;
  std::string m_currentVariant;
  uint8_t m_ftdiUSBBusID = 0;
  uint8_t m_ftdiUSBDeviceID = 0;
  bool m_protocolParamsSet = false;
  bool m_ftdiDeviceSet = false;
  std::mutex m_connectMutex;
  std::mutex m_shutdownMutex;;
  bool m_connectionActive = false;
  bool m_shutdownFlag = false;
  std::atomic<bool> m_stopParamRead = false;
};

