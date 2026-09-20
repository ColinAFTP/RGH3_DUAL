#include <NativeEthernet.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

#include "constants.h"
#include "functions_comms.h"
#include "functions_web.h"
#include "variables_cpu1.h"

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
  // Status bit range is 101 - 130
  // Status bits are read only from the PLC.
  modbusServer.configureDiscreteInputs(101, 30);

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

// Look for a new Modbus TCP connection. Called every loop pass.
// accept() returns a connected client at once. available() would wait up to 10 s for the client's first bytes,
// freezing the whole loop for a client that connects and sends nothing.
// Only one Modbus client is served at a time. A PLC that loses power or its network cable never closes its connection, so the old
// connection would stay "connected" for ever and lock the new one out. So a new client replaces the old one if the old one is closed or has sent
// no request for MODBUS_IDLE_TAKEOVER_MS. An active client is never displaced: the new connection is closed instead (the client retries).
void ethernetConnect() {
  static uint32_t lastNoClientMsg = 0;            // timestamp of last "no client" message
  const uint32_t interval = 5000;                 // 5 seconds

  EthernetClient newClient = ethernetServer.accept();
  if (newClient.connected()) {
    bool oldConnected = ethernetClient.connected();
    if (!oldConnected || millis() - lastModbusRequestMs > MODBUS_IDLE_TAKEOVER_MS) {
      if (oldConnected) {
        ethernetClient.stop();                    // Drop the dead or silent old connection
        logEvent("PLC reconnected: the old connection was silent and has been dropped");
      }
      ethernetClient = newClient;                 // Store globally
      modbusServer.accept(ethernetClient);
      lastModbusRequestMs = millis();
      Serial.println("Ethernet client connected");
    } else {
      newClient.stop();                           // The current PLC connection is active: refuse the newcomer
      static uint32_t lastRefusedLog = 0;
      if (millis() - lastRefusedLog > 5000) {     // Not more than one log line every 5 s
        lastRefusedLog = millis();
        logEvent("Modbus connection refused: another client is active");
      }
    }
  } else if (!ethernetClient.connected()) {
    // No client connected: print a message only every 5 seconds
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
  if (patternSelection < 0 or patternSelection > NUM_PATTERNS) {          // 0 = home, 1 to NUM_PATTERNS = gap patterns
    patternSelection = patternSelectionPrevious;
    Serial.println();
    Serial.println("Error: Pattern selection is out of bounds!");
    // Tell the PLC: Move Refused on, with the reason. The next valid pattern change clears it.
    cpu1Refused = true;
    cpu1RefusedReason = EVT_REASON_BAD_PATTERN;
    logEvent("PLC pattern selection out of range - refused");
    modbusServer.holdingRegisterWrite(ADDR_PATTERN, patternSelectionPrevious);
  }
}

void relayCheck() {
  // Check for relay control updates from the PLC
  // The register is 16 bits and so is relayData, so every value is valid
  relayData = modbusServer.holdingRegisterRead(ADDR_RELAYS);
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

// Reload all the gap patterns from the PLC's holding registers. This runs every 5 s, on start up and whenever the PLC
// changes the pattern selection, so the update coil (ADDR_GAP_UPDATE) is no longer needed. It is still cleared here
// so a PLC that sets it sees it acknowledged.
void patternUpdateCheck() {
    modbusServer.coilWrite(ADDR_GAP_UPDATE, 0);

    // Read into a staging buffer first: the I2C request handler (interrupt) copies gapArrays,
    // so gapArrays must only ever be changed in one quick block.
    static float staging[NUM_PATTERNS][STRIDE_GAPS];
    for (int p = 0; p < NUM_PATTERNS; p++) {
        int baseAddress = ADDR_PATTERN_0_0 + (p * STRIDE_GAPS);
        for (int c = 0; c < STRIDE_GAPS; c++) {
            staging[p][c] = modbusServer.holdingRegisterRead(baseAddress + c);
            if (DEBUG_GAP_UPDATE) {
                Serial.print("P");
                Serial.print(p);
                Serial.print(" G");
                Serial.print(c);
                Serial.print(": ");
                Serial.println(staging[p][c]);
            }
        }
    }
    noInterrupts();
    memcpy(gapArrays, staging, sizeof(gapArrays));
    interrupts();
}