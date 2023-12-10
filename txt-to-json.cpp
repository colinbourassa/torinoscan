//#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <regex>

enum class ProcMode
{
  None,
  Param,
  FaultCode,
  Actuator
};

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <txtfile>" << std::endl;
    return 0;
  }

  ProcMode mode = ProcMode::None;
  int indexNum = -1;

  std::regex paramSectionPattern("\\\[P\\d+\\]");
  std::regex errorSectionPattern("\\\[E\\d+\\]");
  std::regex actuatorSectionPattern("\\\[D\\d+\\]");
  std::regex namePattern("NOME_ING=(.*)");
  std::regex decimalsPattern("DECIMALI=(.*)");
  std::regex unitsPattern("UNITI_DI_MISURA_1=(.*)");
  std::regex generalPattern("GENERAL=(.*)");

  std::string line;
  std::cout << "Opening file '" << argv[1] << "'..." << std::endl;
  std::ifstream infile(argv[1]);

  std::smatch matches;

  if (infile.is_open())
  {
    while (std::getline(infile, line))
    {
      if (std::regex_search(line, matches, paramSectionPattern))
      {
        indexNum = (mode == ProcMode::Param) ? (indexNum + 1) : 0;
        mode = ProcMode::Param;
        std::cout << "param " << indexNum << std::endl;
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        indexNum = (mode == ProcMode::FaultCode) ? (indexNum + 1) : 0;
        mode = ProcMode::FaultCode;
        std::cout << "fault code " << indexNum << std::endl;
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        indexNum = (mode == ProcMode::Actuator) ? (indexNum + 1) : 0;
        mode = ProcMode::Actuator;
        std::cout << "actuator " << indexNum << std::endl;
      }
      else if (std::regex_search(line, matches, namePattern) && (matches.size() >= 2))
      {
        // name in matches[1]
      }
      else if (std::regex_search(line, matches, unitsPattern) && (matches.size() >= 2))
      {
        // units in matches[1]
      }
      else if (std::regex_search(line, matches, decimalsPattern) && (matches.size() >= 2))
      {
        // decimals in matches[1]
      }
      else if (std::regex_search(line, matches, generalPattern) && (matches.size() >= 2))
      {
        // units in matches[1]
      }
    }
    std::cout << "Reached end of file." << std::endl;
    infile.close();
  }
  else
  {
    std::cout << "File not open." << std::endl;
  }

  return 0;
}

