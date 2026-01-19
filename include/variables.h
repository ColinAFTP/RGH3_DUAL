#ifndef VARIABLES_H
#define VARIABLES_H

#include <NativeEthernet.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <FastShiftIn.h>

#include "constants.h"

// Define the arrays for each individual gap pattern. The number of arrays must equal the
// NUM_PATTERNS constant declared in constants.h
extern float gapPattern0[NUM_GAPS];
extern float gapPattern1[NUM_GAPS];
extern float gapPattern2[NUM_GAPS];
extern float gapPattern3[NUM_GAPS];
extern float gapPattern4[NUM_GAPS];
extern float gapPattern5[NUM_GAPS];

// Pointer list for easy iteration
extern float* arrays[NUM_PATTERNS];

// Variables used for I2C communications
extern float transmitData[NUM_PATTERNS][NUM_GAPS];
extern float receiveData[NUM_PATTERNS][NUM_GAPS];
extern volatile uint8_t i2cCommand;

// Variables used for the IO signals
extern FastShiftIn* FSI;      // This is a pointer, not an object
extern uint16_t inputData;
extern bool digitalInput[16];
extern bool relayOutput[16];

// Variables used for Ethernet communications
extern EthernetServer ethernetServer;
extern EthernetClient ethernetClient;
extern ModbusTCPServer modbusServer;
extern int patternSelection;
extern int patternSelectionPrevious;

#endif
