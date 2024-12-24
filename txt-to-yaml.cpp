#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_set>
#include <regex>

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
    bool foundRightBank = false;
    std::map<int,YAML::Node> ecuYamls;
    YAML::Node curArray;
    bool curArrayActive = false;
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
      //ecuYamls[index]["protocol"] = { { "family", "" }, { "variant", "" }, { "baud", 0 }, { "address", "" } };
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
        const int pos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = ecuYamls[pos]["parameters"];
        curArrayActive = true;
      }
      else if (std::regex_search(line, matches, errorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = ecuYamls[pos]["faultcodes"];
        curArrayActive = true;
      }
      else if (std::regex_search(line, matches, actuatorSectionPattern))
      {
        const int pos = getPosition(matches[1]);
        foundRightBank = (foundRightBank || (pos == 1));
        excludeCurrent = false;
        curArray = ecuYamls[pos]["actuators"];
        curArrayActive = true;
      }
      else if (std::regex_search(line, matches, genericSectionPattern))
      {
        std::cout << "Found unknown section ('" << matches[0] << "'), skipping." << std::endl;
        curArrayActive = false;
      }
      else if (std::regex_search(line, matches, namePattern) && (matches.size() >= 2))
      {
        if (excludeNames.count(matches[1]))
        {
          excludeCurrent = true;
        }
        else
        {
          if (curArrayActive)
          {
            const int nextIndex = curArray.size();
            curArray[nextIndex]["name"] = matches[1];
          }
          else
          {
            std::cerr << "Matched name line (" << line << ") outside of known section, ignoring..." << std::endl;
          }
        }
      }
      else if (std::regex_search(line, matches, unitsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArrayActive)
        {
          const int lastIndex = curArray.size() - 1;
          if (lastIndex >= 0)
          {
            curArray[lastIndex]["units"] = matches[1];
          }
        }
        else
        {
          std::cerr << "Matched units line (" << line << ") outside of known section, ignoring..." << std::endl;
        }
      }
      else if (std::regex_search(line, matches, decimalsPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArrayActive)
        {
          const int lastIndex = curArray.size() - 1;
          if (lastIndex >= 0)
          {
            try
            {
              curArray[lastIndex]["decimals"] = std::stoi(matches[1]);
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
        }
        else
        {
          std::cerr << "Matched decimals line (" << line << ") outside of known section, ignoring..." << std::endl;
        }
      }
      else if (std::regex_search(line, matches, generalPattern) && (matches.size() >= 2) && !excludeCurrent)
      {
        if (curArrayActive)
        {
          const int lastIndex = curArray.size() - 1;
          if (lastIndex >= 0)
          {
            curArray[lastIndex]["address"] = matches[1];
          }
        }
        else
        {
          std::cerr << "Matched general/address line (" << line << ") outside of known section, ignoring..." << std::endl;
        }
      }
      else if (std::regex_search(line, matches, enumPattern) && (matches.size() >= 3) && !excludeCurrent)
      {
        if (curArrayActive)
        {
          const int lastIndex = curArray.size() - 1;
          if (lastIndex >= 0)
          {
            curArray[lastIndex]["enum"][matches[1]] = matches[2];
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
      for (int paramIndex = 0; paramIndex < ecuYamls[ecuIndex]["parameters"].size(); paramIndex++)
      {
        ecuYamls[ecuIndex]["parameters"][paramIndex]["numbytes"] = 1;
        if (!ecuYamls[ecuIndex]["parameters"][paramIndex]["enum"])
        {
          ecuYamls[ecuIndex]["parameters"][paramIndex]["lsb"] = 0;
          ecuYamls[ecuIndex]["parameters"][paramIndex]["zero"] = 0;
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

