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
uint32_t plotTime;
static bool lastInputA1State = false;                      // Remember previous input state

// Print the stepper positions in Serial Plotter format (values must be plain numbers)
static void plotPositions() {
  updateStepperPositions();
  Serial.print(">");
  for (int i = 0; i < NUM_GAPS; i++) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(stepperPositions[i] / STEPS_PER_MM);
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
  // Wait for a USB host for up to 3 s only, so the controller still starts when running stand-alone
  while (!Serial && millis() < 3000) {
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

  // Initialise the update time variables
  // Set to trigger an immediate update on the first loop
  dataUpdateTime = millis() - 10000;
  plotTime = millis();

}

void loop()
{

  // Detect the end of a move and set the at-home / at-target outputs
  moveService();

  // If the stepper motors are standing still, do housekeeping
  if (!moveInProgress()) {

    // If more than 10 seconds have elapsed since the last pattern update, then initiate data request from CPU1
    if (millis() - dataUpdateTime > 10000) {
      dataUpdateTime = millis();
      readGapPatterns();
    }
  }

  // Check for new pattern movement requests
  bool currentInputA1State = digitalRead(INPUT_A1);
  if (currentInputA1State && !lastInputA1State) {
    Serial.println("Trigger signal (INPUT_A1) detected!");
    if (moveInProgress()) {
      // Do nothing else: reading data or changing speeds here would disturb the running move
      Serial.println("Move requested while already moving - ignored.");
      lastInputA1State = currentInputA1State;
      return;
    }
    int pattern = readPattern();
    Serial.print("   | Current pattern: ");
    Serial.println(pattern);

    // Refresh the gap data and speed straight away so the move never uses stale values
    if (pattern < 0 || pattern >= NUM_PATTERNS) {
      Serial.println("Move aborted: invalid pattern received from CPU1.");
    } else if (!readGapPatterns()) {
      Serial.println("Move aborted: could not refresh gap data from CPU1.");
    } else if (stepTargetCalc(pattern)) {
      updateStepperSpeeds(stepperSpeed);
      triggerMove();
    } else {
      Serial.println("Move aborted: targets not valid.");
    }
  }

  // Update state for next loop
  lastInputA1State = currentInputA1State;

  // Optional position plotting for the Serial Plotter
  if (DEBUG_PLOT && millis() - plotTime >= 50) {
    plotTime = millis();
    plotPositions();
  }

}
