#ifndef PARAMWIDGETGROUP_H
#define PARAMWIDGETGROUP_H

#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QMap>

class ParamWidgetGroup : public QWidget
{
  Q_OBJECT
public:
  explicit ParamWidgetGroup(const QString& name,
                            unsigned int address,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  void setChecked(bool checked);
  void setValue(int val);
  void clearValue();

  bool isChecked() const;
  unsigned int address() const;

signals:

private:
  unsigned int m_address;
  QMap<int,QString> m_enumVals;
  QHBoxLayout* m_hlayout;
  QCheckBox* m_checkbox;
  QLabel* m_valLabel;
  QString m_units;
  //QLabel* m_unitLabel;

};

#endif // PARAMWIDGETGROUP_H
