#ifndef VARIABLES_H
#define VARIABLES_H

#include "constants.h"

// GLOBAL VARIABLES USED BY BOTH CPUS
// ==================================

// Define the arrays for each individual gap pattern. The number of arrays must equal the
// numPatterns constant declared in constants.h
extern float gapPattern0[numGaps];
extern float gapPattern1[numGaps];
extern float gapPattern2[numGaps];
extern float gapPattern3[numGaps];
extern float gapPattern4[numGaps];
extern float gapPattern5[numGaps];

// Pointer list for easy iteration
extern float* arrays[numPatterns];

// 2-dimensional arrays used for transmitting and receving I2C data between the Teensys
extern float transmitData[numPatterns][numGaps];
extern float receiveData[numPatterns][numGaps];

// CPU SPECIFIC GLOBAL VARIABLES
// =============================

#ifdef CPU1
extern int cpu1Counter;
#endif

#ifdef CPU2
extern int cpu2Counter;
#endif

#endif
