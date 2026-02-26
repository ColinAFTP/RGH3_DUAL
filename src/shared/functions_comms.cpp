#include <NativeEthernet.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

#include "constants.h"
#include "functions_comms.h"
#include "variables.h"

// Create the Modbus IP object
byte mac1[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1 }; 
byte mac2[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE2 };
byte mac3[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE3 };
byte mac4[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE4 };
byte mac5[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE5 };
byte mac6[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE6 };
byte mac7[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE7 };
byte mac8[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE8 }; 
IPAddress ip1(192, 168, 2, 51);
IPAddress ip2(192, 168, 2, 52);
IPAddress ip3(192, 168, 2, 53);
IPAddress ip4(192, 168, 2, 54);
IPAddress ip5(192, 168, 2, 55);
IPAddress ip6(192, 168, 2, 56);
IPAddress ip7(192, 168, 2, 57);
IPAddress ip8(192, 168, 2, 58);
IPAddress ips[8] = {ip1, ip2, ip3, ip4, ip5, ip6, ip7, ip8};

// This subroutine sets up all the Modbus coils, registers, inputs, etc.
void modbusSetup() {

  // Set up the inputs from the DIP switches
  pinMode(DIP_SW1, INPUT);
  pinMode(DIP_SW2, INPUT);
  
  int dipSelection = digitalRead(DIP_SW2) * 2 + digitalRead(DIP_SW1);
  Serial.print("IP address selected: ");
  Serial.println(dipSelection);

  // Start the Ethernet connection and the server:
  switch (dipSelection) {
    case 0:
      Ethernet.begin(mac1, ip1);
      Serial.println("IP address: 192.168.2.51");
      break;
    case 1:
      Ethernet.begin(mac2, ip2);
      Serial.println("IP address: 192.168.2.52");
      break;
    case 2:
      Ethernet.begin(mac3, ip3);
      Serial.println("IP address: 192.168.2.53");
      break;
    case 3:
      Ethernet.begin(mac4, ip4);
      Serial.println("IP address: 192.168.2.54");
      break;
  }
  
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No Ethernet connection found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Start the server
  ethernetServer.begin();
  
  // Start the Modbus TCP server
  if (!modbusServer.begin()) {
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }

  // Set up input status bits. 
  // Status bit range is 101 - 120
  // Status bits are read only from the PLC.
  modbusServer.configureDiscreteInputs(101, 20);

  // Set up coils. 
  // Coil range is 101 - 120
  // Coils are read/write from the PLC.
  modbusServer.configureCoils(101, 20);

  // Set up input registers
  // Input register range is 101 - 110
  // Input registers are read only from the PLC.
  modbusServer.configureInputRegisters(101, 10);
  
  // Set up holding registers
  // Holding register range is 101 - 160
  // Holding registers are read/write from the PLC.
  modbusServer.configureHoldingRegisters(101, 60);
}

void ethernetConnect() {
  static uint32_t lastNoClientMsg = 0;            // timestamp of last "no client" message
  const uint32_t interval = 5000;                 // 5 seconds

  ethernetClient = ethernetServer.available();    // <— store globally
  if (ethernetClient.connected()) {
    Serial.println("Ethernet client connected");
    modbusServer.accept(ethernetClient);
  } else {
    // No client connected — print message only every 5 seconds
    uint32_t now = millis();
    if (now - lastNoClientMsg >= interval) {
        Serial.println("No client connected yet");
        lastNoClientMsg = now;
    }    
  }
}

void updateInputs() {
  // Save input data to holding register 3
  modbusServer.holdingRegisterWrite(ADDR_INPUTS, inputData);
}

void patternCheck() {
  // Check which pattern selection must go to the stepper motors
  patternSelection = modbusServer.holdingRegisterRead(ADDR_PATTERN);
  if (patternSelection < 0 or patternSelection >= NUM_PATTERNS) {
    patternSelection = patternSelectionPrevious;
    Serial.println();
    Serial.println("Error: Pattern selection is out of bounds!");
    modbusServer.holdingRegisterWrite(ADDR_PATTERN, patternSelectionPrevious);
  }
}

void relayCheck() {
  // Check for relay control updates from the PLC
  relayData = modbusServer.holdingRegisterRead(ADDR_RELAYS);
  if (relayData < 0 or relayData > 0xFFFF) {
    relayData = relayDataPrevious;
    Serial.println();
    Serial.println("Error: Relay data is out of bounds!");
    modbusServer.holdingRegisterWrite(ADDR_RELAYS, relayDataPrevious);}
}

void updateTicker(word tickerData) {
  // Update the ticker value in the holding register
  modbusServer.holdingRegisterWrite(ADDR_TICKER, tickerData);
  // Write the ticker value to the serial output every 10th count
  if (tickerData % 10 == 0) {
    Serial.print("   | New ticker value: ");
    Serial.println(tickerData / 10);
  }
}

void speedCheck() {
  // Check for speed data updates from the PLC
  speedData = modbusServer.holdingRegisterRead(ADDR_SPEED);
  if (speedData < 0 or speedData > MAX_SPEED) {
    speedData = speedDataPrevious;
    Serial.println();
    Serial.println("Error: Speed data is out of bounds!");
    modbusServer.holdingRegisterWrite(ADDR_SPEED, speedDataPrevious);
  }
}

void patternUpdateCheck() {
  // This functions checks for pattern updates from the PLC
  bool patternUpdateFlag = modbusServer.coilRead(ADDR_GAP_UPDATE);  
  if ((patternUpdateFlag == true) ||(bootLoadGaps == true)) {
    Serial.println("Pattern update requested from PLC.");           
    // Clear the pattern update flag
    modbusServer.coilWrite(ADDR_GAP_UPDATE, 0);
    // Update the pattern 0 gaps
    Serial.println("Updating pattern 0 gaps.");
    for (int c = 0; c < NUM_GAPS; c++) {
      gapArrays[0][c] = modbusServer.holdingRegisterRead(ADDR_PATTERN_0_0 + c);
      Serial.print("New pattern 0 gap #");
      Serial.print(c + 1);
      Serial.print(": ");
      Serial.println(gapArrays[0][c]);
    }

  }
}