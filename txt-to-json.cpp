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

std::string trim(const std::string& line)
{
  const char* ws = " \t\v\r\n";
  std::size_t start = line.find_first_not_of(ws);
  std::size_t end = line.find_last_not_of(ws);
  return start == end ? std::string() : line.substr(start, end - start + 1);
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <txtfile>" << std::endl;
    return 0;
  }

  ProcMode mode = ProcMode::None;
  int indexNum = -1;

  // TODO: The section names can start with just a single letter (P, E, or D)
  // when the ECU manages the entire engine, or with a DX/SX suffix when the
  // section pertains to the right (destra) or left (sinistra) bank.
  //const std::regex paramSectionPattern("\\\[P(SX)?\\d+\\]");

  const std::regex paramSectionPattern("\\\[P\\d+\\]");
  const std::regex errorSectionPattern("\\\[E\\d+\\]");
  const std::regex actuatorSectionPattern("\\\[D\\d+\\]");
  const std::regex namePattern("NOME_ING=(.*)");
  const std::regex decimalsPattern("DECIMALI=(.*)");
  const std::regex unitsPattern("UNITI_DI_MISURA_1=(.*)");
  const std::regex generalPattern("GENERAL=(.*)");
  const std::regex enumPattern("POS_ING_(\\d+)=(.*)");

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
      line = trim(line);
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
          if (curArray)
          {
            (*curArray)[indexNum]["name"] = matches[1];
          }
          else
          {
            std::cerr << "ERROR: Found name line (" << line << ") without previous section header!" << std::endl;
          }
        }
      }
      else if (std::regex_search(line, matches, unitsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArray)
        {
          (*curArray)[indexNum]["units"] = matches[1];
        }
        else
        {
          std::cerr << "ERROR: Found units line (" << line << ") without previous section header!" << std::endl;
        }
      }
      else if (std::regex_search(line, matches, decimalsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArray)
        {
          (*curArray)[indexNum]["decimals"] = matches[1];
        }
        else
        {
          std::cerr << "ERROR: Found decimals line (" << line << ") without previous section header!" << std::endl;
        }
      }
      else if (std::regex_search(line, matches, generalPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArray)
        {
          (*curArray)[indexNum]["address"] = matches[1];
        }
        else
        {
          std::cerr << "ERROR: Found general/address line (" << line << ") without previous section header!" << std::endl;
        }
      }
      else if (std::regex_search(line, matches, enumPattern) && (matches.size() >= 3) && !excludeCurrent)
      {
        if (curArray)
        {
          (*curArray)[indexNum]["enum"][matches[1]] = matches[2];
        }
        else
        {
          std::cerr << "ERROR: Found enum line (" << line << ") without previous section header!" << std::endl;
        }
      }
    }
    std::cout << "Reached end of file." << std::endl;
    infile.close();

    // Set some defaults for fields whose values we will eventually need,
    // but that are not stored in the original .txt file.
    for (int i = 0; i < paramArray.size(); i++)
    {
      paramArray[i]["numbytes"] = 1;
      if (paramArray[i].count("enum") == 0)
      {
        paramArray[i]["lsb"] = 0;
        paramArray[i]["zero"] = 0;
      }
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

