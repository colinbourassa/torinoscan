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

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  Ui::MainWindow* ui;
  std::map<std::string,std::map<std::string,std::string>> m_carConfigFilenames;
  YAML::Node m_currentYAML;
  ProtocolIface m_iface;
  QThread m_ifaceThread;

  QMap<int,QString> getEnumVals(YAML::Node node) const;
  void setParamCheckboxStates(bool checked);
  bool scanDefinitionDir(std::string& errorMsgs);
  void populateCarPickList();

  bool createWidgetForMemoryParam(YAML::Node node, ParamWidgetGroup*& widget);
  bool createWidgetForStoredValueParam(YAML::Node node, ParamWidgetGroup*& widget);
  bool createWidgetForSnapshotParam(YAML::Node node, ParamWidgetGroup*& widget);
  void populateParamWidgets();
  void clearParamWidgets();
  void populateActuatorWidgets();
  void clearActuatorWidgets();
  void updateParamData(const QList<ParamWidgetGroup*>& paramWidgets);

signals:
  void requestThreadShutdown();
};
#endif // MAINWINDOW_H

