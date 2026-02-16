#pragma once
#include <QMainWindow>
#include <QCloseEvent>
#include <QList>
#include <QThread>
#include <map>
#include "protocoliface.h"
#include "paramwidgetgroup.h"
#include "ecuconfig.h"
#include <iceblock/ftdi_enumerator.h>

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
  ProtocolIface m_iface;
  QThread m_ifaceThread;
  std::vector<FtdiDeviceInfo> m_ftdiDeviceInfo;
  uint8_t m_ecuAddr;
  ECUConfig m_ecuConfig;

  void setupThreadsAndSignals();
  void setParamCheckboxStates(bool checked);
  bool scanDefinitionDir(std::string& errorMsgs);
  void populateCarPickList();
  void populateParamWidgets();
  void clearParamWidgets();
  void populateActuatorWidgets();
  void clearActuatorWidgets();
  void updateParamData(const QList<ParamWidgetGroup*>& paramWidgets);
  bool setFTDIDeviceInfo();
  void refreshFTDIDeviceList();
  bool setProtocol();

signals:
  void connectInterface(uint8_t ecuAddr);
  void disconnectInterface();
  void requestThreadShutdown();
  void startParameterReading();
  void stopParameterReading();
  void requestFaultCodes();
  void activateAcuator(unsigned int index);
};

