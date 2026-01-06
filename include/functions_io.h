#ifndef FUNCTIONS_IO_H
#define FUNCTIONS_IO_H

#include <Arduino.h>

void initShiftRegisters();              // Shift registers initialisation function
void inputsUpdate();
void relayControl(word outputData);

#endif
