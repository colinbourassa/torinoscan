#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <QString>
#include <QMap>
#include <iceblock/BlockExchangeProtocol.h>
#include "paramtype.h"
#include "protocoltype.h"

using json = nlohmann::json;

struct ParamDefinition
{
  ParamType type = ParamType::Unspecified;
  QString name;
  QString units;
  MemoryType memType = MemoryType::Unspecified;
  QMap<int,QString> enumVals;
  unsigned int address = 0;
  unsigned int id = 0;
  unsigned int snapshot = 0;
  unsigned int numByte = 0;
  float lsb = 1.0f;
  float offset = 0.0f;
};

class ECUConfig
{
public:
  explicit ECUConfig(const std::string schemaFilePath);

  bool loadFile(const std::string& jsonFile);
  bool isValid();
  bool getCarAndECUNames(std::string& carName,
                         std::string& ecuName);
  bool getProtocolInfo(uint8_t& ecuAddr,
                       ProtocolType& type,
                       int& baud,
                       LineType& initLine,
                       std::string& variant);
  std::vector<std::pair<std::string,unsigned int>> getActuators();
  std::vector<ParamDefinition> getParameterDefs();

private:
  json m_currentJSON;
  std::string m_schemaFilePath;

  QMap<int,QString> getMapOfEnumVals(const json& node);
};

