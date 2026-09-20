#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <teensystep4.h>

#include "functions_homing.h"
#include "functions_i2c.h"
#include "functions_manual.h"
#include "functions_status.h"
#include "functions_steppers.h"
#include "functions_watchdog.h"
#include "functions_io.h"
#include "variables.h"

using namespace TS4;

// Set up local variables
int led = LED_BUILTIN;
uint32_t dataUpdateTime;
uint32_t plotTime;
static bool lastInputA1State = false;                      // Remember previous start move input state
static bool lastInputA3State = false;                      // Remember previous fault reset input state

// Print the stepper positions in Serial Plotter format (values must be plain numbers).
// The traces are named after the spreader numbers 1 to 4 and 6 to 10 (spreader 5 is static).
static void plotPositions() {
  updateStepperPositions();
  Serial.print(">");
  for (int i = 0; i < NUM_GAPS; i++) {
    Serial.print("S");
    Serial.print(i < NUM_LEFT_SPREADERS ? i + 1 : i + 2);
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
  bool watchdogReset = watchdogCausedReset();   // Read this first: it says why the CPU started
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

  statusEvent(EVT_BOOT, watchdogReset ? 1 : 0);

  // Home automatically if not all the home sensors are on
  homingStartup();

  // Every long wait in setup() is over: from now on the watchdog resets the CPU if the main loop stops
  watchdogStart();

}

void loop()
{

  watchdogFeed();
  if (DEBUG_HANG_TEST_CPU2_S > 0 && millis() > (uint32_t)DEBUG_HANG_TEST_CPU2_S * 1000UL) {
    while (true) {}                     // TEST ONLY: hang, to check that the watchdog resets the CPU
  }

  // Over travel sensors: stop everything at once if CPU1 says one is on
  overTravelService();

  // Manual mode: the DIP switch and the PLC request, and the jog commands
  manualService();

  // Send the status and queued events to CPU1 for its web page
  statusService();

  // Detect the end of a TeensyStep move, and run the homing routine
  bool moveFinished = moveService();
  homingService(moveFinished);

  // A finished pattern move (or a TeensyStep only home) means at target. While homing, the next stage continues instead.
  if (moveFinished && moveWasAborted()) {
    // The move was stopped by the manual DIP switch: it never reached its target, so no At Target
    statusEvent(EVT_MOVE_ABORTED, 0);
  } else if (moveFinished && !homingActive()) {
    statusEvent(EVT_MOVE_DONE);
    digitalWrite(OUTPUT_B2, HIGH);
  }

  bool busy = moveInProgress() || homingActive() || manualRequested();

  // If the stepper motors are standing still, do housekeeping
  if (!busy) {

    // If more than 10 seconds have elapsed since the last pattern update, then initiate data request from CPU1
    if (millis() - dataUpdateTime > 10000) {
      dataUpdateTime = millis();
      readGapPatterns();
    }
  }

  // Fault reset request from CPU1 (rising edge on input A3)
  bool currentInputA3State = digitalRead(INPUT_A3);
  if (currentInputA3State && !lastInputA3State) {
    if (busy) {
      Serial.println("Fault reset requested while busy - ignored.");
    } else {
      faultReset();
    }
  }
  lastInputA3State = currentInputA3State;

  // Check for new pattern movement requests (rising edge on input A1)
  bool currentInputA1State = digitalRead(INPUT_A1);
  if (currentInputA1State && !lastInputA1State) {
    Serial.println("Trigger signal (INPUT_A1) detected!");
    // A new request: clear the "refused" flag, and At Target goes off until this request has really been carried out
    requestStarted();
    digitalWrite(OUTPUT_B2, LOW);
    if (manualRequested()) {
      // Manual mode is on: pattern and home requests are refused. The PLC has to send a new request after manual mode.
      Serial.println("Request refused: manual mode is on.");
      requestRefused(EVT_REASON_MANUAL);
    } else if (busy) {
      // Do nothing else: reading data or changing speeds here would disturb the running move
      Serial.println("Move requested while already moving - ignored.");
      requestRefused(EVT_REASON_BUSY);
    } else if (faultActive()) {
      Serial.println("Move refused: homing fault is active.");
      requestRefused(EVT_REASON_FAULT);
    } else {
      int pattern = readPattern();
      Serial.print("   | Current pattern: ");
      Serial.println(pattern);
      statusEvent(EVT_TRIGGER, pattern);

      // Refresh the gap data and speed straight away so the move never uses stale values
      if (pattern < 0 || pattern > NUM_PATTERNS) {
        Serial.println("Move aborted: invalid pattern received from CPU1.");
        requestRefused(EVT_REASON_BAD_PATTERN);
      } else if (!readGapPatterns()) {
        Serial.println("Move aborted: could not refresh gap data from CPU1.");
        requestRefused(EVT_REASON_NO_GAPS);
      } else if (pattern == 0) {
        // Pattern 0 is home
        if (!homeRequest()) {
          Serial.println("Home request failed.");
          requestRefused(EVT_REASON_HOME_FAILED);
        }
      } else if (!positionsKnown()) {
        // Only refused while the stepper positions are unknown (power up before the first home, or after a fault).
        // Moving from one pattern to another without going home in between is allowed.
        Serial.println("Move refused: stepper positions unknown, home first.");
        requestRefused(EVT_REASON_UNKNOWN_POS);
      } else if (stepTargetCalc(pattern)) {
        updateStepperSpeeds(stepperSpeed);
        if (triggerMove()) {
          statusEvent(EVT_MOVE_START, pattern);
        }
      } else {
        Serial.println("Move aborted: targets not valid.");
        requestRefused(EVT_REASON_BAD_TARGETS);
      }
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
