#ifndef PARAMWIDGETGROUP_H
#define PARAMWIDGETGROUP_H

#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include "iceblock/BlockExchangeProtocol.h"

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
  // Parameter is read from a memory location
  explicit ParamWidgetGroup(const QString& name,
                            MemoryType memoryType,
                            unsigned int address,
                            int lsb,
                            int offset,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  // Parameter is a value stored by index
  explicit ParamWidgetGroup(const QString& name,
                            unsigned int valueId,
                            int lsb,
                            int offset,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  // Parameter is on a snapshot page
  explicit ParamWidgetGroup(const QString& name,
                            unsigned int snapshotPage,
                            unsigned int offsetInPage,
                            int lsb,
                            int offset,
                            const QString& units,
                            const QMap<int,QString>& enumVals,
                            QWidget* parent = nullptr);

  void setChecked(bool checked);
  void setRawValue(int val);
  void clearValue();

  ParamType paramType() const;
  MemoryType memoryType() const;
  unsigned int location() const;
  unsigned int snapshotPage() const;
  bool isChecked() const;

private:
  ParamType m_paramType;
  MemoryType m_memoryType = MemoryType::Unspecified;

  /// A memory address, ID of a stored value, or offset within a snapshot page
  unsigned int m_address = 0;

  /// Page number (for snapshot parameter data only)
  unsigned int m_snapshotPage = 0;

  /// Value of each LSB in the target units
  int m_lsb = 0;

  /// Offset in target units for a raw value of zero
  int m_offset = 0;

  QMap<int,QString> m_enumVals;
  QHBoxLayout* m_hlayout = nullptr;
  QCheckBox* m_checkbox = nullptr;
  QLabel* m_valLabel = nullptr;
  QString m_units;

  void setupContainedWidgets(const QString& name);
};

#endif // PARAMWIDGETGROUP_H

