//Opening the Plotter
//In VS Code, press CTRL + SHIFT + P (CMD + SHIFT + P on macOS) to open the command palette.
//Type "Serial Plotter: Open pane" and select the command. The pane will open.


#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <teensystep4.h>

#include "functions_i2c.h"
#include "functions_steppers.h"
#include "functions_io.h"
#include "variables.h"

using namespace TS4;

// Set up local variables
int led = LED_BUILTIN;
uint32_t dataUpdateTime;
uint32_t positionUpdateTime;
static bool lastInputA1State = false;                      // Remember previous input state

IntervalTimer t1;

// This callback function is called by the interval timer to plot the stepper positions.
void onTimer() {
  updateStepperPositions();
  Serial.print(">");
  for (int i = 0; i < NUM_GAPS; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(stepperPositions[i] / STEPS_PER_MM);
    Serial.print(" mm");
    if (i < NUM_GAPS - 1) {
      Serial.print(",");
    }
  }
  Serial.println();
}

void setup()
{
  pinMode(led, OUTPUT);
  Wire2.begin();                         // Join I2C bus

  Serial.begin(9600);                    // Start serial for output
  while(!Serial)
  {

  }
  
  // Clear the PuTTY terminal
  Serial.print("\033[2J");   // Clear screen
  Serial.print("\033[H");    // Move cursor to home position

  // Start the stepper service
  TS4::begin();

  // Give stepper drivers a chance to start up
  delay(5000);
  Serial.println();
  Serial.println("CPU 2 online and standing by...");
  Serial.println("===============================");
  Serial.println();

  // Call initialisation routines
  initCPU2HardIO();

  // Start the plotting timer (50 ms interval)
  t1.begin(onTimer, 50000);

  // Initialise the update time variables
  // Set to trigger an immediate update on the first loop
  dataUpdateTime = millis() - 10000;
  positionUpdateTime = millis();

}

void loop()
{

  // If the stepper motors are standing still, do housekeeping
  if (!motorsMoving()) {
    
    // If more than 10 seconds have elapsed since the last pattern update, then initiate data request from CPU1
    if (millis() - dataUpdateTime > 10000) {
      dataUpdateTime = millis();

      // Request new pattern gap data from CPU1
      Serial.println("Housekeeping: Updating gap patterns from CPU1...");
      uint32_t readGapPatternsStart = micros();
      readGapPatterns();
      uint32_t readGapPatternsDuration = micros() - readGapPatternsStart;
      // Serial.print("Gap pattern data transfer took ");
      // Serial.print(readGapPatternsDuration);
      // Serial.println(" microseconds");
    }
  }

  // Check for new pattern movement requests
  bool currentInputA1State = digitalRead(INPUT_A1);
  if (currentInputA1State && !lastInputA1State) {
    Serial.println("Trigger signal (INPUT_A1) detected!");
    uint32_t readPatternStart = micros();
    int pattern = readPattern();
    uint32_t readPatternDuration = micros() - readPatternStart;

    Serial.print("Pattern selection transfer took ");
    Serial.print(readPatternDuration);
    Serial.println(" microseconds");
    Serial.print("   | Current pattern: ");
    Serial.println(pattern);

    Serial.println();

    // Calculate the new stepper target positions based on the selected pattern
    stepTargetCalc(pattern);

    // Update the stepper speeds based on the latest value received from CPU1
    updateStepperSpeeds(stepperSpeed);

    // Trigger the movement (blocking)
    triggerMove(pattern);
  }

  // Update state for next loop
  lastInputA1State = currentInputA1State;

}
