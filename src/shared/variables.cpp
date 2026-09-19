#include <Arduino.h>

#include "variables.h"

// Shared globals
PatternPacket transmitPacket; 
PatternPacket receivePacket; 
float gapArrays[NUM_PATTERNS][STRIDE_GAPS]; 
int stepperSpeed = 0;
volatile uint8_t i2cCommand = 0;
volatile uint16_t faultMaskRx = 0;
volatile uint8_t faultTypeRx = 0;
volatile bool faultMaskNew = false;

// Variables used for stepper control
long stepperTargets[NUM_GAPS] = {0};
long stepperPositions[NUM_GAPS] = {0};
