#ifndef FUNCTIONS_IO_H
#define FUNCTIONS_IO_H

#include <Arduino.h>

void initShiftRegisters();              // Shift registers initialisation function
void initCPU1HardIO();                  // Initialise the hardwired CPU1/CPU2 signals in CPU1
void initCPU2HardIO();                  // Initialise the hardwired CPU1/CPU2 signals in CPU2
void inputsUpdate();
void inputsStrip();
void relayControl(word outputData);

#endif
