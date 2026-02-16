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
#include "ecuconfig.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_ifaceThread(this),
  m_ecuConfig("ecu-defs/schema.json") // TODO: build this more intelligently
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
    if (m_ecuConfig.loadFile(filePath))
    {
      if (m_ecuConfig.isValid())
      {
        std::string carName;
        std::string ecuName;
        if (m_ecuConfig.getCarAndECUNames(carName, ecuName))
        {
          m_carConfigFilenames[carName][ecuName] = filePath;
        }
      }
      else
      {
      }
    }
    else
    {
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

bool MainWindow::setProtocol()
{
  ProtocolType protocol = ProtocolType::None;
  LineType initLine = LineType::None;
  std::string variant;
  int baud = 0;
  bool status = false;

  if (m_ecuConfig.getProtocolInfo(m_ecuAddr, protocol, baud, initLine, variant))
  {
    status = m_iface.setProtocol(protocol, baud, initLine, variant);
    if (!status)
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

void MainWindow::populateParamWidgets()
{
  int row = 0;
  int col = 0;

  clearParamWidgets();

  const auto paramDataList = m_ecuConfig.getParameterDefs();
  for (const auto& p : paramDataList)
  {
    ParamWidgetGroup* widget = nullptr;

    if (p.type == ParamType::MemoryAddress)
    {
      widget = new ParamWidgetGroup(p.name, p.memType, p.address, p.numByte, p.lsb, p.offset, p.units, p.enumVals, this);
    }
    else if (p.type == ParamType::StoredValue)
    {
      widget = new ParamWidgetGroup(p.name, p.id, p.lsb, p.offset, p.units, p.enumVals, this);
    }
    else if (p.type == ParamType::SnapshotLocation)
    {
      widget = new ParamWidgetGroup(p.name, p.snapshot, p.address, p.numByte, p.lsb, p.offset, p.units, p.enumVals, this);
    }

    if (widget)
    {
      ui->parametersGrid->addWidget(widget, row, col);
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

  const auto actuatorList = m_ecuConfig.getActuators();

  for (const auto [name, id] : actuatorList)
  {
    ActuatorWidgetGroup* actWidget = new ActuatorWidgetGroup(QString::fromStdString(name), id, this);
    ui->actuatorsGrid->addWidget(actWidget, row, col);
    // TODO: connect the button click signal

    if (++col > 1)
    {
      col = 0;
      row++;
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
    const std::string ecuCfgFilePath = m_carConfigFilenames.at(carName).at(ecuName);

    if (m_ecuConfig.loadFile(ecuCfgFilePath))
    {
      if (m_ecuConfig.isValid())
      {
        populateParamWidgets();
        populateActuatorWidgets();
        setProtocol();
      }
      else
      {
        //TODO
      }
    }
    else
    {
      //TODO
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

