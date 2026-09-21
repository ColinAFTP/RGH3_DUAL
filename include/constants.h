#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// Modbus holding address constants
constexpr int ADDR_TICKER = 101;
constexpr int ADDR_HOME_COUNTS = 102;
constexpr int ADDR_INPUTS = 103;
constexpr int ADDR_PATTERN = 104;             // PLC selects the gripper position: 0 = go HOME, 1 to NUM_PATTERNS = gap pattern n (register block n-1 below)
constexpr int ADDR_SPEED = 105;
constexpr int ADDR_RELAYS = 106;              // The 16 relays. Bit 0 is relay 1 ... bit 15 is relay 16. Controlled by the PLC only: nothing in the controller writes to the relays (except the relay test)
constexpr int ADDR_MANUAL_PTR = 107;          // Manual mode: the spreader to move, as its physical number 1 to 10 (5 is the static spreader and is refused)
constexpr int ADDR_FAULT_SPREADERS = 108;     // Bitmask of the spreaders that failed to home. Bit (n-1) is spreader n (spreader 5 is static so bit 4 is never set). Read only from the PLC. Cleared by a fault reset.
constexpr int ADDR_FAULT_TYPE = 109;          // Fault type (FAULT_NONE, FAULT_HOMING, FAULT_OVERTRAVEL or FAULT_CPU2). For FAULT_OVERTRAVEL the bit in ADDR_FAULT_SPREADERS is spreader 1 (left) or 10 (right). Read only from the PLC
constexpr int ADDR_REFUSED_REASON = 110;      // Why the last request was refused (EVT_REASON_ constants, 0 = not refused). Valid while the Move Refused status bit is on. Read only from the PLC

// Pattern 1 gaps
constexpr int ADDR_PATTERN_0_0 = 111;
constexpr int ADDR_PATTERN_0_1 = 112;
constexpr int ADDR_PATTERN_0_2 = 113;
constexpr int ADDR_PATTERN_0_3 = 114;
constexpr int ADDR_PATTERN_0_4 = 115;
constexpr int ADDR_PATTERN_0_5 = 116;
constexpr int ADDR_PATTERN_0_6 = 117;
constexpr int ADDR_PATTERN_0_7 = 118;
constexpr int ADDR_PATTERN_0_8 = 119;
constexpr int ADDR_PATTERN_0_9 = 120;
// Pattern 2 gaps
constexpr int ADDR_PATTERN_1_0 = 121;
constexpr int ADDR_PATTERN_1_1 = 122;
constexpr int ADDR_PATTERN_1_2 = 123;
constexpr int ADDR_PATTERN_1_3 = 124;
constexpr int ADDR_PATTERN_1_4 = 125;
constexpr int ADDR_PATTERN_1_5 = 126;
constexpr int ADDR_PATTERN_1_6 = 127;
constexpr int ADDR_PATTERN_1_7 = 128;
constexpr int ADDR_PATTERN_1_8 = 129;
constexpr int ADDR_PATTERN_1_9 = 130;
// Pattern 3 gaps
constexpr int ADDR_PATTERN_2_0 = 131;
constexpr int ADDR_PATTERN_2_1 = 132;
constexpr int ADDR_PATTERN_2_2 = 133;
constexpr int ADDR_PATTERN_2_3 = 134;
constexpr int ADDR_PATTERN_2_4 = 135;
constexpr int ADDR_PATTERN_2_5 = 136;
constexpr int ADDR_PATTERN_2_6 = 137;
constexpr int ADDR_PATTERN_2_7 = 138;
constexpr int ADDR_PATTERN_2_8 = 139;
constexpr int ADDR_PATTERN_2_9 = 140;
// Pattern 4 gaps
constexpr int ADDR_PATTERN_3_0 = 141;
constexpr int ADDR_PATTERN_3_1 = 142;
constexpr int ADDR_PATTERN_3_2 = 143;
constexpr int ADDR_PATTERN_3_3 = 144;
constexpr int ADDR_PATTERN_3_4 = 145;
constexpr int ADDR_PATTERN_3_5 = 146;
constexpr int ADDR_PATTERN_3_6 = 147;
constexpr int ADDR_PATTERN_3_7 = 148;
constexpr int ADDR_PATTERN_3_8 = 149;
constexpr int ADDR_PATTERN_3_9 = 150;
// Pattern 5 gaps
constexpr int ADDR_PATTERN_4_0 = 151;
constexpr int ADDR_PATTERN_4_1 = 152;
constexpr int ADDR_PATTERN_4_2 = 153;
constexpr int ADDR_PATTERN_4_3 = 154;
constexpr int ADDR_PATTERN_4_4 = 155;
constexpr int ADDR_PATTERN_4_5 = 156;
constexpr int ADDR_PATTERN_4_6 = 157;
constexpr int ADDR_PATTERN_4_7 = 158;
constexpr int ADDR_PATTERN_4_8 = 159;
constexpr int ADDR_PATTERN_4_9 = 160;

