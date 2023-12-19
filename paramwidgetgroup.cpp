#include "paramwidgetgroup.h"

ParamWidgetGroup::ParamWidgetGroup(const QString& name,
                                   unsigned int address,
                                   const QString& units,
                                   const QMap<int,QString>& enumVals,
                                   QWidget* parent) :
  QWidget(parent),
  m_address(address),
  m_enumVals(enumVals),
  m_units(units)
{
  m_hlayout = new QHBoxLayout(this);
  m_checkbox = new QCheckBox(name, this);
  m_valLabel = new QLabel(this);
  m_valLabel->setAlignment(Qt::AlignRight);

  m_hlayout->addWidget(m_checkbox);
  m_hlayout->addWidget(m_valLabel);

  clearValue();
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

unsigned int ParamWidgetGroup::address() const
{
  return m_address;
}

bool ParamWidgetGroup::isChecked() const
{
  return m_checkbox->isChecked();
}

void ParamWidgetGroup::setChecked(bool checked)
{
  m_checkbox->setChecked(checked);
}
