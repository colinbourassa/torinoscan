#include <QMap>
#include <fstream>
#include "ecuconfig.h"
#include "paramtype.h"
#include "utils.h"

ECUConfig::ECUConfig(const std::string schemaFilePath) :
  m_schemaFilePath(schemaFilePath)
{
}

bool ECUConfig::loadFile(const std::string& jsonFile)
{
  bool status = true;
  try
  {
    std::ifstream jsonFstream(jsonFile);
    m_currentJSON = json::parse(jsonFstream);
  }
  catch (...)
  {
    status = false;
  }
  return status;
}

bool ECUConfig::isValid()
{
  // TODO
  return false;
}

bool ECUConfig::getCarAndECUNames(std::string& carName,
                                  std::string& ecuName)
{
  bool status = true;
  try
  {
    carName = m_currentJSON.at("vehicle");
    ecuName = m_currentJSON.at("ecu");
    if (m_currentJSON.contains("position"))
    {
      ecuName += " (" + m_currentJSON.at("position").get<std::string>() + ")";
    }
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }

  return status;
}

bool ECUConfig::getProtocolInfo(uint8_t& ecuAddr,
                                ProtocolType& type,
                                int& baud,
                                LineType& initLine,
                                std::string& variant)
{
  bool status = true;
  try
  {
    const json& protocolNode = m_currentJSON.at("protocol");
    ecuAddr = std::stoul(protocolNode.at("address").get<std::string>(), nullptr, 0);
    baud = protocolNode.at("baud");
    type = protocolTypeFromString(protocolNode.at("family"));
    variant = protocolNode.at("variant");
    status = (type != ProtocolType::None);
  }
  catch (const json::out_of_range& e)
  {
    status = false;
  }

  return status;
}

std::vector<std::pair<std::string,unsigned int>> ECUConfig::getActuators()
{
  std::vector<std::pair<std::string,unsigned int>> actuators;

  if (m_currentJSON.contains("actuators") && m_currentJSON.at("actuators").is_array())
  {
    for (const auto& actEntry : m_currentJSON.at("actuators"))
    {
      if (actEntry.contains("name") && actEntry.contains("id"))
      {
        actuators.push_back(std::make_pair(actEntry.at("name").get<std::string>(),
                                           actEntry.at("id").get<unsigned int>()));
      }
    }
  }

  return actuators;
}

QMap<int,QString> ECUConfig::getMapOfEnumVals(const json& node)
{
  QMap<int,QString> enumVals;
  if (node.contains("enumvals"))
  {
   const std::map<std::string,std::string> enumValsStd = node.at("enumvals");
   for (auto const& [key, value] : enumValsStd)
   {
     enumVals.insert(std::stoi(key), QString::fromStdString(value));
   }
  }
  return enumVals;
}

std::vector<ParamDefinition> ECUConfig::getParameterDefs()
{
  std::vector<ParamDefinition> params;

  try
  {
    for (const auto node : m_currentJSON.at("parameters"))
    {
      // First, populate the fields that are common across the different memory parameter types
      ParamDefinition p;
      p.name = QString::fromStdString(node.at("name"));
      p.units = node.contains("units") ? QString::fromStdString(node.at("units")) : QString();
      p.enumVals = getMapOfEnumVals(node);
      p.lsb = node.contains("lsb") ? node.at("lsb").get<float>() : 1.0f;
      p.offset = node.contains("offset") ? node.at("offset").get<float>() : 0.0f;

      // Check first for the parameter fields required for a raw memory address param,
      // then for a "stored value" param (accessed via an ID rather than an address),
      // and finally for a snapshot param. Use the first one for which all the
      // appropriate fields are available.

      if (node.contains("address") && node.contains("numbytes"))
      {
        p.type = ParamType::MemoryAddress;
        p.address = node.at("address");
        p.numByte = node.at("numbytes");
        params.push_back(p);
      }
      else if (node.contains("id"))
      {
        p.type = ParamType::StoredValue;
        p.id = node.at("id");
        params.push_back(p);
      }
      else if (node.contains("snapshot") && node.contains("address") && node.contains("numbytes"))
      {
        p.type = ParamType::SnapshotLocation;
        p.snapshot = node.at("snapshot");
        p.address = node.at("address");
        p.numByte = node.at("numbytes");
        params.push_back(p);
      }
    }
  }
  catch(...)
  {
  }

  return params;
}