// Modbus coil address constant
const int ADDR_HOMING = 101;            // Homing in progress. Written by CPU1 (1 while CPU2 is homing in any stage, including power up and fault reset homing), read by the PLC. Not a request
const int ADDR_GAP_UPDATE = 102;        // Update all the gap patterns
const int ADDR_RELAY_TEST = 103;        // Relay test: while the PLC keeps this on, the relays switch on one at a time, relay 1 to 16, round and round (RELAY_TEST_STEP_MS each). The relay bits are ignored during the test
const int ADDR_MANUAL = 104;            // Manual mode request from the PLC. Works together with the manual DIP switch (either one is enough, the DIP switch overrides the PLC)
const int ADDR_MANUAL_OPN = 105;        // Manual mode: move the spreader in ADDR_MANUAL_PTR forward (open) while this coil is on
const int ADDR_MANUAL_CLS = 106;        // Manual mode: move the spreader in ADDR_MANUAL_PTR backward (close) while this coil is on
const int ADDR_FAULT_RESET = 107;       // PLC sets this to reset a fault. CPU1 clears it again and pulses OUTPUT_A3 so CPU2 clears its fault and starts a search home

// Modbus status address constants (discrete inputs, read only from the PLC)
const int ADDR_HOME = 117;              // Gripper is home: all nine home proximity sensors (proxy 2 to 10) are on and there is no fault. Calculated live by CPU1
const int ADDR_MOVE_DONE = 118;         // At target: the requested pattern move or homing has finished. Off while a move or homing is running
const int ADDR_MANUAL_MODE = 119;       // Manual mode is active (CPU2 has stopped, and takes jog commands). Also reported on the web page in orange
const int ADDR_HOMING_FAULT = 120;      // Fault is active (a spreader failed to home, or an over travel sensor stopped the motion). ADDR_FAULT_TYPE says which, ADDR_FAULT_SPREADERS which spreaders. Cleared with ADDR_FAULT_RESET
const int ADDR_MOVE_REFUSED = 121;      // The last request (pattern selection) was refused, see ADDR_REFUSED_REASON. Cleared by the next valid pattern change. At Target stays off for a refused request
const int ADDR_CPU2_ONLINE = 122;       // CPU2 (the motion controller) is running and reporting to CPU1. If it stops, Fault (ADDR_HOMING_FAULT) turns on with fault type FAULT_CPU2

// General constants
constexpr uint32_t RELAY_TEST_STEP_MS = 500;      // Relay test: how long each relay stays on
constexpr bool DEBUG_STEPPER_CALC = false;  // Print stepper target calculations
constexpr bool DEBUG_PLOT = false;       // Print stepper positions in Serial Plotter format (CPU2)
constexpr bool DEBUG_GAP_UPDATE = false;    // Print every gap value read from the PLC (CPU1)
const int DEBOUNCE_DELAY = 20;          // 20 ms debounce delay for inputs

