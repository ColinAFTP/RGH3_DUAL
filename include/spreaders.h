#ifndef SPREADERS_H
#define SPREADERS_H

#include <stdint.h>
#include "constants.h"

// Conversions between the stepper index (0 to 8) and the physical spreader number (1 to 10, left to right; spreader 5 is the static one).

inline int spreaderNumber(int index) {                  // Stepper index -> spreader number
  return index < NUM_LEFT_SPREADERS ? index + 1 : index + 2;
}

inline int spreaderIndex(int number) {                  // Spreader number -> stepper index, or -1 for spreader 5 (static) or an invalid number
  if (number >= 1 && number <= NUM_LEFT_SPREADERS) return number - 1;
  if (number >= NUM_LEFT_SPREADERS + 2 && number <= NUM_GAPS + 1) return number - 2;
  return -1;
}

inline uint16_t spreaderBit(int index) {                // Bit of a stepper in the failed spreader bitmask: bit (spreader number - 1)
  return (uint16_t)1 << (spreaderNumber(index) - 1);
}

#endif
