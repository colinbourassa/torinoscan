#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <regex>
using json = nlohmann::ordered_json;

/**
 * Describes the known types of sections from the input file.
 */
enum class ProcMode
{
  None,
  Param,
  FaultCode,
  Actuator
};

/**
 * Determines the ECU position based on a substring regex match from a section title.
 * This function takes the alpha-character portion of the section name, e.g. "P" from "P23"
 * or "PDX" from "PDX0".
 */
int getPosition(const std::string& s)
{
  // Interpret a section name with "D" as the second character as representing
  // the "destra" (right) bank ECU. We don't bother with explicit checking for
  // "S" ("sinistra") as both left-bank ECUs and single ECUs are treated the same.
  return ((s.size() > 1) && (s.at(1) == 'D')) ? 1 : 0;
}

/**
 * Retrieves the JSON array object for the specified position from the provided
 * map, creating the array object first if necessary.
 */
json* getJSONArrayAtPos(std::map<int,json>& jArrs, int pos)
{
  if (jArrs.count(pos) == 0)
  {
    std::cout << "Creating json::array for position: " << ((pos == 0) ? "left/single" : "right") << std::endl;
    jArrs.emplace(pos, json::array());
  }
  return &jArrs[pos];
}

/**
 * Forms an appropriate outfile filename by changing the extension on the input
 * filename to .json, and also adding an ECU position indicator (e.g. '-left' or
 * '-right') if necessary.
 */
std::string getOutputFilename(const std::string& inputFilename, const std::string& positionDesc)
{
  const size_t lastDotIndex = inputFilename.find_last_of(".");
  std::string outFilename =
    (lastDotIndex == std::string::npos) ? inputFilename : inputFilename.substr(0, lastDotIndex);
  outFilename += (positionDesc.empty() ? ".json" : ("-" + positionDesc + ".json"));
  return outFilename;
}

/**
 * Trims whitespace from both the start and end of the provided string.
 */
std::string trim(const std::string& line)
{
  std::string result;
  const char* ws = " \t\v\r\n";
  std::size_t start = line.find_first_not_of(ws);
  std::size_t end = line.find_last_not_of(ws);
  if (start != end)
  {
    result = line.substr(start, end - start + 1);
  }

  // Replace the degree symbol (extended ASCII 0xB0) with "deg" so that the JSON
  // processor doesn't complain. Note that there may be other examples of >0x80
  // chars that we will need to address.
  result = std::regex_replace(result, std::regex("\xB0"), "deg");
  return result;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <txtfile>" << std::endl;
    return 0;
  }

  ProcMode mode = ProcMode::None;

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
  std::map<int,json> ecuJsons;

  if (infile.is_open())
  {
    // TODO: Determine whether we want to populate index 0 in each of these
    // maps with an empty JSON array (so that the JSON output file will always
    // have param, fault code, and actuator sections -- even if one or more of
    // those sections is empty), OR if we want to do a check later and only
    // add the array to the JSON parent object if the array is non-null.
    std::map<int,json> paramArrays;
    std::map<int,json> faultCodeArrays;
    std::map<int,json> actuatorArrays;
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
        const int pos = getPosition(matches[1]);
        excludeCurrent = false;
        mode = ProcMode::Param;
        curArray = getJSONArrayAtPos(paramArrays, pos);
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
        excludeCurrent = false;
        mode = ProcMode::FaultCode;
        curArray = getJSONArrayAtPos(faultCodeArrays, pos);
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
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
            (*curArray)[lastIndex]["decimals"] = matches[1];
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
            (*curArray)[lastIndex]["address"] = matches[1];
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
            (*curArray)[lastIndex]["enum"][matches[1]] = matches[2];
          }
        }
        else
        {
          std::cerr << "ERROR: Found enum line (" << line << ") without previous section header!" << std::endl;
        }
      }
    }
    std::cout << "Reached end of input file." << std::endl;
    infile.close();

    // Set some defaults for fields whose values we will eventually need,
    // but that are not stored in the original .txt file.
    for (auto& [pos, paramArray] : paramArrays)
    {
      std::cout << "Setting default fields in parameter array (" << paramArray.size() << " elements)..." << std::endl;
      for (int i = 0; i < paramArray.size(); i++)
      {
        paramArray[i]["numbytes"] = 1;
        if (paramArray[i].count("enum") == 0)
        {
          paramArray[i]["lsb"] = 0;
          paramArray[i]["zero"] = 0;
        }
      }
    }

    ecuJsons[0]["vehicle"] = "";
    ecuJsons[0]["ecu"] = "";
    ecuJsons[0]["position"] = ""; // e.g. left or right bank - this is only populated later
                                  // if we need to discriminate between more than one
    ecuJsons[0]["protocol"] = { { "family", "" }, { "variant", "" }, { "baud", 0 }, { "address", "" } };

    // Populate the parent JSON object with the three sub-arrays
    ecuJsons[0]["parameters"] = paramArrays[0];
    ecuJsons[0]["faultcodes"] = faultCodeArrays[0];
    ecuJsons[0]["actuators"] = actuatorArrays[0];

    if ((paramArrays.size() > 1) ||
        (faultCodeArrays.size() > 1) ||
        (actuatorArrays.size() > 1))
    {
      std::cout << "At least one of the parameter, fault code, or actuator arrays has more than one entry, " <<
       "suggesting a left/right engine bank arrangement." << std::endl;

      // If we have a right bank, then we know that we can label the first ECU as "left".
      ecuJsons[0]["position"] = "left";

      ecuJsons[1]["vehicle"] = "";
      ecuJsons[1]["ecu"] = "";
      ecuJsons[1]["position"] = "right"; // e.g. right or left bank
      ecuJsons[1]["protocol"] = { { "family", "" }, { "variant", "" }, { "baud", 0 }, { "address", "" } };

      if (paramArrays.count(1))
      {
        ecuJsons[1]["parameters"] = paramArrays[1];
      }
      if (faultCodeArrays.count(1))
      {
        ecuJsons[1]["faultcodes"] = faultCodeArrays[1];
      }
      if (actuatorArrays.count(1))
      {
        ecuJsons[1]["actuators"] = actuatorArrays[1];
      }
    }

    for (auto const& [pos, j] : ecuJsons)
    {
      const std::string outFilename = getOutputFilename(infilename, j["position"].get<std::string>());
      std::cout << "Writing output file '" << outFilename << "'... ";
      std::ofstream outfile(outFilename);
      outfile << std::setw(2) << j << std::endl;
      std::cout << "done." << std::endl;
    }
  }

  return 0;
}

