#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <teensystep4.h>

#include "functions_i2c.h"
#include "functions_steppers.h"
#include "functions_io.h"
#include "variables.h"

using namespace TS4;


int led = LED_BUILTIN;
uint32_t patternUpdateTime = 0;

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

  // Get the start time
  patternUpdateTime = millis();

}

void loop()
{

  // If the stepper motors are standing still and more than 10 seconds have elapsed since the
  // last pattern update, then request new pattern values from CPU1
  if (!motorsMoving() && (millis() - patternUpdateTime > 10000)) {
    patternUpdateTime = millis();

    uint32_t readGapPatternsStart = micros();
    readGapPatterns();
    uint32_t readGapPatternsDuration = micros() - readGapPatternsStart;

    Serial.print("Gap pattern data transfer took ");
    Serial.print(readGapPatternsDuration);
    Serial.println(" microseconds");

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

static bool lastState = false;                      // Remember previous input state
bool currentState = digitalRead(INPUT_A1);
if (currentState && !lastState) {
    uint32_t readPatternStart = micros();
    int pattern = readPattern();
    uint32_t readPatternDuration = micros() - readPatternStart;

    Serial.print("Pattern selection transfer took ");
    Serial.print(readPatternDuration);
    Serial.println(" microseconds");
    Serial.print("   | Current pattern: ");
    Serial.println(pattern);

    Serial.println();
  }
// Update state for next loop
lastState = currentState;

}