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
    populateParamWidgets();
  }
}

void MainWindow::populateParamWidgets()
{
  int row = 0;
  int col = 0;

  for (auto paramEntry : m_currentJson.at("parameters"))
  {
    if (paramEntry.count("name"))
    {
      const QString paramName = QString::fromStdString(paramEntry.at("name"));
      ui->parameterGrid->addWidget(new QCheckBox(paramName, ui->parametersScrollArea), row, col);
      ui->parameterGrid->addWidget(new QLabel("0", ui->parametersScrollArea), row, col + 1);
      if (paramEntry.count("units"))
      {
        const QString paramUnits = QString::fromStdString(paramEntry.at("units"));
        ui->parameterGrid->addWidget(new QLabel(paramUnits, ui->parametersScrollArea), row, col + 2);
      }
      if (col == 0)
      {
        col = 3;
      }
      else
      {
        col = 0;
        row++;
      }
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

void MainWindow::setParamCheckboxStates(bool checked)
{
  QList<QCheckBox*> childWidgets = ui->parametersScrollArea->findChildren<QCheckBox*>();
  foreach (QCheckBox* checkbox, childWidgets)
  {
    checkbox->setChecked(checked);
  }
}

