#ifndef FUNCTIONS_STEPPERS_H
#define FUNCTIONS_STEPPERS_H

bool motorsMoving();
bool stepTargetCalc(int patternChoice); // Returns false (targets untouched) if the pattern or any target is invalid
void updateStepperPositions();
void updateStepperSpeeds(int speed);
bool triggerMove();                     // Start a non-blocking move to stepperTargets. Returns false if already moving
void moveService();                     // Call every loop: detects move completion and sets the at-target output
bool moveInProgress();

#endif
