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
using json = nlohmann::json;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  scanJSONDir();
  populateCarPickList();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::scanJSONDir()
{
  QDir jsonDir("json");
  // TODO: catch appropriate exception(s) from nlohmann
  foreach (const QFileInfo& file,
           jsonDir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks))
  {
    std::ifstream f(file.filePath().toStdString().c_str());
    nlohmann::json data = nlohmann::json::parse(f);

    const std::string carName(data.at("vehicle"));
    const std::string ecuName(data.at("ecu"));
    const std::string filePath = file.filePath().toStdString();
    m_carConfigFilenames[carName][ecuName] = filePath;
  }
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
  const std::string carName = arg1.toStdString();
  if (m_carConfigFilenames.count(carName))
  {
    for (auto it = m_carConfigFilenames.at(carName).begin();
         it != m_carConfigFilenames.at(carName).end(); it++)
    {
      ui->ecuComboBox->addItem(QString::fromStdString(it->first));
    }
  }
  else
  {
    ui->ecuComboBox->clear();
  }
}

void MainWindow::on_connectButton_clicked()
{
  const std::string carName = ui->carComboBox->currentText().toStdString();
  const std::string ecuName = ui->ecuComboBox->currentText().toStdString();
  if (m_carConfigFilenames.count(carName) && m_carConfigFilenames.at(carName).count(ecuName))
  {
    const std::string jsonFilepath = m_carConfigFilenames.at(carName).at(ecuName);
    std::ifstream f(jsonFilepath.c_str());
    m_currentJson = nlohmann::json::parse(f);

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

void MainWindow::populateParamWidgets()
{
  int row = 0;
  int col = 0;

  clearParamWidgets();

  for (auto paramEntry : m_currentJson.at("parameters"))
  {
    if (paramEntry.count("name") && paramEntry.count("address"))
    {
      bool ok = false;

      const QString paramName = QString::fromStdString(paramEntry.at("name"));
      const unsigned int paramAddr = QString::fromStdString(paramEntry.at("address")).toUInt(&ok, 16);
      const QString paramUnits = paramEntry.count("units") ?
        QString::fromStdString(paramEntry.at("units")) : QString();

      QMap<int,QString> enumVals;
      if (paramEntry.count("enum"))
      {
        for (const auto& [key, val] : paramEntry.at("enum").items())
        {
          const int numeric = QString::fromStdString(key).toInt(&ok, 10);
          if (ok)
          {
            enumVals[numeric] = QString::fromStdString(val);
          }
        }
      }

      ParamWidgetGroup* paramWidget = new ParamWidgetGroup(paramName, paramAddr, paramUnits, enumVals, this);
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

  for (auto actEntry : m_currentJson.at("actuators"))
  {
    if (actEntry.count("name") && actEntry.count("address"))
    {
      bool ok = false;

      const QString name = QString::fromStdString(actEntry.at("name"));
      const unsigned int addr = QString::fromStdString(actEntry.at("address")).toUInt(&ok, 16);

      ActuatorWidgetGroup* actWidget = new ActuatorWidgetGroup(name, addr, this);
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


