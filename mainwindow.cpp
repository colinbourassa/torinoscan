#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QList>
#include "paramwidgetgroup.h"
#include "actuatorwidgetgroup.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  std::string scanErrors;
  scanDefinitionDir(scanErrors);
  populateCarPickList();
}

MainWindow::~MainWindow()
{
  delete ui;
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
  const std::string carName = ui->carComboBox->currentText().toStdString();
  const std::string ecuName = ui->ecuComboBox->currentText().toStdString();
  if (m_carConfigFilenames.count(carName) && m_carConfigFilenames.at(carName).count(ecuName))
  {
    const std::string yamlFilepath = m_carConfigFilenames.at(carName).at(ecuName);
    m_currentYAML = YAML::LoadFile(yamlFilepath);

    // TODO: clear out any old widgets
    populateParamWidgets();
    populateActuatorWidgets();
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
  }
}

void MainWindow::on_disconnectButton_clicked()
{
  // TODO: disconnect via the ECU interface
  ui->connectButton->setEnabled(true);
  ui->disconnectButton->setEnabled(false);
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
  if (node["name"] && node["address"])
  {
    bool ok = false;
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const MemoryType memType; // TODO
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    const unsigned int addr = QString::fromStdString(node["address"].as<std::string>()).toUInt(&ok, 0);

    if (ok)
    {
      status = true;
      widget = new ParamWidgetGroup(name, memType, addr, lsb, offset, units, enumVals, this);
    }
  }
  return status;
}

bool MainWindow::createWidgetForStoredValueParam(YAML::Node node, ParamWidgetGroup*& widget)
{
  bool status = false;
  if (node["name"] && node["id"])
  {
    bool ok = false;
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    const unsigned int id = QString::fromStdString(node["id"].as<std::string>()).toUInt(&ok, 0);

    if (ok)
    {
      status = true;
      widget = new ParamWidgetGroup(name, id, lsb, offset, units, enumVals, this);
    }
  }
  return status;
}

bool MainWindow::createWidgetForSnapshotParam(YAML::Node node, ParamWidgetGroup*& widget)
{
  bool status = false;
  if (node["name"] && node["snapshot"] && node["offset"])
  {
    const QString name = QString::fromStdString(node["name"].as<std::string>());
    const QString units = node["units"] ? QString::fromStdString(node["units"].as<std::string>()) : QString();
    const QMap<int,QString> enumVals = getEnumVals(node);
    bool pageOk = false;
    bool offsetOk = false;
    const unsigned int page = QString::fromStdString(node["snapshot"].as<std::string>()).toUInt(&pageOk, 0);
    const unsigned int offset = QString::fromStdString(node["offset"].as<std::string>()).toUInt(&offsetOk, 0);

    if (pageOk && offsetOk)
    {
      status = true;
      widget = new ParamWidgetGroup(name, page, offset, lsb, offset, units, enumVals, this);
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

void MainWindow::on_enableAllParamButton_clicked()
{
  setParamCheckboxStates(true);
}

void MainWindow::on_disableAllParamButton_clicked()
{
  setParamCheckboxStates(false);
}

void MainWindow::setParamCheckboxStates(bool checked)
{
  QList<ParamWidgetGroup*> childWidgets = ui->parametersScrollArea->findChildren<ParamWidgetGroup*>();
  foreach (ParamWidgetGroup* widget, childWidgets)
  {
    widget->setChecked(checked);
  }
}

void MainWindow::updateParamData(const QList<ParamWidgetGroup*>& paramWidgets)
{
  m_iface.updateParamData(paramWidgets);
}

