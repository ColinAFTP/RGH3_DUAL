#ifndef VARIABLES_H
#define VARIABLES_H

#include <NativeEthernet.h>
#include <ArduinoRS485.h>       // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <FastShiftIn.h>

#include "structures.h"

// Define the arrays for each individual gap pattern. The number of arrays must equal the
// NUM_PATTERNS constant declared in constants.h
extern float gapPattern0[NUM_GAPS];
extern float gapPattern1[NUM_GAPS];
extern float gapPattern2[NUM_GAPS];
extern float gapPattern3[NUM_GAPS];
extern float gapPattern4[NUM_GAPS];
extern float gapPattern5[NUM_GAPS];

// Variables used for I2C communications
extern PatternPacket transmitPacket; 
extern PatternPacket receivePacket;
extern float gapArrays[NUM_PATTERNS][NUM_GAPS]; 
extern int stepperSpeed;
extern volatile uint8_t i2cCommand;

// Variables used for the IO signals
extern FastShiftIn* FSI;      // This is a pointer, not an object
extern uint16_t inputData;
extern uint16_t inputDataPrevious;
extern bool digitalInput[16];
extern bool relayOutput[16];

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

// Variables used for stepper control
extern long stepperTargets[NUM_GAPS];
extern long stepperPositions[NUM_GAPS];

#endif
