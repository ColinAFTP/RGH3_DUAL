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

// Variables used for the input signals
uint16_t inputData = 0;

// Variables used for comms
bool newClient = false;
EthernetServer ethernetServer(502);
EthernetClient ethernetClient;
ModbusTCPServer modbusServer;
