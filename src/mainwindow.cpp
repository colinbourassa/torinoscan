#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QList>
#include <fstream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "paramwidgetgroup.h"
#include "actuatorwidgetgroup.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_ifaceThread(this)
{
  ui->setupUi(this);
  setupThreadsAndSignals();
  std::string scanErrors;
  scanDefinitionDir(scanErrors);
  populateCarPickList();
  refreshFTDIDeviceList();

  ui->parametersGrid->setHorizontalSpacing(20);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::setupThreadsAndSignals()
{
  m_iface.moveToThread(&m_ifaceThread);
  connect(this, &MainWindow::connectInterface, &m_iface, &ProtocolIface::onConnectRequest);
  connect(this, &MainWindow::requestThreadShutdown, &m_iface, &ProtocolIface::onShutdownRequest);
  connect(this, &MainWindow::startParameterReading, &m_iface, &ProtocolIface::onStartParamRead);
  connect(this, &MainWindow::stopParameterReading, &m_iface, &ProtocolIface::onStopParamRead);
  connect(this, &MainWindow::requestFaultCodes, &m_iface, &ProtocolIface::onRequestFaultCodes);
  connect(&m_iface, &ProtocolIface::connected, this, &MainWindow::onInterfaceConnected);
  connect(&m_iface, &ProtocolIface::connectionFailed, this, &MainWindow::onInterfaceConnectionError);
  connect(&m_iface, &ProtocolIface::disconnected, this, &MainWindow::onInterfaceDisconnected);
  connect(&m_iface, &ProtocolIface::protocolParamsNotSet, this, &MainWindow::onProtocolParamsNotSet);
  m_ifaceThread.start();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (m_ifaceThread.isRunning())
  {
    emit requestThreadShutdown();
    m_ifaceThread.wait(3000);
  }
  event->accept();
}

void MainWindow::onInterfaceConnected()
{
  ui->disconnectButton->setEnabled(true);
  ui->statusLabel->setText("Status: connected");
  if (ui->tabWidget->currentIndex() == 0)
  {
    m_iface.setParamWidgetList(m_paramWidgets);
    emit startParameterReading();
  }
}

void MainWindow::onInterfaceConnectionError()
{
  ui->statusLabel->setText("Status: not connected");
  QMessageBox::warning(this, "Error", "Failed to connect.", QMessageBox::Ok);
  ui->connectButton->setEnabled(true);
}

void MainWindow::onInterfaceDisconnected()
{
  ui->connectButton->setEnabled(true);
  ui->statusLabel->setText("Status: not connected");
}

void MainWindow::onProtocolParamsNotSet()
{
  QMessageBox::warning(this, "Error", "Protocol parameters not set.", QMessageBox::Ok);
  ui->connectButton->setEnabled(true);
  ui->statusLabel->setText("Status: not connected");
}

bool MainWindow::scanDefinitionDir(std::string& errorMsgs)
{
  bool status = true;
  QDir defDir("ecu-defs");
  errorMsgs.clear();

  foreach (const QFileInfo& file,
           defDir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks))
  {
    const std::string filePath = file.filePath().toStdString();

    try
    {
      std::ifstream jsonFstream(filePath);
      json data = json::parse(jsonFstream);
      const bool requiredFieldsPresent = data.contains("vehicle") && data.contains("ecu");

      if (requiredFieldsPresent)
      {
        const std::string carName = data.at("vehicle");
        std::string ecuName = data.at("ecu");
        if (data.contains("position"))
        {
          ecuName += " (" + data.at("position").get<std::string>() + ")";
        }
        m_carConfigFilenames[carName][ecuName] = filePath;
      }
      else
      {
        status = false;
        errorMsgs += "Required fields missing from '" + filePath + "'\n";
      }
    }
    catch (const json::parse_error& e)
    {
      status = false;
      errorMsgs += "Error parsing JSON: " + std::string(e.what()) + "\n";
    }
  }

  return status;
}

void MainWindow::populateCarPickList()
{
  for (auto it = m_carConfigFilenames.begin(); it != m_carConfigFilenames.end(); it++)
  {
    ui->carComboBox->addItem(QString::fromStdString(it->first));
  }
}

bool MainWindow::setFTDIDeviceInfo()
{
  const int index = ui->ftdiDeviceComboBox->currentIndex();
  if (index >= 0)
  {
    const FtdiDeviceInfo& device = m_ftdiDeviceInfo.at(index);
    m_iface.setFTDIDevice(device.busNumber, device.deviceAddress);
  }

  return (index >= 0);
}

