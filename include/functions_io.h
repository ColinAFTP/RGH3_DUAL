#ifndef FUNCTIONS_IO_H
#define FUNCTIONS_IO_H

#include <Arduino.h>

// CPU1, see src/cpu1/functions_io_cpu1.cpp
void initShiftRegisters();              // Shift registers initialisation function
void initCPU1HardIO();                  // Initialise the hardwired CPU1/CPU2 signals in CPU1
void inputsCheck();                     // Sample the 16 inputs from the input shift registers into inputData
void relayControl(word outputData);     // Shift the 16 relay bits out to the relay shift registers
void feedbackCheck();                   // Update the Home, At Target and Homing Fault status bits and the two feedback relays

// CPU2, see src/cpu2/functions_io_cpu2.cpp
void initCPU2HardIO();                  // Initialise the hardwired CPU1/CPU2 signals in CPU2

#endif
