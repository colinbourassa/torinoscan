#include "actuatorwidgetgroup.h"

ActuatorWidgetGroup::ActuatorWidgetGroup(const QString& name,
                                         unsigned int address,
                                         QWidget* parent) :
  QWidget(parent),
  m_address(address)
{
  m_hlayout = new QHBoxLayout(this);
  m_button = new QPushButton("-->", this);
  m_label = new QLabel(name, this);

  m_hlayout->setAlignment(Qt::AlignLeft);
  m_hlayout->addWidget(m_button);
  m_hlayout->addWidget(m_label);

  connect(m_button, &QPushButton::clicked, this, &ActuatorWidgetGroup::onButtonClicked);
}

void ActuatorWidgetGroup::onButtonClicked()
{
  emit buttonClicked(m_address);
}
