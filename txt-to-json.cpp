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

enum class Position
{
  LeftBank, // also used for single ECUs
  RightBank
};

Position getPosition(const std::string& s)
{
  return ((s.size() > 1) && (s.at(1) == 'D')) ? Position::RightBank : Position::LeftBank;
}

json* getJSONArrayAtPos(std::map<Position,json>& jArrs, Position pos)
{
  if (jArrs.count(pos) == 0)
  {
    std::cout << "Creating json::array for position: " << ((pos == Position::LeftBank) ? "left/single" : "right") << std::endl;
    jArrs.emplace(pos, json::array());
  }
  return &jArrs[pos];
}

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

  const std::regex paramSectionPattern("\\\[(P([DS]X)?)\\d+\\]");
  const std::regex errorSectionPattern("\\\[(E([DS]X)?)\\d+\\]");
  const std::regex actuatorSectionPattern("\\\[(D([DS]X)?)\\d+\\]");
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
  std::map<Position,json> ecuJsons;

  if (infile.is_open())
  {
    std::map<Position,json> paramArrays;
    std::map<Position,json> faultCodeArrays;
    std::map<Position,json> actuatorArrays;
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
        const Position pos = getPosition(matches[1]);
        std::cout << "Found parameter section '" << matches[0] << "'" << std::endl;
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
        curArray = getJSONArrayAtPos(paramArrays, pos);
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        const Position pos = getPosition(matches[1]);
        std::cout << "Found error/fault code section '" << matches[0] << "'" << std::endl;
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
        curArray = getJSONArrayAtPos(faultCodeArrays, pos);
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        const Position pos = getPosition(matches[1]);
        std::cout << "Found actuator section '" << matches[0] << "'" << std::endl;
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
        curArray = getJSONArrayAtPos(actuatorArrays, pos);
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
            const int nextIndex = curArray->size();
            std::cout << "Setting name for parameter at index " << nextIndex << " to " << matches[1] << std::endl;
            (*curArray)[nextIndex]["name"] = matches[1];
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
          const int lastIndex = curArray->size() - 1;
          if (lastIndex >= 0)
          {
            (*curArray)[lastIndex]["units"] = matches[1];
          }
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
          const int lastIndex = curArray->size() - 1;
          if (lastIndex >= 0)
          {
            (*curArray)[indexNum]["decimals"] = matches[1];
          }
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
          const int lastIndex = curArray->size() - 1;
          if (lastIndex >= 0)
          {
            (*curArray)[indexNum]["address"] = matches[1];
          }
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
          const int lastIndex = curArray->size() - 1;
          if (lastIndex >= 0)
          {
            (*curArray)[indexNum]["enum"][matches[1]] = matches[2];
          }
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
    //for (json paramArray : paramArrays)
    for (auto& [pos, paramArray] : paramArrays)
    {
      std::cout << "Setting default fields in parameter array of size " << paramArray.size() << "..." << std::endl;
      for (int i = 0; i < paramArray.size(); i++)
      {
        std::cout << "Setting numbytes field for parameter at index " << i << "..." << std::endl;
        paramArray[i]["numbytes"] = 1;
        if (paramArray[i].count("enum") == 0)
        {
          paramArray[i]["lsb"] = 0;
          paramArray[i]["zero"] = 0;
        }
      }
    }

    ecuJsons[Position::LeftBank]["vehicle"] = "";
    ecuJsons[Position::LeftBank]["ecu"] = "";
    ecuJsons[Position::LeftBank]["position"] = ""; // e.g. right or left bank
    ecuJsons[Position::LeftBank]["protocol"] = {
      { "family", "" },
      { "variant", "" },
      { "baud", 0 },
      { "address", "" },
    };
    // Populate the parent JSON object with the three sub-arrays
    ecuJsons[Position::LeftBank]["parameters"] = paramArrays[Position::LeftBank];
    ecuJsons[Position::LeftBank]["faultcodes"] = faultCodeArrays[Position::LeftBank];
    ecuJsons[Position::LeftBank]["actuators"] = actuatorArrays[Position::LeftBank];

    if ((paramArrays.size() > 1) ||
        (faultCodeArrays.size() > 1) ||
        (actuatorArrays.size() > 1))
    {
      std::cout << "At least one of the parameter, fault code, or actuator arrays has more than one entry, " <<
       "suggesting a left/right engine bank arrangement." << std::endl;
      // If we have a right bank, then we know that we can label the first ECU as "left".
      ecuJsons[Position::LeftBank]["position"] = "left";

      ecuJsons[Position::RightBank]["vehicle"] = "";
      ecuJsons[Position::RightBank]["ecu"] = "";
      ecuJsons[Position::RightBank]["position"] = "right"; // e.g. right or left bank
      ecuJsons[Position::RightBank]["protocol"] = {
        { "family", "" },
        { "variant", "" },
        { "baud", 0 },
        { "address", "" },
      };

      if (paramArrays.count(Position::RightBank))
      {
        std::cout << "Adding right bank parameters array to JSON for right bank ECU..." << std::endl;
        ecuJsons[Position::RightBank]["parameters"] = paramArrays[Position::RightBank];
      }
      if (faultCodeArrays.count(Position::RightBank))
      {
        std::cout << "Adding right bank fault code array to JSON for right bank ECU..." << std::endl;
        ecuJsons[Position::RightBank]["faultcodes"] = faultCodeArrays[Position::RightBank];
      }
      if (actuatorArrays.count(Position::RightBank))
      {
        std::cout << "Adding right bank actuator array to JSON for right bank ECU..." << std::endl;
        ecuJsons[Position::RightBank]["actuators"] = actuatorArrays[Position::RightBank];
      }
    }

    std::cout << "Preparing to write output file..." << std::endl;

    // Determine output filename and write the output
    size_t lastIndex = infilename.find_last_of(".");
    for (auto const& [pos, j] : ecuJsons)
    {
      const std::string ecuPosDescriptor = std::string("-") + j["position"].get<std::string>();
      const std::string outfilename = (lastIndex == std::string::npos) ?
        (infilename + ecuPosDescriptor + ".json") : (infilename.substr(0, lastIndex) + ecuPosDescriptor + ".json");
      std::cout << "Writing output file '" << outfilename << "'... ";
      std::ofstream outfile(outfilename);
      outfile << std::setw(2) << j << std::endl;
      std::cout << "done." << std::endl;
    }
    std::cout << "done." << std::endl;
  }

  return 0;
}