// Stepper movement control constants
constexpr int INIT_SPEED = 5000;            // 15,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int MAX_SPEED = 14000;
constexpr float INIT_ACCEL = 20000;         // 40,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int MAX_STEPS = 16000;            // 16,000 is the maximum amount of steps for the rack length
constexpr float STEPS_PER_MM = 70.73553;    // Racks & pinions are mod 2, so tooth pitch is 2xPi mm. Pinions have 18 teeth. Distance per revolution is 113.0973 mm. There are 8000 steps per revolution, so distance per step is 0.014137 mm

// Pattern constants
constexpr int NUM_PATTERNS = 5;             // Number of gap patterns. The PLC selects them with register 104 = 1 to NUM_PATTERNS (0 = home, which needs no gap data)
constexpr int NUM_GAPS = 9;                 // Number of gaps actually used in the current application. There are 10 spreaders, so 9 gaps and 9 steppers
constexpr int STRIDE_GAPS = 10;             // Fixed Modbus register spacing per pattern

// Spreader layout. There are 10 spreaders numbered 1 to 10 from left to right (looking from the control box side).
// Spreader 5 is static. Stepper index 0 to 8 drives spreaders 1,2,3,4,6,7,8,9,10 in that order.
// Gap n (index 0 to 8) is the gap between spreader n+1 and n+2, so gap 3 is spreader 4 to the static spreader 5 and gap 4 is the static spreader 5 to spreader 6.
constexpr int NUM_LEFT_SPREADERS = 4;       // Spreaders 1 to 4 are left of the static spreader (stepper index 0 to 3, gaps 0 to 3)
constexpr int NUM_RIGHT_SPREADERS = 5;      // Spreaders 6 to 10 are right of the static spreader (stepper index 4 to 8, gaps 4 to 8)
static_assert(NUM_LEFT_SPREADERS + NUM_RIGHT_SPREADERS == NUM_GAPS, "Spreader layout must add up to NUM_GAPS");

// Over travel sensors. Proxy 1 (left, triggered by spreader 1) and proxy 11 (right, triggered by spreader 10) are on input bits 0 and 10.
// When one turns on, CPU1 raises the OUTPUT_A4 line to CPU2, which stops the TeensyStep motion at once and raises a fault
// (fault type FAULT_OVERTRAVEL, register ADDR_FAULT_SPREADERS says which spreader). The direct pulse homing stage ignores the line,
// because homing closes the spreaders and so moves them away from the sensors. Recovery: fault reset (coil ADDR_FAULT_RESET) starts a search home.
// KEEP THIS false UNTIL THE TWO INPUTS ARE WIRED: a floating input would cause a fault at random. The sensors are normally open like the others.
constexpr bool OVERTRAVEL_ENABLED = false;
constexpr int OVERTRAVEL_LEFT_BIT = 0;                  // Input bit of proxy 1
constexpr int OVERTRAVEL_RIGHT_BIT = 10;                // Input bit of proxy 11
constexpr uint16_t OVERTRAVEL_MASK = 0x0401;            // Input bits 0 and 10

// Fault types, reported in register ADDR_FAULT_TYPE
constexpr uint8_t FAULT_NONE = 0;
constexpr uint8_t FAULT_HOMING = 1;                     // A spreader failed to home
constexpr uint8_t FAULT_OVERTRAVEL = 2;                 // An over travel sensor turned on
constexpr uint8_t FAULT_CPU2 = 3;                       // CPU2 stopped reporting to CPU1 (crashed, hung or not powered). Clears by itself when CPU2 is back

// Input filter (CPU1). Each input must hold its new value for this many consecutive 1 ms samples before it counts. Removes single-sample noise
// pulses (seen on the bench) at the cost of this many milliseconds of latency: about 0.02 mm per sample at the 1500 steps/s homing rate.
// Set to 1 for no filtering.
constexpr int INPUT_FILTER_SAMPLES = 3;

