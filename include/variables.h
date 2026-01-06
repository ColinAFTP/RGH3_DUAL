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

// 2-dimensional arrays used for transmitting and receving I2C data between the Teensys
extern float transmitData[NUM_PATTERNS][NUM_GAPS];
extern float receiveData[NUM_PATTERNS][NUM_GAPS];

// Variables used for the input signals
extern FastShiftIn* FSI;      // This is a pointer, not an object
extern uint16_t inputData;

// Variables used for communications
extern EthernetServer ethernetServer;
extern EthernetClient ethernetClient;
extern ModbusTCPServer modbusServer;
extern int patternSelection;
extern int patternSelectionPrevious;

#endif