bool MainWindow::parseProtocolNode(const json& protocolNode)
{
  ProtocolType protocol = ProtocolType::None;
  LineType initLine = LineType::None;
  std::string variant;
  int baud = 0;
  bool status = true;

  try
  {
    m_ecuAddr = std::stoul(protocolNode.at("address").get<std::string>(), nullptr, 0);
    baud = protocolNode.at("baud");
    protocol = protocolTypeFromString(protocolNode.at("family"));
    variant = protocolNode.at("variant");
    status = (protocol != ProtocolType::None);
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }

  if (status)
  {
    if (!m_iface.setProtocol(protocol, baud, initLine, variant))
    {
      QMessageBox::warning(this, "Error", "Failed setting up protocol interface.", QMessageBox::Ok);
    }
  }
  else
  {
    QMessageBox::warning(this, "Error", "Failed parsing protocol node in config file.", QMessageBox::Ok);
  }

  return status;
}

QMap<int,QString> MainWindow::getMapOfEnumVals(const json& node)
{
  QMap<int,QString> enumVals;
  if (node.contains("enumvals"))
  {
   const std::map<std::string,std::string> enumValsStd = node.at("enumvals");
   for (auto const& [key, value] : enumValsStd)
   {
    enumVals.insert(std::stoi(key), QString::fromStdString(value));
   }
  }
  return enumVals;
}

bool MainWindow::createWidgetForMemoryParam(const json& node, ParamWidgetGroup*& widget)
{
  bool status = true;
  try
  {
    const QString name = QString::fromStdString(node.at("name"));
    const MemoryType memType = node.contains("memtype") ? memTypeFromString(node.at("memtype")) : MemoryType::Unspecified;
    const QString units = node.contains("units") ? QString::fromStdString(node.at("units")) : QString();
    const QMap<int,QString> enumVals = getMapOfEnumVals(node);

    const unsigned int addr = std::stoul(node.at("address").get<std::string>(), nullptr, 0);
    const unsigned int numBytes = node.at("numbytes");
    const float lsb = node.contains("lsb") ? node.at("lsb").get<float>() : 1.0f;
    const float offset = node.contains("offset") ? node.at("offset").get<float>() : 0.0f;
    widget = new ParamWidgetGroup(name, memType, addr, numBytes, lsb, offset, units, enumVals, this);
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }

  return status;
}

bool MainWindow::createWidgetForStoredValueParam(const json& node, ParamWidgetGroup*& widget)
{
  bool status = true;

  try
  {
    const QString name = QString::fromStdString(node.at("name"));
    const QString units = node.contains("units") ? QString::fromStdString(node.at("units")) : QString();
    const std::map<std::string,std::string> enumValsStd = node.at("enumvals");
    QMap<int,QString> enumVals;
    for (auto const& [key, value] : enumValsStd)
    {
      enumVals.insert(std::stoi(key), QString::fromStdString(value));
    }

    const unsigned int id = node.at("id");
    const float lsb = node.contains("lsb") ? node["lsb"].get<float>() : 1.0f;
    const float offset = node.contains("offset") ? node.at("offset").get<float>() : 0.0f;
    widget = new ParamWidgetGroup(name, id, lsb, offset, units, enumVals, this);
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }
  
  return status;
}

bool MainWindow::createWidgetForSnapshotParam(const json& node, ParamWidgetGroup*& widget)
{
  bool status = true;

  try
  {
    const QString name = QString::fromStdString(node.at("name"));
    const QString units = node.contains("units") ? QString::fromStdString(node.at("units")) : QString();
    const QMap<int,QString> enumVals = getMapOfEnumVals(node);
    const unsigned int snapshot = node.at("snapshot");
    const unsigned int addr = node.at("address");
    const unsigned int numBytes = node.at("numbytes");
    const float lsb = node.contains("lsb") ? node.at("lsb").get<float>() : 1.0f;
    const float offset = node.contains("offset") ? node.at("offset").get<float>() : 0.0f;
    widget = new ParamWidgetGroup(name, snapshot, addr, numBytes, lsb, offset, units, enumVals, this);
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }

  return status;
}

void MainWindow::populateParamWidgets()
{
  int row = 0;
  int col = 0;

  clearParamWidgets();

  for (auto node : m_currentJSON.at("parameters"))
  {
    ParamWidgetGroup* paramWidget = nullptr;

    // Check the node for all the required fields to define a memory offset, stored value, or snapshot parameter.
    if (createWidgetForMemoryParam(node, paramWidget) ||
        createWidgetForStoredValueParam(node, paramWidget) ||
        createWidgetForSnapshotParam(node, paramWidget))
    {
      ui->parametersGrid->addWidget(paramWidget, row, col);
      if (++col > 1)
      {
        col = 0;
        row++;
      }
    }
  }

  m_paramWidgets = ui->parametersScrollArea->findChildren<ParamWidgetGroup*>();
}

