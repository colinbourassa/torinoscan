#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <regex>
using json = nlohmann::ordered_json;

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
  const std::string infilename(argv[1]);
  std::cout << "Processing file '" << infilename << "'..." << std::endl;
  std::ifstream infile(infilename);

  std::smatch matches;

  if (infile.is_open())
  {
    json j;
    j["vehicle"] = "";
    j["ecu"] = "";
    j["protocol"] = {
      { "family", "" },
      { "variant", "" },
      { "baud", 0 },
      { "address", "" },
    };

    json paramArray = json::array();
    json faultCodeArray = json::array();
    json actuatorArray = json::array();
    json* curArray = nullptr;
    bool excludeCurrent = false;

    const std::unordered_set<std::string> excludeNames =
    {
      "A/D CONVERTER SD2",
      "SELF-LEARNT PARAMETER CANCELLING",
      "CHECK STATUS"
    };

    while (std::getline(infile, line))
    {
      if (std::regex_search(line, matches, paramSectionPattern))
      {
        // Found a section header for a parameter
        if (mode != ProcMode::Param)
        {
          indexNum = 0;
        }
        else if (!excludeCurrent)
        {
          indexNum++;
        }
        excludeCurrent = false;
        mode = ProcMode::Param;
        curArray = &paramArray;
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        // Found a section header for a fault code definition
        if (mode != ProcMode::FaultCode)
        {
          indexNum = 0;
        }
        else if (!excludeCurrent)
        {
          indexNum++;
        }
        excludeCurrent = false;
        mode = ProcMode::FaultCode;
        curArray = &faultCodeArray;
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        // Found a section header for an actuator
        if (mode != ProcMode::Actuator)
        {
          indexNum = 0;
        }
        else if (!excludeCurrent)
        {
          indexNum++;
        }
        excludeCurrent = false;
        mode = ProcMode::Actuator;
        curArray = &actuatorArray;
      }
      else if (std::regex_search(line, matches, namePattern) && (matches.size() >= 2))
      {
        if (excludeNames.count(matches[1]))
        {
          excludeCurrent = true;
        }
        else
        {
          (*curArray)[indexNum]["name"] = matches[1];
        }
      }
      else if (std::regex_search(line, matches, unitsPattern) && (matches.size() >= 2) && excludeCurrent)
      {
        (*curArray)[indexNum]["units"] = matches[1];
      }
      else if (std::regex_search(line, matches, decimalsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        (*curArray)[indexNum]["decimals"] = matches[1];
      }
      else if (std::regex_search(line, matches, generalPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        (*curArray)[indexNum]["address"] = matches[1];
      }
    }
    std::cout << "Reached end of file." << std::endl;
    infile.close();

    // Set some defaults for fields whose values we will eventually need,
    // but that are not stored in the original .txt file.
    for (int i = 0; i < paramArray.size(); i++)
    {
      paramArray[i]["numbytes"] = 1;
      paramArray[i]["lsb"] = 0;
      paramArray[i]["zero"] = 0;
      paramArray[i]["units"] = "";
    }

    // Populate the parent JSON object with the three sub-arrays
    j["parameters"] = paramArray;
    j["faultcodes"] = faultCodeArray;
    j["actuators"] = actuatorArray;

    // Determine output filename and write the output
    const std::string infilename(argv[1]);
    size_t lastIndex = infilename.find_last_of(".");
    const std::string outfilename = (lastIndex == std::string::npos) ?
      (infilename + ".json") : (infilename.substr(0, lastIndex) + ".json");
    std::cout << "Writing output file '" << outfilename << "'... ";
    std::ofstream outfile(outfilename);
    outfile << std::setw(2) << j << std::endl;
    std::cout << "done." << std::endl;
  }

  return 0;
}

