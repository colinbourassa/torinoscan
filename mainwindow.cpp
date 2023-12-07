#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  populatePickLists();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::populatePickLists()
{
  QDir jsonDir("json");
  // TODO: catch appropriate exception(s) from nlohmann
  foreach (const QFileInfo& file,
           jsonDir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks))
  {
    std::ifstream f(file.filePath().toStdString().c_str());
    nlohmann::json data = nlohmann::json::parse(f);
    std::string carName(data.at("vehicle"));
    QString qCarName = QString::fromStdString(carName);
    ui->carComboBox->addItem(qCarName);
  }
}
