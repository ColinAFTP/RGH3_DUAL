#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_comms.h"
#include "functions_i2c.h"
#include "functions_io.h"
#include "variables.h"


int led = 13;
uint32_t tickerTime = 0;

void receiveCommand(int howMany) {
    if (Wire2.available()) {
        i2cCommand = Wire2.read();   // 1 = status, 2 = gap patterns, etc.
    }
}

void setup()
{
  pinMode(led, OUTPUT);
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

  digitalWrite(led, HIGH); 

  // Set up the Modbus server and address locations
  modbusSetup();

  // Clear the relay outputs
  relayControl(0);

  // Get the start time
  tickerTime = millis();

}

void loop()
{

  if (!ethernetClient.connected()) {
    ethernetConnect();
  } else {

      // Poll for Modbus TCP requests
      modbusServer.poll();

      // 2 second timer
      if (millis() - tickerTime > 2000) {
        tickerTime = millis();
        inputsUpdate();
        Serial.print("   | Input data: ");
        Serial.println(inputData);
        relayControl(inputData);
      }
    }
  
    //Check for new pattern requests
    patternCheck();
    static uint32_t pulseStartTime = 0;
    static bool pulseActive = false;
    if (patternSelection != patternSelectionPrevious) {

      Serial.print("   | Current pattern: ");
      Serial.println(patternSelection);
      
      // Start 500 ms pulse on the new move output pin
      digitalWrite(OUTPUT_A1, HIGH);
      pulseStartTime = millis();
      pulseActive = true;

      patternSelectionPrevious = patternSelection;
    }

    // Handle pulse timing (non-blocking)
    if (pulseActive && (millis() - pulseStartTime >= 500)) {
        digitalWrite(OUTPUT_A1, LOW);
        pulseActive = false;
    }

}


	

   
  


