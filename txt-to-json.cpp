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

  // Replace the CP1252 symbols with a short description "deg" so that the JSON
  // processor doesn't complain. Note that there may be other examples of >0x80
  // chars that we will need to address.
  result = std::regex_replace(result, std::regex("\xB0"), "deg");
  result = std::regex_replace(result, std::regex("\x85"), "...");
  return result;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <txtfile>" << std::endl;
    return 0;
  }

  // TODO: Some of the SD2 TXT files have section titles in the
  // format "[S#]", and these contain fields similar to those
  // found in the parameter ("[P#]") sections. We need to figure
  // out how these sections are processed by SD2. See DMul0092.
  const std::regex paramSectionPattern("^\\s*\\\[(P([DS]X)?)\\d+\\]");
  const std::regex errorSectionPattern("^\\s*\\\[(E([DS]X)?)\\d+\\]");
  const std::regex actuatorSectionPattern("^\\s*\\\[(D([DS]X)?)\\d+\\]");
  const std::regex genericSectionPattern("\\\[.*\\]");
  const std::regex namePattern("^\\s*NOME_ING=(.*)");
  const std::regex decimalsPattern("^\\s*DECIMALI=(.*)");
  const std::regex unitsPattern("^\\s*UNITI_DI_MISURA_1=(.*)");
  const std::regex generalPattern("^\\s*GENERAL=(.*)");
  const std::regex enumPattern("^\\s*POS_ING_(\\d+)=(.*)");

  std::string line;
  const std::string infilename(argv[1]);
  std::cout << "Processing file '" << infilename << "'..." << std::endl;
  std::ifstream infile(infilename);

  std::smatch matches;

  if (infile.is_open())
  {
    bool foundRightBank = false;
    std::map<int,json> ecuJsons;
    json* curArray = nullptr;
    bool excludeCurrent = false;

    for (int index = 0; index <= 1; index++)
    {
      ecuJsons[index]["vehicle"] = "";
      ecuJsons[index]["ecu"] = "";
      ecuJsons[index]["position"] = "";
      ecuJsons[index]["protocol"] = { { "family", "" }, { "variant", "" }, { "baud", 0 }, { "address", "" } };
      ecuJsons[index]["parameters"] = json::array();
      ecuJsons[index]["faultcodes"] = json::array();
      ecuJsons[index]["actuators"] = json::array();
    }

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
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = &ecuJsons[pos]["parameters"];
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = &ecuJsons[pos]["faultcodes"];
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = &ecuJsons[pos]["actuators"];
      }
      else if (std::regex_search(line, matches, genericSectionPattern))
      {
        std::cout << "Found unknown section ('" << matches[0] << "'), skipping." << std::endl;
        curArray = nullptr;
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
            std::cerr << "Matched name line (" << line << ") outside of known section, ignoring..." << std::endl;
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
          std::cerr << "Matched units line (" << line << ") outside of known section, ignoring..." << std::endl;
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
          std::cerr << "Matched decimals line (" << line << ") outside of known section, ignoring..." << std::endl;
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
          std::cerr << "Matched general/address line (" << line << ") outside of known section, ignoring..." << std::endl;
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
          std::cerr << "Matched enum line (" << line << ") outside of known section, ignoring..." << std::endl;
        }
      }
    }
    std::cout << "Reached end of input file." << std::endl;
    infile.close();

    // Set some defaults for fields whose values we will eventually need,
    // but that are not stored in the original .txt file.
    for (int ecuIndex = 0; ecuIndex <= 1; ecuIndex++)
    {
      for (int paramIndex = 0; paramIndex < ecuJsons[ecuIndex]["parameters"].size(); paramIndex++)
      {
        ecuJsons[ecuIndex]["parameters"][paramIndex]["numbytes"] = 1;
        if (ecuJsons[ecuIndex]["parameters"][paramIndex].count("enum") == 0)
        {
          ecuJsons[ecuIndex]["parameters"][paramIndex]["lsb"] = 0;
          ecuJsons[ecuIndex]["parameters"][paramIndex]["zero"] = 0;
        }
      }
    }

    if (foundRightBank)
    {
      ecuJsons[0]["position"] = "left";
      ecuJsons[1]["position"] = "right";
      std::cout << "Found at least one parameter, fault code, actuator entry marked for the right bank ECU." << std::endl;

      const std::string outFilename = getOutputFilename(infilename, ecuJsons[1]["position"].get<std::string>());
      std::cout << "Writing output file '" << outFilename << "'... ";
      std::ofstream outfile(outFilename);
      outfile << std::setw(2) << ecuJsons[1] << std::endl;
      std::cout << "done." << std::endl;
    }

    const std::string outFilename = getOutputFilename(infilename, ecuJsons[0]["position"].get<std::string>());
    std::cout << "Writing output file '" << outFilename << "'... ";
    std::ofstream outfile(outFilename);
    outfile << std::setw(2) << ecuJsons[0] << std::endl;
    std::cout << "done." << std::endl;
  }

  return 0;
}

