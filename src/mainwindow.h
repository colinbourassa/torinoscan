#pragma once
#include <QMainWindow>
#include <QCloseEvent>
#include <QList>
#include <QThread>
#include <map>
#include <nlohmann/json.hpp>
#include "protocoliface.h"
#include "paramwidgetgroup.h"
#include <iceblock/ftdi_enumerator.h>

using json = nlohmann::json;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private slots:
  void on_carComboBox_currentTextChanged(const QString &arg1);
  void on_connectButton_clicked();
  void on_disconnectButton_clicked();
  void on_enableAllParamButton_clicked();
  void on_disableAllParamButton_clicked();
  void on_tabWidget_currentChanged(int index);
  void on_ftdiDeviceRefreshButton_clicked();
  void onInterfaceConnected();
  void onInterfaceConnectionError();
  void onInterfaceDisconnected();
  void onProtocolParamsNotSet();
  void on_setDefinitionButton_clicked();

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  Ui::MainWindow* ui;
  QList<ParamWidgetGroup*> m_paramWidgets;
  std::map<std::string,std::map<std::string,std::string>> m_carConfigFilenames;
  json m_currentJSON;
  ProtocolIface m_iface;
  QThread m_ifaceThread;
  std::vector<FtdiDeviceInfo> m_ftdiDeviceInfo;
  uint8_t m_ecuAddr;

  void setupThreadsAndSignals();
  void setParamCheckboxStates(bool checked);
  bool scanDefinitionDir(std::string& errorMsgs);
  void populateCarPickList();
  bool parseProtocolNode(const json& protocolNode);//, uint8_t& ecuAddr);
  bool createWidgetForMemoryParam(const json& node, ParamWidgetGroup*& widget);
  bool createWidgetForStoredValueParam(const json& node, ParamWidgetGroup*& widget);
  bool createWidgetForSnapshotParam(const json& node, ParamWidgetGroup*& widget);
  void populateParamWidgets();
  void clearParamWidgets();
  void populateActuatorWidgets();
  void clearActuatorWidgets();
  void updateParamData(const QList<ParamWidgetGroup*>& paramWidgets);
  bool setFTDIDeviceInfo();
  void refreshFTDIDeviceList();
  QMap<int,QString> getMapOfEnumVals(const json& node);

signals:
  void connectInterface(uint8_t ecuAddr);
  void disconnectInterface();
  void requestThreadShutdown();
  void startParameterReading();
  void stopParameterReading();
  void requestFaultCodes();
  void activateAcuator(unsigned int index);
};

