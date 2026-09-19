#ifndef VARIABLES_CPU1_H
#define VARIABLES_CPU1_H

// Variables used only by CPU1 (Modbus/Ethernet, the input and relay shift registers).

#include <Arduino.h>
#include <NativeEthernet.h>
#include <ArduinoRS485.h>       // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <FastShiftIn.h>

#include "variables.h"

// Variables used for the IO signals
extern FastShiftIn* FSI;      // This is a pointer, not an object
extern uint16_t inputData;
extern uint16_t inputDataPrevious;

// Variables used for Ethernet communications
extern EthernetServer ethernetServer;
extern EthernetClient ethernetClient;
extern ModbusTCPServer modbusServer;
extern int patternSelection;
extern int patternSelectionPrevious;
extern word relayData;
extern word relayDataPrevious;
extern int speedData;
extern int speedDataPrevious;
extern uint32_t tickerTime;
extern uint16_t secondTicker;
extern bool bootLoadGaps;

#endif
