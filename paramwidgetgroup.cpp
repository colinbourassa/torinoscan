#include "paramwidgetgroup.h"

/**
 * Constructor used for parameters read from a memory location.
 */
ParamWidgetGroup::ParamWidgetGroup(const QString& name,
                                   MemoryType memoryType,
                                   unsigned int address,
                                   int lsb,
                                   int offset,
                                   const QMap<int,QString>& enumVals,
                                   const QString& units,
                                   QWidget* parent) :
  QWidget(parent),
  m_paramType(ParamType::MemoryAddress),
  m_memoryType(memoryType),
  m_location(location),
  m_lsb(lsb),
  m_offset(offset),
  m_enumVals(enumVals),
  m_units(units)
{
  setupContainedWidgets(name);
  clearValue();
}

/**
 * Constructor used for parameters that are stored values, accessed by index.
 */
ParamWidgetGroup::ParamWidgetGroup(const QString& name,
                                   unsigned int valueId,
                                   int lsb,
                                   int offset,
                                   const QMap<int,QString>& enumVals,
                                   const QString& units,
                                   QWidget* parent) :
  QWidget(parent),
  m_paramType(ParamType::StoredValue),
  m_memoryType(MemoryType::Unspecified),
  m_address(valueId),
  m_lsb(lsb),
  m_offset(offset),
  m_enumVals(enumVals),
  m_units(units)
{
  setupContainedWidgets(name);
  clearValue();
}

/**
 * Constructor used for parameters that are on a snapshot data page.
 */
ParamWidgetGroup::ParamWidgetGroup(const QString& name,
                                   unsigned int snapshotPage,
                                   unsigned int offsetInPage,
                                   int lsb,
                                   int offset,
                                   const QString& units,
                                   const QMap<int,QString>& enumVals,
                                   QWidget* parent) :
  QWidget(parent),
  m_paramType(ParamType::SnapshotLocation),
  m_address(offsetInPage),
  m_snapshotPage(snapshotPage),
  m_lsb(lsb),
  m_offset(offset),
  m_enumVals(enumVals),
  m_units(units)
{
  setupContainedWidgets(name);
  clearValue();
}
                       
ParamType ParamWidgetGroup::paramType() const
{
  return m_paramType;
}

MemoryType ParamWidgetGroup::memoryType() const
{
  return m_memoryType;
}

void ParamWidgetGroup::setupContainedWidgets(const QString& name)
{
  m_hlayout = new QHBoxLayout(this);
  m_checkbox = new QCheckBox(name, this);
  m_valLabel = new QLabel(this);
  m_valLabel->setAlignment(Qt::AlignRight);

  m_hlayout->addWidget(m_checkbox);
  m_hlayout->addWidget(m_valLabel);
}

void ParamWidgetGroup::setValue(int val)
{
  if (m_enumVals.contains(val))
  {
    m_valLabel->setText(m_enumVals.value(val));
  }
  else
  {
    m_valLabel->setText(QString("%1 %2").arg(val).arg(m_units));
  }
}

void ParamWidgetGroup::clearValue()
{
  m_valLabel->setText("--- " + m_units);
}

unsigned int ParamWidgetGroup::location() const
{
  return m_location;
}

unsigned int ParamWidgetGroup::snapshotPage() const
{
  return m_snapshotPage;
}

bool ParamWidgetGroup::isChecked() const
{
  return m_checkbox->isChecked();
}

void ParamWidgetGroup::setChecked(bool checked)
{
  m_checkbox->setChecked(checked);
}

