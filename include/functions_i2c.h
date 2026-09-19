#ifndef FUNCTIONS_I2C_H
#define FUNCTIONS_I2C_H

#include <stdint.h>

// CPU1 (I2C slave), see src/cpu1/functions_i2c_cpu1.cpp
void copyToTransmitData();
void onI2CRequest();

// CPU2 (I2C master), see src/cpu2/functions_i2c_cpu2.cpp
void copyFromReceiveData();
int readIO();                           // Returns the 16 bit input word from CPU1, or -1 if the transfer failed
bool writeFaultMask(uint16_t mask);     // Sends the failed spreader bitmask to CPU1. Returns false if the transfer failed
int readPattern();                      // Returns the pattern selection (0 = home) from CPU1, or -1 if the transfer failed
bool readGapPatterns();                 // Returns false if the I2C transfer failed (old data kept)

#endif
