#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_comms.h"
#include "functions_i2c.h"
#include "functions_io.h"
#include "functions_manual_cpu1.h"
#include "functions_web.h"
#include "functions_watchdog.h"
#include "variables_cpu1.h"

// Set up local variables
static uint32_t inputUpdateTime;
static uint32_t dataUpdateTime;
static uint32_t pulseStartTime;
static bool pulseActive = false;
static uint32_t faultPulseStartTime;
static bool faultPulseActive = false;

// Called (in interrupt context) when CPU2 sends data to CPU1. The first byte is the command (see the I2C_CMD_ constants).
void receiveCommand(int howMany) {
    if (Wire2.available()) {
        i2cCommand = Wire2.read();
        if (i2cCommand == I2C_CMD_FAULT_MASK && Wire2.available() >= 3) {
            uint16_t mask = Wire2.read();
            mask |= (uint16_t)Wire2.read() << 8;
            faultMaskRx = mask;
            faultTypeRx = Wire2.read();
            faultMaskNew = true;         // The main loop copies it to the Modbus register
        } else if (i2cCommand == I2C_CMD_STATUS && Wire2.available() >= (int)sizeof(StatusPacket)) {
            // Status from CPU2 for the web page. It goes into a small ring that the main loop empties.
            uint8_t next = (statusRingHead + 1) % STATUS_RING_SIZE;
            if (next != statusRingTail) {
                uint8_t* dest = (uint8_t*)&statusRing[statusRingHead];
                for (size_t i = 0; i < sizeof(StatusPacket); i++) dest[i] = Wire2.read();
                statusRingHead = next;
            }
        }
    }
}

