#ifndef VARIABLES_H
#define VARIABLES_H

// Variables used by both CPUs. Variables only CPU1 uses (Ethernet, Modbus, IO and relays) are in variables_cpu1.h.

#include <Arduino.h>

#include "structures.h"

// Variables used for I2C communications
extern PatternPacket transmitPacket;    // CPU1 fills this and sends it to CPU2
extern PatternPacket receivePacket;     // CPU2 receives into this
extern float gapArrays[NUM_PATTERNS][STRIDE_GAPS]; 
extern int stepperSpeed;
extern volatile uint8_t i2cCommand;     // Last command CPU1 received from CPU2 (see the I2C_CMD_ constants)
extern volatile uint16_t faultMaskRx;   // Failed spreader bitmask received from CPU2 over I2C (CPU1)
extern volatile uint8_t faultTypeRx;    // Fault type received from CPU2 over I2C (CPU1), a FAULT_ constant
extern volatile bool faultMaskNew;      // Set by the I2C receive handler when faultMaskRx has a new value (CPU1)

// Variables used for stepper control (CPU2)
extern long stepperTargets[NUM_GAPS];
extern long stepperPositions[NUM_GAPS];

#endif
