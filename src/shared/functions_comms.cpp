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
  // Holding register range is 101 - 150
  // Holding registers are read/write from the PLC.
  modbusServer.configureHoldingRegisters(101, 50);
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

void patternCheck() {
  // Check which pattern selection must go to the stepper motors
  patternSelection = modbusServer.holdingRegisterRead(ADDR_PATTERN);
  if (patternSelection < 0 or patternSelection >= NUM_PATTERNS) {
    patternSelection = 0;
    Serial.println();
    Serial.println("Error: Pattern selection is out of bounds!");
    modbusServer.holdingRegisterWrite(ADDR_PATTERN, patternSelectionPrevious);
  }
}