// Inputs whose changes are written to the web event log and counted as glitches. Proxy 1 and 11 (the over travel sensors, input bits 0 and 10)
// are left out while they are not wired (OVERTRAVEL_ENABLED false): floating inputs pick up noise all the time.
constexpr uint16_t INPUT_LOG_MASK = OVERTRAVEL_ENABLED ? 0x07FF : 0x03FE;   // 0x03FE = input bits 1 to 9 = proxy 2 to 10, the nine home sensors. Bits 0 and 10 are added when OVERTRAVEL_ENABLED

// Home proximity sensors. They are wired to the CPU1 input shift registers (input bit = proxy number - 1).
// Proxy 1 (bit 0) and proxy 11 (bit 10) are the left and right over travel sensors (see OVERTRAVEL_ENABLED).
// Proxy 2 to 5 are the home sensors of spreaders 1 to 4 and proxy 6 to 10 those of spreaders 6 to 10, so the sensor of stepper index i is on input bit PROXY_FIRST_BIT + i.
// A sensor is on when its spreader is closed up against its neighbour on the side of the static spreader.
constexpr int PROXY_FIRST_BIT = 1;
constexpr uint16_t PROXY_ALL_MASK = 0x03FE; // Input bits 1 to 9: all nine home sensors

// Homing constants (CPU2). These are meant to be tuned on the real gripper.
// A normal home moves every stepper with TeensyStep to HOME_APPROACH_MM from home, then the direct pulse routine closes
// the remaining distance and stops each stepper the moment its proximity sensor goes on. Set HOME_APPROACH_MM to 0 to let
// TeensyStep move all the way to position 0 (no direct pulse stage) if the hybrid homing turns out to be too slow.
// After a power up without all sensors on, or after a fault, the direct pulse routine runs on its own (search home).
constexpr float HOME_APPROACH_MM = 5.0;                 // Distance from home at which TeensyStep hands over to the direct pulse routine
constexpr long HOME_APPROACH_STEPS = (long)(HOME_APPROACH_MM * STEPS_PER_MM + 0.5f);
constexpr int HOME_PULSE_RATE = 1500;                   // Direct pulse homing speed in steps/s
constexpr int HOME_START_RATE = 400;                    // Direct pulse speed when a stepper starts moving, in steps/s
constexpr int HOME_RAMP_MS = 80;                        // Time to ramp from HOME_START_RATE to HOME_PULSE_RATE. Stopping is always instant
constexpr int HOME_TICK_US = 20;                        // Direct pulse timer period. A step pulse is one tick wide, so keep HOME_PULSE_RATE well below 1 / (2 x tick)
constexpr int HOME_MAX_STEPS = MAX_STEPS + 500;         // A stepper that emits more than this many pulses without its sensor going on has failed to home
constexpr uint32_t HOME_TIMEOUT_MS = 30000;             // Overall time limit for the direct pulse routine
constexpr int HOME_IO_FAIL_LIMIT = 50;                  // Consecutive failed I2C reads of the sensors before homing is aborted with a fault. Pulses stop at the first failed read

// Watchdog and CPU2 link supervision
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 2000;          // Both CPUs reset themselves if their main loop does not run for this long (multiple of 500 ms)
constexpr uint32_t MODBUS_IDLE_TAKEOVER_MS = 3000;       // A new Modbus client replaces the current one if that has sent no request for this long (PLC lost power or cable). An active client is never replaced
constexpr uint32_t CPU2_TIMEOUT_MS = 1500;              // CPU1 declares CPU2 lost (Fault, type FAULT_CPU2) if no status arrives for this long
constexpr uint32_t CPU2_BOOT_GRACE_MS = 40000;          // ...but after a power up CPU2 gets this long to send its first status (it waits 5 s for the drivers and up to 20 s for CPU1)
constexpr uint32_t HOME_SENSOR_TIMEOUT_US = 20000;      // The direct pulse interrupt stops all pulses if the last good sensor read is older than this
constexpr int DEBUG_HANG_TEST_CPU1_S = 0;               // TEST ONLY. 0 = off. Otherwise CPU1 hangs in its main loop after this many seconds, to test its watchdog. Keep 0
constexpr int DEBUG_HANG_TEST_CPU2_S = 0;               // TEST ONLY. The same for CPU2. Keep 0
constexpr bool DEBUG_STALL_TEST = false;                // TEST ONLY. CPU2 delays its homing loop by 40 ms once a second, to test the pulse timeout above. Keep false

