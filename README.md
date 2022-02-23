# Device and Link Discovery in Industrial Ethernet Networks
## Abstract
Today production facilities are optimized for single products with high quantities. Switching the focus of production requires a lot of preparation, planning, and workforce. With Industry 4.0 interoperability and flexibility of sensors, actuators, and computing units are getting more important. The increased amount of devices makes the reconfiguration of the Industrial Ethernet structures more complex, which can result in long downtimes. Therefore an automatic discovery mechanism is desired, which allows reconfiguring the network with a single button press.

This thesis implements a prototype of such discovery, it detects devices and their ports with Link Layer Discovery Protocol (LLDP) and prepares a network graph with the collected data for the Time-Sensitive Networking (TSN) Standard. The prototype was tested in a laboratory environment. It turned out that the functionality was limited by the manufacturers of the network devices used.

## Intro
This application uses SNMP to collect LLDP data from ethernet devices, by creating an SNMP daemon. With this data, it is possible to view the structure of an Ethernet network. The collected data and structure are saved in an SQLite database. After an initial scan, the application is listening for incoming SNMPv1 traps. This way network devices can announce changes on their interfaces.

**This application is only a prototype and only works in special test environments.**

## Structure
This project is structured in two sections:
- **application** - This is a prototype for Device and Link Discovery wrote in C. It includes the source code and documentation. The documentation is created by Doxygen.
- **thesis** - This is the bachelor thesis written in LaTeX.

## Application
For building this project Manjaro Linux was used, but it should be possible with any Linux Distribution.

1. Install `net-snmp lldp gcc make doxygen clang sqlite`
2. Open a terminal
3. Change the directory with `cd application`
4. Run `make all` to build the application, docs, dependencies
5. This should generate three binaries: `bin/application bin/external/onesixtyone bin/external/snmpwalk`
6. The folder structure including the external folder must remain.
5. To clean the build run `make clean`

### Usage
The application needs rood privileges because an SNMP daemon is created. It also needs two arguments:
- **host:** The network including the subnet. eg: 192.168.0.0/16
- **community:** The SNMP community used for getting the SNMP data, default is public.

## Thesis
For building this project Manjaro Linux was used, but it should be possible with any Linux Distribution.

1. Install `texlive-full`
2. Open a terminal
3. Change the directory with `cd thesis`
4. Set the build script as executable with `chmod +x ./build-thesis.sh`
5. Run the build script with `./build-thesis.sh`
6. This should generate a pdf file called **thesis.pdf**
7. To clean the build run `./clean-thesis.sh`

## VSCode
This project used VSCode as an IDE, these plugins have been used:
- C/C++ by Microsoft
- Doxygen Documentation Generator by Christoph Schlosser
- LaTeX Workshop by James Yu
- SQLite by alexcvzz

## Tipps
The thesis includes the Concept(Chapter 3) of the application and how it is implemented(Chapter 4). These chapters should be read if a better insight is needed.

## Projects used (application/external)
- net-snmp: https://github.com/net-snmp/net-snmp
- onesixtyone: https://github.com/trailofbits/onesixtyone

## Libaries used (application/src/lib)
- generic linked list: https://github.com/philbot9/generic-linked-list
- simple dynamic strings: https://github.com/antirez/sds