#include <Arduino.h>

#include "variables.h"

// Shared globals
float gapPattern0[NUM_GAPS] = {0};
float gapPattern1[NUM_GAPS] = {0};
float gapPattern2[NUM_GAPS] = {0};
float gapPattern3[NUM_GAPS] = {0};
float gapPattern4[NUM_GAPS] = {0};
float gapPattern5[NUM_GAPS] = {0};

volatile uint8_t i2cCommand = 0;

PatternPacket transmitPacket; 
PatternPacket receivePacket; 
float gapArrays[NUM_PATTERNS][NUM_GAPS]; 
int stepperSpeed = 0;

// Variables used for the IO signals
FastShiftIn* FSI = nullptr;
uint16_t inputData = 0;
uint16_t inputDataPrevious = 0;
bool digitalInput[16] = {0};
bool relayOutput[16] = {0};

// Variables used for comms
EthernetServer ethernetServer(502);
EthernetClient ethernetClient;
ModbusTCPServer modbusServer;
int patternSelection = 0;
int patternSelectionPrevious = 0;
word relayData = 0;
word relayDataPrevious = 0;
int speedData = 0;
int speedDataPrevious = 0;
uint32_t tickerTime = 0;
uint16_t secondTicker = 0;
bool bootLoadGaps = false;

// Variables used for stepper control
long stepperTargets[NUM_GAPS] = {0};
long stepperPositions[NUM_GAPS] = {0};

