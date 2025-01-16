#pragma once

#include <QObject>
#include <QList>
#include <iceblock/BlockExchangeProtocol.h>
#include "paramwidgetgroup.h"

enum class ProtocolType
{
  None,
  KWP71,
  FIAT9141,
  Marelli1AF
};

class ProtocolIface : public QObject
{
  Q_OBJECT

public:
  ProtocolIface();
  void setProtocol(ProtocolType type, int baud, LineType initLine, const std::string& variant = "");
  bool connect(uint8_t bus, uint8_t addr, uint8_t ecuAddr);
  void disconnect();
  void updateParamData(const QList<ParamWidgetGroup*>& paramWidgets);

public slots:
  void onShutdownRequest();

private:
  ProtocolType m_currentType = ProtocolType::None;
  std::string m_currentVariant;
  std::shared_ptr<BlockExchangeProtocol> m_iface = nullptr;
  std::mutex m_connectMutex;
  std::mutex m_shutdownMutex;;
  bool m_connectionActive = false;
  bool m_shutdownFlag = false;
};

