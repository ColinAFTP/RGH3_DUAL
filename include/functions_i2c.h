#ifndef FUNCTIONS_I2C_H
#define FUNCTIONS_I2C_H

void copyToTransmitData();
void copyFromReceiveData();
void onI2CRequest();
int readIO();
int readPattern();
void readGapPatterns();

#endif
