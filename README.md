### TorinoScan

**TorinoScan** is an automotive diagnostic tool that is intended to provide diagnostic capability for the ECUs found in Ferrari vehicles of the 1990s. It is still under development, with the idea that it will eventually be usable as an open-source alternative to the Ferrari/Digitek SD2 factory diagnostic tool.

TorinoScan is designed as a generic interface, and it is configured by YAML files. Each of these configuration files describes the parameters, fault codes, and actuators supported by a single ECU. With the right config file, it should be possible to use this tool to run diagnostics on any ECU that speaks one of the supported protocols; for example, many of the Bosch Motronic units in BMW products from the late 1980s and early 1990s use the supported KWP-71 protocol.

The actual software protocols used to communicate with the ECUs are provided by one or more external libraries. Currently, the only supported library is [libiceblock](https://github.com/colinbourassa/libiceblock), which implements several older "block-exchange" protocols that predate the OBD-2 era. These older protocols include KWP-71, FIAT-9141, and Marelli 1AF.

### Hardware requirements

The libiceblock library requires an FTDI-based USB K-line interface device. An older Ross-Tech cable was tested successfully, and there are also generic cables available on the market (though it is important to get one that has a geuine FTDI chip.)

### Conversion of SD2 data

Because this utility is intended to be a replacement for the Ferrari SD2 system, it is helpful to be able to convert the original SD2 configuration files that contain ECU parameter definitions. These original definition files have a .TXT extension but use the .INI format. They contain definitions for readable parameters, fault codes, and actuators that may be triggered via the diagnostic connection.

Parameters are identified by section headers like "[P1]" for single ECUs. When ECUs exist in pairs (e.g. for cars that have separate controls for the left and right engine banks), the section headers will look like "[PSX1]" or "[PDX1]", which indicate 'sinistra' and 'destra' respectively.

The name of a parameter or fault code is given by a key/value pair line that begins with `NOME_ING=...`, where `ING` is the abbreviation of the language.

This repo contains the source code module `txt-to-yaml.cpp`, which attempts to convert these SD2 .TXT definitions to a modern (YAML) representation as automatically as possible. A significant amount of manual effort is still required on the YAML output of this tool. More specifically, the `GENERAL=` line in the .TXT files often contains a memory address, but sometimes it is an offset within a page of snapshop data that was saved by the ECU. In other cases, the `GENERAL=` line contains both an address *and* number of bytes, or even a sequence of several bytes. Furthermore, the knowledge required to convert raw parameter values into real/physical units -- such as the value of each LSB increment -- is not recorded in the .TXT configuration file and must be discovered by other means.

