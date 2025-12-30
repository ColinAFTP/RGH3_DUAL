#include <structures.h>

// Define the arrays for each individual gap pattern. The number of arrays must equal the
// numPatterns constant declared in constants.h
float gapPattern0[numGaps];
float gapPattern1[numGaps];
float gapPattern2[numGaps];
float gapPattern3[numGaps];
float gapPattern4[numGaps];
float gapPattern5[numGaps];

// Pointer list for easy iteration
float* arrays[numPatterns] = { gapPattern0, gapPattern1, gapPattern2, gapPattern3, gapPattern4, gapPattern5 };

// Combined 1-dimensional array used for I2C communications between the Teensys since multi-dimensional
// arrays don't seem to work as variable structures for the I2C comms library
float transmitData[numPatterns*numGaps];
float receiveData[numPatterns*numGaps];

// i2cData transmitData;
// i2cData receiveData;

