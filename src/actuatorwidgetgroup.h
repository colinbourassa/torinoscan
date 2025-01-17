#ifndef ACTUATORWIDGETGROUP_H
#define ACTUATORWIDGETGROUP_H

#include <QWidget>
#include <QPushButton>
#include <QString>
#include <QHBoxLayout>
#include <QLabel>

class ActuatorWidgetGroup : public QWidget
{
  Q_OBJECT
public:
  explicit ActuatorWidgetGroup(const QString& name,
                               unsigned int address,
                               QWidget* parent = nullptr);

signals:
  void buttonClicked(unsigned int address);

private slots:
  void onButtonClicked();

private:
  unsigned int m_address;
  QHBoxLayout* m_hlayout;
  QPushButton* m_button;
  QLabel* m_label;
};

#endif
