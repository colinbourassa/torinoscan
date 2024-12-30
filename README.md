# torinoscan

This tool is a generic framework that is intended to be used for displaying automotive ECU parameters and fault codes. It contains no built-in support for diagnostic protocols; support for certain protocols is provided by external libraries. Information about ECU types and addresses/locations of parameters is provided in JSON configuration files. These configuration files also indicate the protocol that must be used to communicate with a particular ECU.

## Conversion of SD2 data

Because this utility is intended to be a replacement for the Ferrari SD2 system, it is helpful to be able to convert the original SD2 configuration files that contain ECU parameter definitions. These original definition files have a .TXT extension but use the .INI format. They contain definitions for readable parameters, fault codes, and actuators that may be triggered via the diagnostic connection.

Parameters are identified by section headers like "[P1]" for single ECUs. When ECUs exist in pairs (e.g. for cars that have separate controls for the left and right engine banks), the section headers will look like "[PSX1]" or "[PDX1]", which indicate 'sinistra' and 'destra' respectively.

The name of a parameter or fault code is given by a key/value pair line that begins with `NOME_ING=...`, where `ING` is the abbreviation of the language.

This repo contains the source code module `txt-to-yaml.cpp`, which attempts to convert these SD2 .TXT definitions to a modern (YAML) representation as automatically as possible. A significant amount of manual effort is still required on the YAML output of this tool. More specifically, the `GENERAL=` line in the .TXT files often contains a memory address, but sometimes it is an offset within a page of snapshop data that was saved by the ECU. In other cases, the `GENERAL=` line contains both an address *and* number of bytes, or even a sequence of several bytes. Furthermore, the knowledge required to convert raw parameter values into real/physical units -- such as the value of each LSB increment -- is not recorded in the .TXT configuration file and must be discovered by other means.

