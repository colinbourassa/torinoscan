#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QList>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
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
           defDir.entryInfoList(QStringList() << "*.yaml", QDir::Files | QDir::NoSymLinks))
  {
    const std::string filePath = file.filePath().toStdString();

    try
    {
      YAML::Node data = YAML::LoadFile(filePath);
      const bool requiredFieldsPresent = data["vehicle"] && data["ecu"];

      if (requiredFieldsPresent)
      {
        const std::string carName = data["vehicle"].as<std::string>();
        std::string ecuName = data["ecu"].as<std::string>();
        if (data["position"])
        {
          ecuName += " (" + data["position"].as<std::string>() + ")";
        }
        m_carConfigFilenames[carName][ecuName] = filePath;
      }
      else
      {
        status = false;
        errorMsgs += "Required fields missing from '" + filePath + "'\n";
      }
    }
    catch (const YAML::ParserException& e)
    {
      status = false;
      errorMsgs += "Error parsing YAML: " + std::string(e.what()) + "\n";
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

bool MainWindow::parseProtocolNode(YAML::Node protocolNode)
{
  bool status = false;
  ProtocolType protocol = ProtocolType::None;
  LineType initLine = LineType::None;
  std::string variant;
  int baud = 0;

  if (protocolNode["family"])
  {
    protocol = protocolTypeFromString(protocolNode["family"].as<std::string>());
    status = (protocol != ProtocolType::None);
  }

  if (status && protocolNode["variant"])
  {
    variant = protocolNode["variant"].as<std::string>();
  }

  if (status && protocolNode["baud"])
  {
    try
    {
      baud = protocolNode["baud"].as<int>();
    }
    catch (const YAML::Exception& e)
    {
      status = false;
    }
  }

  if (status && protocolNode["address"])
  {
    try
    {
      m_ecuAddr = protocolNode["address"].as<int>();
    }
    catch (const YAML::Exception& e)
    {
      status = false;
    }
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

QMap<int,QString> MainWindow::getEnumVals(YAML::Node node) const
{
  QMap<int,QString> enumVals;
  if (node["enumvals"].IsSequence())
  {
    YAML::Node enumNode = node["enumvals"];
    for (YAML::const_iterator it = enumNode.begin(); it != enumNode.end(); it++)
    {
      const QString keyStr = QString::fromStdString(it->first.as<std::string>());
      bool ok = false;
      const int key = keyStr.toInt(&ok, 0);
      if (ok)
      {
        enumVals[key] = QString::fromStdString(it->second.as<std::string>());
      }
    }
  }
  return enumVals;
}

bool MainWindow::createWidgetForMemoryParam(YAML::Node node, ParamWidgetGroup*& widget)
{
  bool status = false;
  if (node["name"] && node["address"] && node["numbytes"])
  {
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const MemoryType memType = node["memtype"] ? memTypeFromString(node["memtype"].as<std::string>()) : MemoryType::Unspecified;
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    unsigned int addr = 0;
    unsigned int numBytes = 1;
    float lsb = 1.0f;
    float offset = 0.0f;

    try
    {
      status = true;
      addr = node["address"].as<unsigned int>();
      numBytes = node["numbytes"].as<unsigned int>();
      lsb = node["lsb"] ? node["lsb"].as<float>() : 1.0f;
      offset = node["offset"] ? node["offset"].as<float>() : 0.0f;
    }
    catch (const YAML::Exception& e)
    {
      status = false;
    }

    if (status)
    {
      widget = new ParamWidgetGroup(name, memType, addr, numBytes, lsb, offset, units, enumVals, this);
    }
  }

  return status;
}

bool MainWindow::createWidgetForStoredValueParam(YAML::Node node, ParamWidgetGroup*& widget)
{
  bool status = false;
  if (node["name"] && node["id"])
  {
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    unsigned int id = 0;
    float lsb = 1.0f;
    float offset = 0.0f;

    try
    {
      status = true;
      id = node["id"].as<unsigned int>();
      lsb = node["lsb"] ? node["lsb"].as<float>() : 1.0f;
      offset = node["offset"] ? node["offset"].as<float>() : 0.0f;
    }
    catch (const YAML::Exception& e)
    {
      status = false;
    }

    if (status)
    {
      widget = new ParamWidgetGroup(name, id, lsb, offset, units, enumVals, this);
    }
  }

  return status;
}

bool MainWindow::createWidgetForSnapshotParam(YAML::Node node, ParamWidgetGroup*& widget)
{
  bool status = false;
  if (node["name"] && node["snapshot"] && node["address"] && node["numbytes"])
  {
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    unsigned int snapshot = 0;
    unsigned int addr = 0;
    unsigned int numBytes = 1;
    float lsb = 1.0f;
    float offset = 0.0f;

    try
    {
      status = true;
      snapshot = node["snapshot"].as<unsigned int>();
      addr = node["address"].as<unsigned int>();
      numBytes = node["numbytes"].as<unsigned int>();
      lsb = node["lsb"] ? node["lsb"].as<float>() : 1.0f;
      offset = node["offset"] ? node["offset"].as<float>() : 0.0f;
    }
    catch (const YAML::Exception& e)
    {
      status = false;
    }

    if (status)
    {
      widget = new ParamWidgetGroup(name, snapshot, addr, numBytes, lsb, offset, units, enumVals, this);
    }
  }

  return status;
}

void MainWindow::populateParamWidgets()
{
  int row = 0;
  int col = 0;

  clearParamWidgets();

  for (auto node : m_currentYAML["parameters"])
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

  if (m_currentYAML["actuators"].IsSequence())
  {
    for (const auto& actEntry : m_currentYAML["actuators"])
    {
      if (actEntry["name"] && actEntry["id"])
      {
        bool ok = false;
        const QString name = QString::fromStdString(actEntry["name"].as<std::string>());
        const unsigned int id = actEntry["id"].as<unsigned int>();

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
    const std::string yamlFilepath = m_carConfigFilenames.at(carName).at(ecuName);
    m_currentYAML = YAML::LoadFile(yamlFilepath);

    populateParamWidgets();
    populateActuatorWidgets();

    if (m_currentYAML["protocol"])
    {
      parseProtocolNode(m_currentYAML["protocol"]);
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
