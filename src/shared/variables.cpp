#include <Arduino.h>

#include "variables.h"

// Shared globals
float gapPattern0[NUM_GAPS] = {0};
float gapPattern1[NUM_GAPS] = {0};
float gapPattern2[NUM_GAPS] = {0};
float gapPattern3[NUM_GAPS] = {0};
float gapPattern4[NUM_GAPS] = {0};
float gapPattern5[NUM_GAPS] = {0};
float transmitData[NUM_PATTERNS][NUM_GAPS] = {0};
float receiveData[NUM_PATTERNS][NUM_GAPS] = {0};
float* arrays[NUM_PATTERNS] = { gapPattern0, gapPattern1, gapPattern2, gapPattern3, gapPattern4, gapPattern5 };
volatile uint8_t i2cCommand = 0;

// Variables used for the IO signals
FastShiftIn* FSI = nullptr;
uint16_t inputData = 0;
bool digitalInput[16] = {0};
bool relayOutput[16] = {0};

// Variables used for comms
EthernetServer ethernetServer(502);
EthernetClient ethernetClient;
ModbusTCPServer modbusServer;
int patternSelection = 0;
int patternSelectionPrevious = 0;
