#include "variables.h"

// Shared globals
float gapPattern0[numGaps] = {0};
float gapPattern1[numGaps] = {0};
float gapPattern2[numGaps] = {0};
float gapPattern3[numGaps] = {0};
float gapPattern4[numGaps] = {0};
float gapPattern5[numGaps] = {0};
float transmitData[numPatterns][numGaps] = {0};
float receiveData[numPatterns][numGaps] = {0};
float* arrays[numPatterns] = { gapPattern0, gapPattern1, gapPattern2, gapPattern3, gapPattern4, gapPattern5 };

// CPU‑specific globals
#ifdef CPU1
int cpu1Counter = 0;
#endif

#ifdef CPU2
int cpu2Counter = 0;
#endif
