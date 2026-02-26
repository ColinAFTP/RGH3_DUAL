#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "constants.h"

typedef struct {
    float patternData[NUM_PATTERNS][NUM_GAPS];
    int stepperSpeed;
} PatternPacket;

#endif
