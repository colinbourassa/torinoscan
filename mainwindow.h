#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <map>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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
  void on_enableAllParamButton_clicked();
  void on_disableAllParamButton_clicked();

private:
  Ui::MainWindow* ui;
  std::map<std::string,std::map<std::string,std::string>> m_carConfigFilenames;
  json m_currentJson;

  void setParamCheckboxStates(bool checked);
  void scanJSONDir();
  void populateCarPickList();
  void populateParamWidgets();
};
#endif // MAINWINDOW_H
