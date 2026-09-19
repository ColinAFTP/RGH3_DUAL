#ifndef FUNCTIONS_I2C_H
#define FUNCTIONS_I2C_H

#include <stdint.h>

void copyToTransmitData();
void copyFromReceiveData();
void onI2CRequest();
int readIO();                           // Returns the 16 bit input word from CPU1, or -1 if the transfer failed
bool writeFaultMask(uint16_t mask);     // Sends the failed spreader bitmask to CPU1. Returns false if the transfer failed
int readPattern();
bool readGapPatterns();                 // Returns false if the I2C transfer failed (old data kept)

#endif
