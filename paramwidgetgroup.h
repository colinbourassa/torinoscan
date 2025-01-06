#ifndef PARAMWIDGETGROUP_H
#define PARAMWIDGETGROUP_H

#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QMap>

enum class ParamType
{
  MemoryAddress,
  StoredValue,
  SnapshotLocation
};

class ParamWidgetGroup : public QWidget
{
  Q_OBJECT
public:
  explicit ParamWidgetGroup(const QString& name,
                            ParamType type,
                            unsigned int location,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  explicit ParamWidgetGroup(const QString& name,
                            unsigned int snapshotPage,
                            unsigned int offsetInPage,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  void setChecked(bool checked);
  void setValue(int val);
  void clearValue();

  ParamType type() const;
  bool isChecked() const;
  unsigned int location() const;
  unsigned int snapshotPage() const;

private:
  /// A memory address, ID of a stored value, or offset within a snapshot page
  unsigned int m_location = 0;

  /// Page number (for snapshot parameter data only)
  unsigned int m_snapshotPage = 0;

  ParamType m_type;
  QMap<int,QString> m_enumVals;
  QHBoxLayout* m_hlayout = nullptr;
  QCheckBox* m_checkbox = nullptr;
  QLabel* m_valLabel = nullptr;
  QString m_units;

  void setupContainedWidgets(const QString& name);
};

#endif // PARAMWIDGETGROUP_H

