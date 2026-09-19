#ifndef FUNCTIONS_STEPPERS_H
#define FUNCTIONS_STEPPERS_H

bool motorsMoving();
bool stepTargetCalc(int patternChoice); // 0 = home, 1 to NUM_PATTERNS = gap pattern. Returns false (targets untouched) if the pattern or any target is invalid
void updateStepperPositions();
void setAllStepperPositions(long position); // Tell TeensyStep where all the steppers are (used after homing)
void updateStepperSpeeds(int speed);
bool triggerMove();                     // Start a non-blocking TeensyStep move to stepperTargets. Returns false if already moving
bool moveService();                     // Call every loop: returns true once when a move has just finished
bool moveInProgress();

#endif
