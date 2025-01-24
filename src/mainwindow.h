#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QList>
#include <QThread>
#include <map>
#include <yaml-cpp/yaml.h>
#include "protocoliface.h"
#include "paramwidgetgroup.h"
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

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  Ui::MainWindow* ui;
  QList<ParamWidgetGroup*> m_paramWidgets;
  std::map<std::string,std::map<std::string,std::string>> m_carConfigFilenames;
  YAML::Node m_currentYAML;
  ProtocolIface m_iface;
  QThread m_ifaceThread;
  std::vector<FtdiDeviceInfo> m_ftdiDeviceInfo;

  void setupThreadsAndSignals();
  QMap<int,QString> getEnumVals(YAML::Node node) const;
  void setParamCheckboxStates(bool checked);
  bool scanDefinitionDir(std::string& errorMsgs);
  void populateCarPickList();
  bool parseProtocolNode(YAML::Node protocolNode, uint8_t& ecuAddr);
  bool createWidgetForMemoryParam(YAML::Node node, ParamWidgetGroup*& widget);
  bool createWidgetForStoredValueParam(YAML::Node node, ParamWidgetGroup*& widget);
  bool createWidgetForSnapshotParam(YAML::Node node, ParamWidgetGroup*& widget);
  void populateParamWidgets();
  void clearParamWidgets();
  void populateActuatorWidgets();
  void clearActuatorWidgets();
  void updateParamData(const QList<ParamWidgetGroup*>& paramWidgets);
  bool setFTDIDeviceInfo();
  void refreshFTDIDeviceList();

signals:
  void connectInterface(uint8_t ecuAddr);
  void disconnectInterface();
  void requestThreadShutdown();
  void startParameterReading();
  void stopParameterReading();
  void requestFaultCodes();
  void activateAcuator(unsigned int index);
};
#endif // MAINWINDOW_H