// I2C commands from CPU2 (master) to CPU1 (slave, address 0x40)
constexpr uint8_t I2C_CMD_IO = 1;                       // Request: 16 bit input word (uint16_t)
constexpr uint8_t I2C_CMD_GAPS = 2;                     // Request: all gap patterns and the stepper speed (PatternPacket)
constexpr uint8_t I2C_CMD_PATTERN = 3;                  // Request: pattern selection (int)
constexpr uint8_t I2C_CMD_FAULT_MASK = 4;               // Write: followed by the 16 bit failed spreader bitmask (low byte first) and the fault type byte. CPU1 copies them to ADDR_FAULT_SPREADERS and ADDR_FAULT_TYPE

// CPU2 status and event reporting to CPU1 (for the CPU1 web page)
constexpr uint8_t I2C_CMD_STATUS = 5;                   // Write: a StatusPacket (see structures.h)
constexpr int STATUS_MAX_EVENTS = 6;                    // Events carried by one status packet
constexpr uint32_t STATUS_PERIOD_MS = 250;              // CPU2 sends its status this often. Not sent while the direct pulse homing stage is polling the sensors

// CPU2 states reported in StatusPacket.state
constexpr uint8_t STATE_IDLE = 0;                       // Standing still, positions known
constexpr uint8_t STATE_MOVING = 1;                     // TeensyStep pattern move
constexpr uint8_t STATE_HOMING_APPROACH = 2;            // Homing: TeensyStep approach move
constexpr uint8_t STATE_HOMING_PULSES = 3;              // Homing: direct pulse stage
constexpr uint8_t STATE_FAULT = 4;                      // Homing fault active
constexpr uint8_t STATE_UNKNOWN_POS = 5;                // Standing still but positions unknown (not homed yet)

// Event codes sent by CPU2 (StatusEvent.code) and the meaning of the argument
constexpr uint8_t EVT_BOOT = 1;                          // CPU2 started. Arg = 1 if the reset was caused by the watchdog, 0 otherwise
constexpr uint8_t EVT_POWERUP_HOME = 2;                 // Power up with all home sensors on
constexpr uint8_t EVT_POWERUP_SEARCH = 3;               // Power up without all home sensors on, search home started
constexpr uint8_t EVT_TRIGGER = 4;                      // Start move trigger seen. Arg = pattern
constexpr uint8_t EVT_MOVE_START = 5;                   // Pattern move started. Arg = pattern
constexpr uint8_t EVT_MOVE_DONE = 6;                    // Pattern move (or TeensyStep only home) finished
constexpr uint8_t EVT_HOME_START = 7;                   // Homing started. Arg: 0 = TeensyStep only, 1 = approach then pulses, 2 = search (pulses only)
constexpr uint8_t EVT_HOME_PULSES = 8;                  // Direct pulse stage started
constexpr uint8_t EVT_HOME_DONE = 9;                    // Homing complete
constexpr uint8_t EVT_HOME_FAULT = 10;                  // Homing fault. Arg = failed spreader bitmask
constexpr uint8_t EVT_FAULT_RESET = 11;                 // Fault reset, search home started
constexpr uint8_t EVT_REFUSED = 12;                     // Request refused. Arg = EVT_REASON_ constant
constexpr uint8_t EVT_IO_FAIL = 13;                     // Home sensors could not be read over I2C, pulses stopped
constexpr uint8_t EVT_OVERTRAVEL = 14;                  // Over travel sensor stopped the motion. Arg = spreader bitmask (bit 0 = spreader 1, bit 9 = spreader 10)
constexpr uint8_t EVT_PULSE_TIMEOUT = 15;               // The direct pulse interrupt stopped the pulses because the sensor data was too old (main loop stalled)