void setup()
{
  bool watchdogReset = watchdogCausedReset();   // Read this first: it says why the CPU started

  Serial.begin(9600);                // Start serial for output
  // Wait for a USB host for up to 3 s only, so the controller still starts when running stand-alone
  while (!Serial && millis() < 3000) {
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
  manualInit();                       // Reads the manual DIP switch and puts it on the line to CPU2 before CPU2 starts

  // Indicate that gap patterns should be loaded from the PLC on startup
  bootLoadGaps = true;

  // Set up the Modbus server and address locations
  modbusSetup();

  // Start the diagnostics web server
  webSetup();
  if (watchdogReset) {
    logEvent("CPU1 was restarted by the WATCHDOG (it had hung)");
  }

  // Clear the relay outputs
  relayControl(0);

  // Get the start time
  tickerTime = millis();

  // Initialise local variables
  inputUpdateTime = millis();
  dataUpdateTime = millis();
  pulseStartTime = millis();

  // Read the inputs once, then start the I2C slave. CPU2 gets no answer until CPU1 is fully up,
  // so it can never read inputs that have not been sampled yet (it homes at power up from them).
  inputsCheck();
  Wire2.begin(0x40);                  // Join I2C bus as slave with address 0x40
  Wire2.onReceive(receiveCommand);
  Wire2.onRequest(onI2CRequest);
  Wire2.setClock(1000000);

  // Every long wait in setup() is over: from now on the watchdog resets the CPU if the main loop stops
  watchdogStart();

}

void loop()
{

  watchdogFeed();
  if (DEBUG_HANG_TEST_CPU1_S > 0 && millis() > (uint32_t)DEBUG_HANG_TEST_CPU1_S * 1000UL) {
    while (true) {}                     // TEST ONLY: hang, to check that the watchdog resets the CPU
  }

  // Measure how long each pass of the loop takes (shown on the web page)
  webLoopTick();

  // Poll for Modbus TCP requests (server stays active even if client drops)
  if (modbusServer.poll()) {
    lastModbusRequestMs = millis();
  }

  // Look for a new Modbus connection every pass. It replaces a dead or silent client (a PLC that lost power or its cable never
  // closes its connection), but never an active one.
  ethernetConnect();

  if (ethernetClient.connected()) {

    // Check for new pattern gap data from the PLC
    if ((millis() - dataUpdateTime >= 5000) || bootLoadGaps) {  // Check every 5 s or on boot load
      patternUpdateCheck();
      dataUpdateTime = millis();
      bootLoadGaps = false;
    }
  }

  // Serve the diagnostics web page (non-blocking) and decode the status that CPU2 sent
  webService();
  cpu2StatusService();

  // Log when CPU2 stops or starts reporting
  static bool cpu2WasOnline = false;
  bool cpu2IsOnline = cpu2Online();
  if (cpu2IsOnline != cpu2WasOnline) {
    logEvent(cpu2IsOnline ? "CPU2 online" : "CPU2 OFFLINE: no status received");
    cpu2WasOnline = cpu2IsOnline;
  }

  // Log when the PLC connects or disconnects
  static bool plcWasConnected = false;
  bool plcConnected = ethernetClient.connected();
  if (plcConnected != plcWasConnected) {
    if (!plcConnected) {
      logEvent("Modbus client %s disconnected", plcRemoteText);   // The connect is logged, with its address, in ethernetConnect()
      // Give the socket back and forget it. The stack reuses sockets: if this object kept pointing at the old socket number, the next connection
      // to land on it (a web page request, for example) would look like a Modbus client, and the Modbus code would read the web request.
      ethernetClient.stop();
      ethernetClient = EthernetClient();
    }
    plcWasConnected = plcConnected;
  }

  // Manual mode: the DIP switch, the PLC request, the jog commands for CPU2, coil 101 and DI 119
  manualCpu1Service();

  // Check for new pattern requests
  patternCheck();
  if (patternSelection != patternSelectionPrevious && manualModeOn()) {
    // Manual mode is on: the request is refused and forgotten. After manual mode CPU2 homes by itself, and the PLC sends a new request.
    cpu1Refused = true;
    cpu1RefusedReason = EVT_REASON_MANUAL;
    logEvent("PLC pattern request %d refused: manual mode is on", patternSelection);
    patternSelectionPrevious = patternSelection;
  }
  if (patternSelection != patternSelectionPrevious) {

    Serial.print("   | Current pattern: ");
    Serial.println(patternSelection);
    logEvent(patternSelection == 0 ? "PLC selected pattern 0 (home)" : "PLC selected pattern %d", patternSelection);

    // A new valid request: At Target goes off now (until CPU2 has dropped its own line), and an old refusal is cleared
    atTargetBlank = true;
    atTargetBlankStart = millis();
    cpu1Refused = false;

    // Refresh the gap data and speed from the PLC now, so CPU2 always moves with the latest values
    patternUpdateCheck();
    speedCheck();
    if (speedData != speedDataPrevious) {
      stepperSpeed = speedData;
      speedDataPrevious = speedData;
    }

    // Start 500 ms pulse on the new move output pin
    digitalWrite(OUTPUT_A1, HIGH);
    pulseStartTime = millis();
    pulseActive = true;

    patternSelectionPrevious = patternSelection;
  }

  // Relays: the PLC controls them through register ADDR_RELAYS. Nothing else does, except the relay test (coil ADDR_RELAY_TEST).
  relayCheck();
  relayTestService();

  // Check the feedback signals from CPU2 (Home, At Target, Fault) and update the status bits
  feedbackCheck();
  static bool prevHome = false, prevTarget = false, prevFault = false;
  if (statusHome != prevHome) { logEvent("Home %s", statusHome ? "ON" : "OFF"); prevHome = statusHome; }
  if (statusAtTarget != prevTarget) { logEvent("At target %s", statusAtTarget ? "ON" : "OFF"); prevTarget = statusAtTarget; }
  if (statusFault != prevFault) { logEvent("Fault %s", statusFault ? "ON" : "cleared"); prevFault = statusFault; }

  if (!relayTestActive && relayData != relayDataPrevious) {
    Serial.print("   | New relay data: ");
    Serial.println(relayData, BIN);
    relayControl(relayData);
    relayOutputShown = relayData;
    relayDataPrevious = relayData;
  }

  // Check for speed updates
  speedCheck();
  if (speedData != speedDataPrevious) {
    Serial.print("   | New speed data: ");
    Serial.println(speedData);
    logEvent("PLC speed set to %d", speedData);
    stepperSpeed = speedData;
    speedDataPrevious = speedData;
  }

  // Check for new input updates every millisecond. CPU2 reads these over I2C while homing and stops the
  // spreaders the moment a home sensor goes on, so fast sampling keeps the overtravel small.
  if (millis() - inputUpdateTime >= 1) {
    inputsCheck();
    logInputChanges();
    inputUpdateTime = millis();
  }
  if (inputData != inputDataPrevious) {
    Serial.print("   | New input data: ");
    Serial.println(inputData, BIN);
    updateInputs();
    inputDataPrevious = inputData;
  }


  // Over travel sensors (proxy 1 and 11): raise the line to CPU2 while one is on. CPU2 stops the motion and raises a fault.
  if (OVERTRAVEL_ENABLED) {
    static bool overTravelPrev = false;
    bool overTravel = (inputData & OVERTRAVEL_MASK) != 0;
    if (overTravel != overTravelPrev) {
      digitalWrite(OUTPUT_A4, overTravel ? HIGH : LOW);
      if (overTravel) {
        logEvent("OVER TRAVEL sensor on: %s", (inputData & (1 << OVERTRAVEL_LEFT_BIT)) ? ((inputData & (1 << OVERTRAVEL_RIGHT_BIT)) ? "left and right" : "left (spreader 1)") : "right (spreader 10)");
      } else {
        logEvent("Over travel sensors off");
      }
      overTravelPrev = overTravel;
    }
  }
  // Handle pulse timing (non-blocking)
  if (pulseActive && (millis() - pulseStartTime >= 500)) {
      digitalWrite(OUTPUT_A1, LOW);
      pulseActive = false;
  }

  // Copy the failed spreader bitmask that CPU2 sent over I2C to the Modbus register
  if (faultMaskNew) {
    faultMaskNew = false;
    modbusServer.holdingRegisterWrite(ADDR_FAULT_SPREADERS, faultMaskRx);
  }

  // Fault reset requested by the PLC: clear the coil and give CPU2 a 100 ms pulse on the fault reset line
  if (modbusServer.coilRead(ADDR_FAULT_RESET)) {
    modbusServer.coilWrite(ADDR_FAULT_RESET, 0);
    Serial.println("   | Fault reset requested by PLC");
    logEvent("PLC requested a fault reset");
    digitalWrite(OUTPUT_A3, HIGH);
    faultPulseStartTime = millis();
    faultPulseActive = true;
  }
  if (faultPulseActive && (millis() - faultPulseStartTime >= 100)) {
    digitalWrite(OUTPUT_A3, LOW);
    faultPulseActive = false;
  }

  // Update the ticker every second
  if (millis() - tickerTime > 1000) {
    secondTicker++;
    tickerTime = millis();
    updateTicker(secondTicker);
  }


}


	

   
  


