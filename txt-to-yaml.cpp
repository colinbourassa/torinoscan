#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <regex>

/**
 * Describes the known types of sections from the input file.
 */
enum class SectionType
{
  None,
  Param,
  FaultCode,
  Actuator
};

/**
 * Returns the string used to identify one of the three main sections in the YAML data structure.
 */
std::string getSectionName(SectionType section)
{
  std::string name;
  if (section == SectionType::Param)
  {
    name = "parameters";
  }
  else if (section == SectionType::FaultCode)
  {
    name = "faultcodes";
  }
  else if (section == SectionType::Actuator)
  {
    name = "actuators";
  }
  return name;
}

/**
 * Adds a field (with a numeric/integer value) to the YAML data.
 */
bool addYAMLField(YAML::Node parentNode, SectionType section, bool createNewIndex, const std::string& fieldName, int fieldValue)
{
  bool status = false;
  std::string sectionName = getSectionName(section);

  if (!sectionName.empty())
  {
    const int index = parentNode[sectionName].size();
    if (createNewIndex)
    {
      parentNode[sectionName][index][fieldName] = fieldValue;
      status = true;
    }
    else if (index > 0)
    {
      parentNode[sectionName][index - 1][fieldName] = fieldValue;
      status = true;
    }
  }

  return status;
}

/**
 * Adds a field (with a string value) to the YAML data.
 */
bool addYAMLField(YAML::Node parentNode, SectionType section, bool createNewIndex, const std::string& fieldName, const std::string& fieldValue)
{
  bool status = false;
  std::string sectionName = getSectionName(section);

  if (!sectionName.empty())
  {
    const int index = parentNode[sectionName].size();
    if (createNewIndex)
    {
      parentNode[sectionName][index][fieldName] = fieldValue;
      status = true;
    }
    else if (index > 0)
    {
      parentNode[sectionName][index - 1][fieldName] = fieldValue;
      status = true;
    }
  }

  return status;
}

/**
 * Adds an enum value to the last of the existing nodes in the specified sequence node.
 */
bool addYAMLEnumValue(YAML::Node parentNode, SectionType section, const std::string& enumVal, const std::string& enumStr)
{
  bool status = false;
  const std::string sectionName = getSectionName(section);

  if (!sectionName.empty())
  {
    const int index = parentNode[sectionName].size();
    if (index > 0)
    {
      parentNode[sectionName][index - 1]["enum"][enumVal] = enumStr;
      status = true;
    }
  }

  return status;
}

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
 * filename to .yaml, and also adding an ECU position indicator (e.g. '-left' or
 * '-right') if necessary.
 */