constexpr int EVT_REASON_BUSY = 1;                      // Already moving or homing
constexpr int EVT_REASON_FAULT = 2;                     // Homing fault active
constexpr int EVT_REASON_UNKNOWN_POS = 3;               // Stepper positions unknown, home first
constexpr int EVT_REASON_BAD_PATTERN = 4;               // Invalid pattern received
constexpr int EVT_REASON_NO_GAPS = 5;                   // Could not refresh the gap data from CPU1
constexpr int EVT_REASON_BAD_TARGETS = 6;               // Targets outside the rack travel
constexpr int EVT_REASON_HOME_FAILED = 7;               // Home request failed
constexpr int EVT_REASON_MANUAL = 8;                    // Manual mode is on: pattern and home requests are refused

// CPU1 pin numbers
// ================
// Relay shift register pins
constexpr int RELAY_DATA_CLOCK_PIN = 2;       // SHCP
constexpr int RELAY_DATA_PIN = 3;             // DATA
constexpr int RELAY_DATA_LATCH_PIN = 4;       // STCP

// Input shift register pins
constexpr int INPUTS_DATA_PIN = 5;            // Q7
constexpr int INPUTS_DATA_LOAD_PIN = 6;       // PL
constexpr int INPUTS_DATA_CLOCK_PIN = 7;      // CP

// Hardwired IO signal pins between CPU1 and CPU2. A pins are CPU1 outputs (wired to the CPU2 INPUT_A pins), B pins are CPU2 outputs (wired to the CPU1 INPUT_B pins).
constexpr int OUTPUT_A1 = 33;                 // Start move: CPU1 holds this high for 500 ms when the PLC changes the pattern (register 104). CPU2 acts on the rising edge and reads the pattern over I2C. Pattern 0 means go home
constexpr int OUTPUT_A2 = 32;                 // Manual DIP switch: CPU1 holds this high while the manual mode DIP switch is on (see DIP_MANUAL_PIN)
constexpr int OUTPUT_A3 = 31;                 // Fault reset: CPU1 pulses this high for 100 ms when the PLC sets the fault reset coil. CPU2 clears its homing fault on the rising edge and starts a search home
constexpr int OUTPUT_A4 = 30;                 // Over travel: CPU1 holds this high while an over travel sensor (proxy 1 or 11) is on. CPU2 stops the motion and raises a fault. Only used when OVERTRAVEL_ENABLED
constexpr int INPUT_B1 = 29;                  // Reserved, not used. The Home signal is calculated by CPU1 from the home proximity sensors
constexpr int INPUT_B2 = 28;                  // At target: high when CPU2 has finished a move or homing, low while it is moving or homing
constexpr int INPUT_B3 = 27;                  // Fault: high while CPU2 has a fault (homing failed or over travel). The spreaders and fault type are sent over I2C (I2C_CMD_FAULT_MASK)
constexpr int INPUT_B4 = 26;                  // Request refused: high after CPU2 refused a request (reason in the status packet). Cleared when the next request arrives

// DIP switches (CPU1). The numbers are the labels printed on the PCB. A switch is ON when its pin is HIGH.
// The IP address number is DIP 2 x 2 + DIP 1: 0 = 192.168.2.51, 1 = .52, 2 = .53, 3 = .54. It is read once at power up.
constexpr int DIP_PCB1_PIN = 41;                    // PCB DIP 1: IP address, least significant bit
constexpr int DIP_PCB2_PIN = 40;                    // PCB DIP 2: IP address, most significant bit
constexpr int DIP_PCB3_PIN = 39;                    // PCB DIP 3: manual mode

