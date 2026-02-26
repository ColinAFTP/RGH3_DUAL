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
Stepper steppers[NUM_GAPS] = {stepper1, stepper2, stepper3, stepper4, stepper5, stepper6, stepper7, stepper8, stepper9};

StepperGroup g1{steppers[0], steppers[1], steppers[2], steppers[3], steppers[4], steppers[5], steppers[6], steppers[7], steppers[8]};


// This subroutine checks if any of the motors are moving
bool motorsMoving() {
  bool returnState = false;
  // Check if any of the motors are moving
  for (int i = 0; i < NUM_GAPS; i++) {
    returnState |= steppers[i].isMoving;
  }
  // Return the result
  return returnState;
}

// This subroutine calculates the target positions for each stepper motor based on the selected pattern.
void stepTargetCalc(int patternChoice) {

  bool debugPrinting = false;               // Set to true to enable debug printing of stepper target calculations
  float tempStepperArray[NUM_GAPS];

  if (debugPrinting) {
    Serial.println();
    Serial.print("New pattern selection: ");
    Serial.println(patternChoice);
  }

  int mid = NUM_GAPS / 2;                 // Midpoint index

  // ----- LEFT SIDE -----
  // For i = 0 to mid:
  // temp[i] = sum of gap[i..mid]
  for (int i = 0; i <= mid; i++) {
    float sum = 0;
    for (int j = i; j <= mid; j++) {
      sum += gapArrays[patternChoice][j];
    }
    tempStepperArray[i] = sum * STEPS_PER_MM;
    if (debugPrinting) {
      Serial.print("Target ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(sum);
      Serial.print(" mm => ");
      Serial.print(tempStepperArray[i]);
      Serial.println(" steps");
    }
  }
  if (debugPrinting) {
    Serial.print("Mid index: ");
    Serial.println(mid);
  }
  // ----- RIGHT SIDE -----
  // For i = mid+1 to NUM_GAPS-1:
  // temp[i] = sum of gap[mid..i]
  for (int i = mid + 1; i < NUM_GAPS; i++) {
    float sum = 0;
    for (int j = mid + 1; j <= i; j++) {
      sum += gapArrays[patternChoice][j];
    }
    tempStepperArray[i] = sum * STEPS_PER_MM;
    if (debugPrinting) {
      Serial.print("Target ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(sum);
      Serial.print(" mm => ");
      Serial.print(tempStepperArray[i]);
      Serial.println(" steps");
      Serial.println();
    }
  }

  // ----- LIMIT CHECK -----
  if (tempStepperArray[0] <= MAX_STEPS && tempStepperArray[NUM_GAPS - 1] <= MAX_STEPS) {
    for (int i = 0; i < NUM_GAPS; i++) {
      stepperTargets[i] = tempStepperArray[i];
    }
  } else {
    Serial.println("Stepper targets out of bounds! Targets not loaded.");
    Serial.print("Target 0: ");
    Serial.println(tempStepperArray[0]);
    Serial.print("Target last: ");
    Serial.println(tempStepperArray[NUM_GAPS - 1]);
  }
}

void updateStepperPositions() {
  for (int i = 0; i < NUM_GAPS; i++) {
    stepperPositions[i] = steppers[i].getPosition();
  }
}

// This subroutine updates the speed of all stepper motors.
void updateStepperSpeeds(int speed) {
  for (int i = 0; i < NUM_GAPS; i++) {
    steppers[i].setMaxSpeed(speed);
  }
}