std::string getOutputFilename(const std::string& inputFilename, const std::string& positionDesc)
{
  const size_t lastDotIndex = inputFilename.find_last_of(".");
  std::string outFilename =
    (lastDotIndex == std::string::npos) ? inputFilename : inputFilename.substr(0, lastDotIndex);
  outFilename += (positionDesc.empty() ? ".yaml" : ("-" + positionDesc + ".yaml"));
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

  // NOTE: Some of the SD2 TXT files have section titles in the format "[S##]"
  // or "[ST##]", and these are for "statistical parameters". This is just a
  // category of parameters that are displayed in a different window by WSDC32,
  // and they are not necessarily handled in a fundamentally different way.
  // However, the Marelli 1AF protocol provides a "read snapshot" feature that
  // returns a cohesive block of data, and the Marelli Cambio F1 TCU (MCAM0151)
  // puts this snapshot data in the "statistical parameters" category. That
  // category also includes data read using the "read value" 1AF protocol command.

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
    int curECUPos = 0;
    bool foundRightBank = false;
    std::map<int,YAML::Node> ecuYamls;
    SectionType curSection = SectionType::None;
    bool excludeCurrent = false;

    std::cout << "Setting up defaults..." << std::endl;
    for (int index = 0; index <= 1; index++)
    {
      ecuYamls[index]["vehicle"] = "";
      ecuYamls[index]["ecu"] = "";
      ecuYamls[index]["position"] = "";
      ecuYamls[index]["protocol"].push_back("family");
      ecuYamls[index]["protocol"].push_back("variant");
      ecuYamls[index]["protocol"].push_back("baud");
      ecuYamls[index]["protocol"].push_back("address");
      ecuYamls[index]["parameters"] = { };
      ecuYamls[index]["faultcodes"] = { };
      ecuYamls[index]["actuators"] = { };
    }

    const std::unordered_set<std::string> excludeNames =
    {
      "A/D CONVERTER SD2",
      "SELF-LEARNT PARAMETER CANCELLING",
      "CHECK STATUS"
    };

    std::cout << "Reading input file..." << std::endl;
    while (std::getline(infile, line))
    {
      line = trim(line);
      if (std::regex_search(line, matches, paramSectionPattern))
      {
        curECUPos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (curECUPos == 1));
        excludeCurrent = false;
        curSection = SectionType::Param;
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        curECUPos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (curECUPos == 1));
        excludeCurrent = false;
        curSection = SectionType::FaultCode;
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        curECUPos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (curECUPos == 1));
        excludeCurrent = false;
        curSection = SectionType::Actuator;
      }
      else if (std::regex_search(line, matches, genericSectionPattern))
      {
        std::cout << "Found unknown section ('" << matches[0] << "'), skipping." << std::endl;
        curSection = SectionType::None;
      }
      else if (std::regex_search(line, matches, namePattern) && (matches.size() >= 2))
      {
        if (excludeNames.count(matches[1]))
        {
          excludeCurrent = true;
        }
        else
        {
          addYAMLField(ecuYamls[curECUPos], curSection, true, "name", matches.str(1));
        }
      }
      else if (std::regex_search(line, matches, unitsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        addYAMLField(ecuYamls[curECUPos], curSection, false, "units", matches.str(1));
      }
      else if (std::regex_search(line, matches, decimalsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        try
        {
          addYAMLField(ecuYamls[curECUPos], curSection, false, "decimals", std::stoi(matches.str(1)));
        }
        catch (std::invalid_argument const& ex)
        {
          std::cerr << "Failed to parse 'decimals' string (" << line << ")" << std::endl;
        }
        catch (std::out_of_range const& ex)
        {
          std::cerr << "'decimals' string out of range (" << line << ")" << std::endl;
        }
      }
      else if (std::regex_search(line, matches, generalPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        addYAMLField(ecuYamls[curECUPos], curSection, false, "address", matches.str(1));
      }
      else if (std::regex_search(line, matches, enumPattern) && (matches.size() >= 3) && !excludeCurrent)
      {
        // matches.str(1) is the enum value
        // matches.str(2) is the enum string
        addYAMLEnumValue(ecuYamls[curECUPos], curSection, matches.str(1), matches.str(2));
      }
    }
    std::cout << "Reached end of input file." << std::endl;
    infile.close();

    // Set some defaults for fields whose values we will eventually need,
    // but that are not stored in the original .txt file.
    for (int ecuIndex = 0; ecuIndex <= 1; ecuIndex++)
    {
      for (int paramIndex = 0; paramIndex < ecuYamls[ecuIndex]["parameters"].size(); paramIndex++)
      {
        ecuYamls[ecuIndex]["parameters"][paramIndex]["numbytes"] = 1;
        if (!ecuYamls[ecuIndex]["parameters"][paramIndex]["enum"])
        {
          ecuYamls[ecuIndex]["parameters"][paramIndex]["lsb"] = 0;
          ecuYamls[ecuIndex]["parameters"][paramIndex]["offset"] = 0;
        }
      }
    }

    if (foundRightBank)
    {
      ecuYamls[0]["position"] = "left";
      ecuYamls[1]["position"] = "right";
      std::cout << "Found at least one parameter, fault code, actuator entry marked for the right bank ECU." << std::endl;

      const std::string outFilename = getOutputFilename(infilename, ecuYamls[1]["position"].as<std::string>());
      std::cout << "Writing output file '" << outFilename << "'... ";
      std::ofstream outfile(outFilename);
      outfile << ecuYamls[1] << std::endl;
      std::cout << "done." << std::endl;
    }

    const std::string outFilename = getOutputFilename(infilename, ecuYamls[0]["position"].as<std::string>());
    std::cout << "Writing output file '" << outFilename << "'... ";
    std::ofstream outfile(outFilename);
    outfile << ecuYamls[0] << std::endl;
    std::cout << "done." << std::endl;
  }

  return 0;
}

