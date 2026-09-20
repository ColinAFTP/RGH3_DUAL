#ifndef FUNCTIONS_MANUAL_CPU1_H
#define FUNCTIONS_MANUAL_CPU1_H

#include <stdint.h>

// CPU1 side of manual mode: the manual DIP switch, the PLC coils and register, and what the PLC is told back.

void manualInit();                      // Set up the DIP switch pin and the line to CPU2. Call in setup() before the I2C slave starts
void manualCpu1Service();               // Call every loop: reads the switch, coils and register, prepares the command for CPU2, updates DI 119 and coil 101

bool manualDipOn();                     // The manual DIP switch is on (debounced)
bool manualSoftOn();                    // The PLC has set coil ADDR_MANUAL
bool manualModeOn();                    // Manual mode is on or requested: DIP switch, PLC request, or CPU2 reports manual mode
bool manualCpu2Active();                // CPU2 reports that manual mode is active
uint8_t manualOpenClose();              // Bit 0: open coil, bit 1: close coil, as sent to CPU2 (already cancelled if the PLC is silent)
uint8_t manualSpreader();               // Register ADDR_MANUAL_PTR as sent to CPU2

#endif
