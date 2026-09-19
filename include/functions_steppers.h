#ifndef FUNCTIONS_STEPPERS_H
#define FUNCTIONS_STEPPERS_H

bool motorsMoving();
void stepTargetCalc(int patternChoice);
void updateStepperPositions();
void updateStepperSpeeds(int speed);
void triggerMove(int patternChoice);

#endif
