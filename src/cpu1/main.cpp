#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_comms.h"
#include "functions_i2c.h"
#include "functions_io.h"
#include "variables.h"

// Set up local variables
static uint32_t inputUpdateTime;
static uint32_t dataUpdateTime;
static uint32_t pulseStartTime;
static bool pulseActive = false;

void receiveCommand(int howMany) {
    if (Wire2.available()) {
        i2cCommand = Wire2.read();   // 1 = status, 2 = gap patterns, etc.
    }
}

void setup()
{
  Wire2.begin(0x40);                  // Join I2C bus with address #8
//  Wire2.onRequest(writeGapPatterns);   // Register event
  Wire2.onReceive(receiveCommand);
  Wire2.onRequest(onI2CRequest);    // Register event
  Wire2.setClock(1000000);

  Serial.begin(9600);                 // Start serial for output
  while(!Serial)
  {

  }
  
  // Clear the PuTTY terminal
  Serial.print("\033[2J");   // Clear screen
  Serial.print("\033[H");    // Move cursor to home position

  Serial.println("CPU 1 online and standing by...");
  Serial.println("===============================");
  Serial.println();

  // Call initialisation routines
  initShiftRegisters();
  initCPU1HardIO();

  // Indicate that gap patterns should be loaded from the PLC on startup
  bootLoadGaps = true;

  // Set up the Modbus server and address locations
  modbusSetup();

  // Clear the relay outputs
  relayControl(0);

  // Get the start time
  tickerTime = millis();

  // Initialise local variables
  inputUpdateTime = millis();
  dataUpdateTime = millis();
  pulseStartTime = millis();

}

void loop()
{

  if (!ethernetClient.connected()) {
    ethernetConnect();
  } else {

    // Poll for Modbus TCP requests
    modbusServer.poll();
  
    // Check for new pattern gap data from the PLC
    if ((millis() - dataUpdateTime >= 5000) || bootLoadGaps) {  // Check every 5 s or on boot load
      patternUpdateCheck();
      dataUpdateTime = millis();
      bootLoadGaps = false;
    }

    // Check for new pattern requests
    patternCheck();
    if (patternSelection != patternSelectionPrevious) {

      Serial.print("   | Current pattern: ");
      Serial.println(patternSelection);
      
      // Start 500 ms pulse on the new move output pin
      digitalWrite(OUTPUT_A1, HIGH);
      pulseStartTime = millis();
      pulseActive = true;

      patternSelectionPrevious = patternSelection;
    }

    // Check for relay updates
    relayCheck();
    if (relayData != relayDataPrevious) {
      Serial.print("   | New relay data: ");
      Serial.println(relayData, BIN);
      relayControl(relayData);
      relayDataPrevious = relayData;
    }

    // Check for speed updates
    speedCheck();
    if (speedData != speedDataPrevious) {
      Serial.print("   | New speed data: ");
      Serial.println(speedData);
      stepperSpeed = speedData;
      speedDataPrevious = speedData;
    }

  }

  // Check for new input updates every 25 ms
  if (millis() - inputUpdateTime >= 25) {  // Update every 25 ms
    inputsCheck();
    inputUpdateTime = millis();
  }
  if (inputData != inputDataPrevious) {
    Serial.print("   | New input data: ");
    Serial.println(inputData, BIN);
    updateInputs();
    inputDataPrevious = inputData;
  }

  // Handle pulse timing (non-blocking)
  if (pulseActive && (millis() - pulseStartTime >= 500)) {
      digitalWrite(OUTPUT_A1, LOW);
      pulseActive = false;
  }

  // Update the ticker every second
  if (millis() - tickerTime > 1000) {
    secondTicker++;
    tickerTime = millis();
    updateTicker(secondTicker);
  }


}


	

   
  


