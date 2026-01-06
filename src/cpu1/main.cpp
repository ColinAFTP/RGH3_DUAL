#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_comms.h"
#include "functions_i2c.h"
#include "functions_io.h"
#include "variables.h"


int led = 13;
uint32_t tickerTime = 0;

void setup()
{
  pinMode(led, OUTPUT);
  Wire2.begin(0x40);                  // Join I2C bus with address #8
  Wire2.onRequest(writeGapPatterns);   // Register event
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

  // Initialise shift registers
  initShiftRegisters();

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
        //Check for new pattern requests
        patternCheck();
      }
    }
  
}


	

   
  