// Manual mode (see functions_manual.cpp on CPU2). Manual mode is on while the manual DIP switch is on OR the PLC has set coil ADDR_MANUAL.
// The DIP switch overrides the PLC: it stops any motion at once, and the PLC cannot leave manual mode while the switch is on.
// In manual mode the PLC moves ONE spreader at a time: holding register ADDR_MANUAL_PTR = spreader number (1 to 10, not 5), coil ADDR_MANUAL_OPN
// moves it forward (open) and coil ADDR_MANUAL_CLS backward (close), for as long as the coil is on. Opening pushes along the spreaders further out that are touching it; closing
// pushes along the spreaders further in that it touches, up to the static spreader.
// Leaving manual mode starts an automatic home. Pattern requests are refused while manual mode is on.
constexpr int DIP_MANUAL_PIN = DIP_PCB3_PIN;              // The manual mode DIP switch (PCB DIP 3, CPU1)
constexpr int DIP_MANUAL_ACTIVE_LEVEL = 1;              // Pin level when the switch is ON: 1 = HIGH, 0 = LOW. Change to 0 if the switch is wired the other way round
constexpr uint32_t DIP_DEBOUNCE_MS = 30;                // The DIP switch must be stable this long before it counts
constexpr int MANUAL_PULSE_RATE = 1500;                 // Manual movement speed in steps/s (direct pulses, same engine as homing)
constexpr int MANUAL_START_RATE = 400;                  // Speed when a spreader starts moving, in steps/s
constexpr int MANUAL_RAMP_MS = 80;                      // Time to ramp from the start rate to the manual rate. Stopping is always instant
constexpr uint32_t MANUAL_COMM_TIMEOUT_MS = 1000;       // CPU1 cancels any manual movement if the PLC has sent no Modbus request for this long (dead man)
constexpr uint32_t MANUAL_POLL_IDLE_MS = 25;            // CPU2 asks CPU1 for the manual command this often while nothing is moving (every loop pass while moving)
constexpr int MANUAL_MAX_JOG_STEPS = MAX_STEPS + 500;   // One jog stops after this many pulses per stepper (guards against a failed home sensor)
constexpr uint8_t I2C_CMD_MANUAL = 6;                   // Request: a ManualCommand (see structures.h): mode switches, coils, spreader, and the 16 bit input word

// Bits of ManualCommand.flags
constexpr uint8_t MANUAL_FLAG_DIP = 1;                  // The manual DIP switch is on
constexpr uint8_t MANUAL_FLAG_SOFT = 2;                 // The PLC has requested manual mode (coil ADDR_MANUAL)
constexpr uint8_t MANUAL_FLAG_OPEN = 4;                 // Move forward (open) requested (coil ADDR_MANUAL_OPN), already cancelled when the PLC is silent
constexpr uint8_t MANUAL_FLAG_CLOSE = 8;                // Move backward (close) requested (coil ADDR_MANUAL_CLS), already cancelled when the PLC is silent

// Bits of StatusPacket.flags
constexpr uint8_t STATUS_FLAG_KNOWN = 1;                // Stepper positions are known (homed)
constexpr uint8_t STATUS_FLAG_HOMING = 2;               // CPU2 is homing (any stage). CPU1 shows this to the PLC in coil ADDR_HOMING
constexpr uint8_t STATUS_FLAG_MANUAL = 4;               // CPU2 is in manual mode

constexpr uint8_t STATE_MANUAL = 6;                     // CPU2 state: manual mode active