void MainWindow::clearParamWidgets()
{
  QList<ParamWidgetGroup*> childWidgets = ui->parametersScrollArea->findChildren<ParamWidgetGroup*>();
  foreach (ParamWidgetGroup* widget, childWidgets)
  {
    delete widget;
  }
}

void MainWindow::populateActuatorWidgets()
{
  int row = 0;
  int col = 0;

  clearActuatorWidgets();

  if (m_currentJSON.contains("actuators") && m_currentJSON.at("actuators").is_array())
  {
    for (const auto& actEntry : m_currentJSON.at("actuators"))
    {
      if (actEntry.contains("name") && actEntry.contains("id"))
      {
        const QString name = QString::fromStdString(actEntry.at("name"));
        const unsigned int id = actEntry.at("id");

        ActuatorWidgetGroup* actWidget = new ActuatorWidgetGroup(name, id, this);
        ui->actuatorsGrid->addWidget(actWidget, row, col);
        // TODO: connect the button click signal

        if (++col > 1)
        {
          col = 0;
          row++;
        }
      }
    }
  }
}

void MainWindow::clearActuatorWidgets()
{
  QList<ActuatorWidgetGroup*> childWidgets = ui->actuatorsScrollArea->findChildren<ActuatorWidgetGroup*>();
  foreach (ActuatorWidgetGroup* widget, childWidgets)
  {
    delete widget;
  }
}

void MainWindow::setParamCheckboxStates(bool checked)
{
  QList<ParamWidgetGroup*> childWidgets = ui->parametersScrollArea->findChildren<ParamWidgetGroup*>();
  foreach (ParamWidgetGroup* widget, childWidgets)
  {
    widget->setChecked(checked);
  }
}

void MainWindow::refreshFTDIDeviceList()
{
  m_ftdiDeviceInfo = enumerateFtdiDevices();
  ui->ftdiDeviceComboBox->clear();
  for (const FtdiDeviceInfo device : m_ftdiDeviceInfo)
  {
    const QString desc = QString("(%1:%2) %3 - %4")
      .arg(device.busNumber, 3, 10, QChar('0'))
      .arg(device.deviceAddress, 3, 10, QChar('0'))
      .arg(QString::fromStdString(device.manufacturer))
      .arg(QString::fromStdString(device.description));
    ui->ftdiDeviceComboBox->addItem(desc);
  }

  ui->connectButton->setEnabled(ui->ftdiDeviceComboBox->count() > 0);
}

void MainWindow::on_ftdiDeviceRefreshButton_clicked()
{
  refreshFTDIDeviceList();
}

void MainWindow::on_setDefinitionButton_clicked()
{
  const std::string carName = ui->carComboBox->currentText().toStdString();
  const std::string ecuName = ui->ecuComboBox->currentText().toStdString();

  if (m_carConfigFilenames.count(carName) && m_carConfigFilenames.at(carName).count(ecuName))
  {
    const std::string jsonFilepath = m_carConfigFilenames.at(carName).at(ecuName);
    std::ifstream jsonFile(jsonFilepath);
    m_currentJSON = json::parse(jsonFile);

    populateParamWidgets();
    populateActuatorWidgets();

    if (m_currentJSON.contains("protocol"))
    {
      parseProtocolNode(m_currentJSON.at("protocol"));
    }
    else
    {
      QMessageBox::warning(this, "Error", "Definition file does not contain valid protocol information.", QMessageBox::Ok);
    }
  }
}

void MainWindow::on_enableAllParamButton_clicked()
{
  setParamCheckboxStates(true);
}

void MainWindow::on_disableAllParamButton_clicked()
{
  setParamCheckboxStates(false);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
  if ((index == 0) && m_iface.isConnected())
  {
    emit startParameterReading();
  }
  else
  {
    emit stopParameterReading();
  }
}

void MainWindow::on_carComboBox_currentTextChanged(const QString& arg1)
{
  ui->ecuComboBox->clear();
  const std::string carName = arg1.toStdString();

  if (m_carConfigFilenames.count(carName))
  {
    for (auto it = m_carConfigFilenames.at(carName).begin();
         it != m_carConfigFilenames.at(carName).end(); it++)
    {
      ui->ecuComboBox->addItem(QString::fromStdString(it->first));
    }
  }
}

void MainWindow::on_connectButton_clicked()
{
  setFTDIDeviceInfo();
  ui->statusLabel->setText("Status: connecting...");
  emit connectInterface(m_ecuAddr);
  ui->connectButton->setEnabled(false);
}

void MainWindow::on_disconnectButton_clicked()
{
  ui->disconnectButton->setEnabled(false);
  emit disconnectInterface();
}
