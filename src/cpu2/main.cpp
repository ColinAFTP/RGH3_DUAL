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

  // Initialise the update time variables
  dataUpdateTime = millis();
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
      uint32_t readGapPatternsStart = micros();
      readGapPatterns();
      uint32_t readGapPatternsDuration = micros() - readGapPatternsStart;
      Serial.print("Gap pattern data transfer took ");
      Serial.print(readGapPatternsDuration);
      Serial.println(" microseconds");
      // Serial.println("Pattern 0 gaps.");
      // for (int c = 0; c < NUM_GAPS; c++) {
      //   Serial.print("Pattern 0 gap #");
      //   Serial.print(c + 1);
      //   Serial.print(": ");
      //   Serial.println(gapArrays[0][c]);
      // }
      Serial.println();

      // Request new input data from CPU1
      uint32_t readIOStart = micros();
      int status = readIO();
      uint32_t readIODuration = micros() - readIOStart;
      Serial.print("IO data transfer took ");
      Serial.print(readIODuration);
      Serial.println(" microseconds");
      Serial.print("   | Input data: ");
      Serial.println(status);
      Serial.println();
    }

    // Update stepper positions every 100ms
    if (millis() - positionUpdateTime > 100) {
      positionUpdateTime = millis();
      updateStepperPositions();
    }

  }

  // Check for new pattern movement requests
  // Hardwired to input A1 so that CPU1 can trigger pattern movements by changing the state of A1.
  // This is because the I2C communication for pattern data transfer is relatively slow, so we want to avoid it being triggered too frequently. 
  // By using a hardwired input, CPU1 can control exactly when CPU2 requests new pattern data, which is typically only when the stepper motors are standing still and a new pattern needs to be loaded.
  bool currentInputA1State = digitalRead(INPUT_A1);
  if (currentInputA1State && !lastInputA1State) {
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
  }

  // Update state for next loop
  lastInputA1State = currentInputA1State;

}