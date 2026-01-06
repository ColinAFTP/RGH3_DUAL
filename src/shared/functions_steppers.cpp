#include <teensystep4.h>

#include "constants.h"
#include "functions_steppers.h"
#include "variables.h"

using namespace TS4;

// Stepper objects
// Arguments are x & y where
// x is the pulse output pin
// y is the direction output pin
Stepper stepper1(STEPPER1_PULSE_PIN, STEPPER1_DIR_PIN); 
Stepper stepper2(STEPPER2_PULSE_PIN, STEPPER2_DIR_PIN); 
Stepper stepper3(STEPPER3_PULSE_PIN, STEPPER3_DIR_PIN); 
Stepper stepper4(STEPPER4_PULSE_PIN, STEPPER4_DIR_PIN); 
Stepper stepper5(STEPPER5_PULSE_PIN, STEPPER5_DIR_PIN); 
Stepper stepper6(STEPPER6_PULSE_PIN, STEPPER6_DIR_PIN); 
Stepper stepper7(STEPPER7_PULSE_PIN, STEPPER7_DIR_PIN); 
Stepper stepper8(STEPPER8_PULSE_PIN, STEPPER8_DIR_PIN); 
Stepper stepper9(STEPPER9_PULSE_PIN, STEPPER9_DIR_PIN);
Stepper steppers[9] = {stepper1, stepper2, stepper3, stepper4, stepper5, stepper6, stepper7, stepper8, stepper9};

StepperGroup g1{steppers[0], steppers[1], steppers[2], steppers[3], steppers[4], steppers[5], steppers[6], steppers[7], steppers[8]};

// Stepper control arrays
long stepperTargets[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
long stepperPositions[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};


// This subroutine checks if any of the motors are moving
bool motorsMoving() {
  bool returnState = false;
  // Check if any of the motors are moving
  for (int i = 0; i <= 8; i++) {
    returnState |= steppers[i].isMoving;
  }
  // Return the result
  return returnState;
}
