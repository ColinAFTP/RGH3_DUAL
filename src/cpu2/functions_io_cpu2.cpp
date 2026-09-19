#include "constants.h"
#include "functions_io.h"

// CPU2 side of the hardwired CPU1/CPU2 signals. See the pin descriptions in constants.h.

// Initialise the hardwired CPU1/CPU2 signals in CPU2
void initCPU2HardIO() {
  // Pull-downs so a disconnected wire reads LOW instead of floating and giving false triggers
  pinMode(INPUT_A1, INPUT_PULLDOWN);
  pinMode(INPUT_A2, INPUT_PULLDOWN);
  pinMode(INPUT_A3, INPUT_PULLDOWN);
  pinMode(INPUT_A4, INPUT_PULLDOWN);
  pinMode(OUTPUT_B1, OUTPUT);
  pinMode(OUTPUT_B2, OUTPUT);
  pinMode(OUTPUT_B3, OUTPUT);
  pinMode(OUTPUT_B4, OUTPUT);
  digitalWrite(OUTPUT_B1, LOW);
  digitalWrite(OUTPUT_B2, LOW);
  digitalWrite(OUTPUT_B3, LOW);
  digitalWrite(OUTPUT_B4, LOW);
};