constexpr uint8_t EVT_MANUAL_ON = 16;                   // Manual mode started. Arg: 1 = DIP switch, 2 = PLC, 3 = both
constexpr uint8_t EVT_MANUAL_OFF = 17;                  // Manual mode ended (an automatic home follows)
constexpr uint8_t EVT_JOG_START = 18;                   // Spreader started moving. Arg = spreader number + 100 (open) or + 200 (close)
constexpr uint8_t EVT_JOG_STOP = 19;                    // All spreaders stopped. Arg = EVT_JOG_ constant
constexpr uint8_t EVT_MOVE_ABORTED = 20;                // A move or homing was stopped by the manual DIP switch
constexpr uint8_t EVT_MANUAL_BAD_SPREADER = 21;         // Manual movement requested for an invalid spreader (5 is static). Arg = the number
constexpr uint8_t EVT_AUTO_HOME_SKIPPED = 22;           // No automatic home after manual mode because a fault is active

constexpr int EVT_JOG_RELEASED = 0;                     // The coil was released
constexpr int EVT_JOG_TOUCHING = 1;                     // Closing: the spreader is up against its neighbour (home sensor on)
constexpr int EVT_JOG_LIMIT = 2;                        // Travel limit reached
constexpr int EVT_JOG_OVERTRAVEL = 3;                   // An over travel sensor is on
constexpr int EVT_JOG_LINK = 4;                         // CPU2 could not read the manual command from CPU1
constexpr int EVT_JOG_MODE_END = 5;                     // Manual mode ended

// CPU2 pin numbers
// ================
// Stepper driver pins
constexpr int STEPPER1_PULSE_PIN = 13;
constexpr int STEPPER1_DIR_PIN = 33;
constexpr int STEPPER2_PULSE_PIN = 12;
constexpr int STEPPER2_DIR_PIN = 34;
constexpr int STEPPER3_PULSE_PIN = 11;
constexpr int STEPPER3_DIR_PIN = 35;
constexpr int STEPPER4_PULSE_PIN = 10;
constexpr int STEPPER4_DIR_PIN = 36;
constexpr int STEPPER5_PULSE_PIN = 9;
constexpr int STEPPER5_DIR_PIN = 37;
constexpr int STEPPER6_PULSE_PIN = 8;
constexpr int STEPPER6_DIR_PIN = 38;
constexpr int STEPPER7_PULSE_PIN = 7;
constexpr int STEPPER7_DIR_PIN = 32;
constexpr int STEPPER8_PULSE_PIN = 6;
constexpr int STEPPER8_DIR_PIN = 31;
constexpr int STEPPER9_PULSE_PIN = 5;
constexpr int STEPPER9_DIR_PIN = 30;
constexpr int STEPPER10_PULSE_PIN = 4;
constexpr int STEPPER10_DIR_PIN = 29;
constexpr int STEPPER11_PULSE_PIN = 3;
constexpr int STEPPER11_DIR_PIN = 28;
constexpr int STEPPER12_PULSE_PIN = 2;
constexpr int STEPPER12_DIR_PIN = 27;

// Hardwired IO signal pins between CPU1 and CPU2 (see the CPU1 pin descriptions above). These are pulled down so a disconnected wire cannot cause false triggers.
constexpr int INPUT_A1 = 23;                  // Start move: CPU2 acts on the rising edge, reads the pattern over I2C. Pattern 0 means go home
constexpr int INPUT_A2 = 17;                  // Manual DIP switch: high while the manual mode DIP switch is on. CPU2 stops any motion at once and enters manual mode
constexpr int INPUT_A3 = 18;                  // Fault reset: rising edge clears the homing fault and starts a search home
constexpr int INPUT_A4 = 19;                  // Over travel: high while an over travel sensor is on (level, not an edge). Ignored during the direct pulse homing stage
constexpr int OUTPUT_B1 = 20;                 // Reserved, not used. The Home signal is calculated by CPU1 from the home proximity sensors
constexpr int OUTPUT_B2 = 21;                 // At target: set high when a move or homing has finished, cleared when a move or homing starts
constexpr int OUTPUT_B3 = 22;                 // Fault: set high while a fault (homing failed or over travel) is active, until reset with INPUT_A3
constexpr int OUTPUT_B4 = 26;                 // Request refused: set high when a request is refused (see EVT_REASON_ constants), cleared when the next request arrives

#endif