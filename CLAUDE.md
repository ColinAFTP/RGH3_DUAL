# RGH3_DUAL

Dual Teensy 4.1 controller for a gripper spreader. A PLC commands gap patterns over Modbus TCP; 9 stepper motors position the spreaders. Supersedes `GEMINI.md` (stale) — keep this file current at the end of each session.

## Build

`pio` is not on PATH in this shell: use `~/.platformio/penv/Scripts/pio.exe`.

- `pio run -e cpu1` / `pio run -e cpu2` (add `-t upload`). CPU1 = COM9, CPU2 = COM10, USB serial.
- `platformio.ini` uses `build_src_filter = +<cpu1/> +<shared/>` (or cpu2). `src/shared/` and `include/` compile into BOTH CPUs.
- No unit tests. Verification is a clean build of both envs plus bench testing on the desk board.

## Architecture

- **CPU1 (comms/IO)**: Modbus TCP server (192.168.2.51–54 from DIP switches), 16 inputs via 74HC165, 16 relays via 74HC595, I2C slave at 0x40 (Wire2).
- **CPU2 (motion)**: 9 steppers via TeensyStep4 (`StepperGroup g1`), I2C master.
- I2C commands (CPU2 -> CPU1): 1 = input word, 2 = full gap block + speed (`PatternPacket`), 3 = selected pattern.
- Hardwired lines: CPU1 `OUTPUT_A1` -> CPU2 `INPUT_A1` (500 ms pulse = "start move"); CPU2 `OUTPUT_B1/B2` -> CPU1 `INPUT_B1/B2` (at home / at target), which CPU1 mirrors to Modbus discrete inputs 117/118 and relay bits 0–1.
- Modbus map (all in `include/constants.h`): holding 101–160 (101–107 control/status, pattern p gaps at `111 + p*STRIDE_GAPS`); coils 101–120; discrete inputs 101–120.

## Domain facts (from Colin)

- (OUTDATED, see SPEC section: layout is 4 left + static + 5 right) The gripper has 10 spreaders. The 5th is **static** — a fixed reference all others close against when homing. 9 movable spreaders (9 steppers), asymmetric: 5 on one side of the static one (`i = 0..4`), 4 on the other (`5..8`).
- (OUTDATED, see SPEC section) `gapArrays[p][i]`: gap[4] = last left mover to the static spreader, gap[5] = static spreader to first right mover. Target for stepper i = cumulative gap distance from the static spreader (absolute steps from home). The maths in `stepTargetCalc` is correct; the asymmetry is real, not a bug.
- (PARTLY OUTDATED, see SPEC section: pattern 0 IS home) **Home** = all spreaders closed against the static one. Each spreader has a proximity sensor that fires when it touches the previous spreader (sensors land in CPU1's input word; reach CPU2 via I2C command 1 or a new hardwired line). Home is its own PLC operation (`ADDR_HOMING`), NOT pattern 0.
- Normal PLC sequence: home -> pattern x -> home -> pattern y -> home -> ... Re-selecting the same pattern without homing in between is not expected. Note that x -> home -> x reselects the same value, so homing must reset CPU1's "previous pattern" tracking or that move will never trigger.
- A DIP switch is planned to select manual mode (`INPUT_A2` / `OUTPUT_A2` lines exist).
- Steps/mm = 70.73553 (mod-2 rack, 18-tooth pinion, 8000 steps/rev). Max travel 16000 steps.
- The board is currently on Colin's desk with no gripper or proximity sensors attached. Do not assume hardware feedback exists when testing.

## Conventions

- Shared logic goes in `src/shared/` + `include/`; CPU-specific `main.cpp` stays thin.
- CPU1 must keep polling Modbus and CPU2 must keep servicing motion: avoid `delay()` and blocking loops.
- Never print from ISRs.
- Colin is an experienced controls person; keep explanations concise and flag physical-safety implications.

## Known issues / roadmap (as of 2026-09-19 review)

Status key: [ ] open, [x] done. Update as work lands.

Correctness
- [x] CPU2 uses stale gaps/speed at move time (CPU1 refreshes every 5 s, CPU2 every 10 s). Fix: CPU1 refreshes on pattern change; CPU2 re-reads gaps right after the A1 trigger.
- [x] `readPattern()` returns -1 on I2C failure and `stepTargetCalc(-1)` indexes `gapArrays[-1]`.
- [x] `stepTargetCalc` failure is silent: `triggerMove` still moves to stale targets. Limit check ignores negative values and middle steppers.
- [x] `triggerMove` at-target/at-home logic. Now: B2 after every completed move; B1 only when all stepper positions are 0 (interim, until homing exists). BOTH signals go to the PLC and must be kept.
- [x] `onI2CRequest` (ISR) copies `gapArrays` while the main loop may be writing it (torn packet).
- [x] `initShiftRegisters` never sets `INPUTS_DATA_LOAD_PIN` to OUTPUT; `inputsCheck` blocks 2 ms per 25 ms with `delay(1)`.
- [x] `INIT_ACCEL` applied, targets rounded, dead relay range check removed.

Robustness / production
- [x] `while(!Serial)` in both `setup()`s: firmware never starts without USB attached.
- [x] Debug output (now `DEBUG_*` flags in constants.h, plotter runs in loop, no ISR printing) (`debugPrinting`, 50-line dump every 5 s, plotter output printed from an `IntervalTimer` ISR with a " mm" suffix that breaks the Serial Plotter).
- [x] `triggerMove` now non-blocking (`startMove` + `moveService`). Triggers during a move are ignored (PLC must wait for At Target). Old blocking note: `g1.move()`; a trigger during a move is lost (500 ms pulse).
- [x] `ADDR_GAP_UPDATE`: no longer needed (gaps reload every 5 s and on every pattern change); the coil is still cleared as an acknowledgement.

Missing features
- [x] Homing implemented per SPEC (2026-09-19). UNTESTED on hardware: build only. Tune HOME_* constants on the real gripper.
- [ ] Manual mode (coils 104–106, register 107, DIP switch).

Housekeeping
- [x] Housekeeping done 2026-09-19: unused gap pattern arrays, `inputsStrip`, duplicate declarations, `constants.cpp`, `GEMINI.md`/`GEMINI.txt` and the duplicate `lib/TeensyStep4-main` removed; CPU1-only code no longer compiles into CPU2 (see File layout); git libraries pinned to tested commits in platformio.ini.
- [ ] Gap resolution is whole mm (16-bit registers); decide whether 0.1 mm scaling is needed.
- [x] Fix comment in `stepTargetCalc` right side ("gap[mid..i]" should read "gap[mid+1..i]").

## Session log

- 2026-09-19: Review, checkpoint commit 12cc709, then correctness/robustness fixes (commit d74b415): stale-data fix (CPU1 refreshes on pattern change, CPU2 re-reads on trigger), target validation, non-blocking moves, at-home/at-target signals, staged gap copy, timed serial wait, debug flags. Builds clean; NOT yet bench-tested. Next: bench test on desk board, then homing (proximity sensors via I2C cmd 1 `readIO`), then manual mode (DIP switch), then housekeeping.

## Bench testing (desk board, no gripper)

- Flashing: press the Teensy program button when `pio run -e <env> -t upload` starts; the auto-reboot does not work. With BOTH boards on USB the loader "auto-searches" and can flash the wrong one (it did: CPU1 firmware landed on CPU2). **Unplug the other board's USB before flashing.** Do not kill `teensy.exe` before the loader has rebooted the board (leaves it in bootloader mode; power-cycle to recover). Both boards are externally powered.
- Serial: only one program can hold a COM port. Plotter (VS Code) and my logging cannot share COM10. Windows shows one serial number per board: CPU1 = 18203890 (COM9), CPU2 = 18203150 (COM10).
- `tools/modbus_test.js` (Node, no libraries) drives CPU1's Modbus server at 192.168.2.51 for timing-critical tests (`node tools/modbus_test.js`, or `... zero` to return to zero). CPU1 accepts ONE Modbus client at a time, so disconnect the simulator first. Register addresses are raw protocol addresses as in constants.h.
- Set `DEBUG_PLOT = true` in constants.h and reflash CPU2 to feed the VS Code Serial Plotter (values in mm).
- Results 2026-09-19: pattern 1 (30 mm gaps) moves 9 steppers together to 150/120/90/60/30 mm, ~2.3 s at speed 5000; return to zero reaches 0.00 on all nine; over-limit pattern (50 mm gaps, 17684 steps) is refused with no motion; pattern 7 rejected by CPU1; gaps written then pattern selected 14 ms later moves to the NEW gaps (stale-data fix confirmed); trigger during a move is ignored; At Target (118) rises at move end, Home (117) rises when all positions are 0.
- Still untested: I2C behaviour with CPU1 absent (`requestFrom` may succeed with zeros, which would look like an all-zero valid pattern), homing, manual mode, real motors.
- Known limitation: a pattern selected mid-move is dropped by CPU2 while CPU1 records it as current (PLC must wait for At Target).

## SPEC (agreed 2026-09-19): layout, pattern numbering, homing, faults

This section supersedes any contradicting text above (notably "home is not pattern 0" and the 5-left/4-right layout).

**Physical layout** (CAD-confirmed): 10 spreaders numbered 1-10 left to right, looking from the control-box side. Spreader 5 is static. Movers: 1-4 (left of static) and 6-10 (right). Steppers 1-9 (index 0-8) drive spreaders 1,2,3,4,6,7,8,9,10 in order. Gap registers of a pattern run left to right: gap[0]=S1-S2 ... gap[3]=S4-S5, gap[4]=S5-S6 ... gap[8]=S9-S10. So left group = gap[0..3] (4 steppers, index 0-3), right group = gap[4..8] (5 steppers, index 4-8). Stepper position 0 = spreader closed against its home-side neighbour; target of a stepper = cumulative gaps from the static spreader outwards. Motors run forward = open, backward = close on both sides (wiring swapped physically).

**Pattern register (holding 104)**: 0 = HOME (no gap data needed). 1-5 = patterns using register blocks 111-120, 121-130, ... 151-160 (value n reads block n-1). PLC never selects the same value twice in a row (0 always sits between patterns). PLC waits for Home (DI 117) before collecting.

**Proximity sensors** (CPU1 input word, bit = proxy number - 1): proxy 1 (bit 0) = left over-travel, proxy 11 (bit 10) = right over-travel: both UNUSED for now, ignore. Proxy 2-5 (bits 1-4) = spreaders 1-4 home; proxy 6-10 (bits 5-9) = spreaders 6-10 home. So stepper index i uses bit i+1. Mask of all nine = 0x3FE. Sensor ON = spreader closed against its home-side neighbour. Bit 15 was a bench LED test bridge: ignore.

**Homing** (CPU2), used when PLC writes 0, at power-up if any of proxies 2-10 is off, and after a fault reset:
- Normal home when positions are known: TeensyStep moves every stepper to +HOME_APPROACH_MM, then the direct-pulse cascade closes the rest. If HOME_APPROACH_MM = 0, TeensyStep goes all the way to 0 and there is no cascade (fallback if hybrid homing is too slow). Search home (positions unknown: power-up, after fault): cascade only.
- Direct-pulse cascade: TeensyStep is NOT used. One timer interrupt generates step pulses for all nine steppers at a constant rate (HOME_PULSE_RATE) with a short ramp up from HOME_START_RATE over HOME_RAMP_MS. Ramp up only; stop is instant (pulses just cease) for accuracy. Each stepper runs closed while its proxy is OFF and stops the instant it goes ON; it restarts as soon as it goes off again (neighbour moved away). Left and right groups are independent: no waiting for neighbours. Done when all nine proxies are on; then TeensyStep positions are set to 0.
- CPU2 reads the proxies via I2C command 1 (`readIO`). CPU1 samples inputs every 1 ms. If I2C reads fail, pulses stop (never move blind).
- Limit is in STEPS not time: a spreader that emits more than HOME_MAX_STEPS pulses without its proxy going on is failed. Plus an overall time watchdog HOME_TIMEOUT_MS.
- Colin plans to tune HOME_* constants on the real gripper, and will revert to TeensyStep-only if hybrid homing is too slow.

**Signals**
- Home (DI 117): computed by CPU1 = proxies 2-10 all on AND no fault. Relay bit 0 mirrors it. CPU2 OUTPUT_B1 is no longer used for it.
- At Target (DI 118, relay bit 1): pattern move finished, or homing finished with all proxies on. Cleared when any move/homing starts. Driven by CPU2 OUTPUT_B2.
- Homing fault (DI 120): CPU2 OUTPUT_B3 -> CPU1 INPUT_B3. Holding register 108 = bitmask of failed spreaders, bit (spreader number - 1), sent CPU2 -> CPU1 with I2C write command 4. Fault stops all pulses. While faulted, or while positions are unknown, CPU2 refuses pattern moves.
- Fault reset: coil 107 (PLC writes 1; CPU1 clears it and pulses OUTPUT_A3 -> CPU2 INPUT_A3). Reset clears the fault and automatically starts a search home.
- Coil 101 (`ADDR_HOMING`) is left in place unused. REVISIT this decision later.

**Hardware**: driver PST8072, motor PHG57S56-430-PR20-40HC (NEMA23, 1.8 deg, 3 A, 20:1 gearbox). 8000 steps/rev at the pinion (half-step driver setting). Look up PST8072 min pulse width / direction setup time when finalising pulse timing.

## Session log (continued)

- 2026-09-19 (later): implemented the SPEC above. New: `src/cpu2/functions_homing.cpp` + `include/functions_homing.h` (direct-pulse cascade, fault handling), 4/5 layout in `stepTargetCalc`, pattern 0 = home and value n -> block n-1, CPU1 computes Home from proximity bits, 1 ms input sampling, coil 107 fault reset -> A3 pulse, holding 108 fault mask via I2C command 4, I2C slave now starts last in CPU1 `setup()`, `readIO` returns a uint16. Both envs build. NOT flashed or bench tested yet.
- Bench test plan for the new firmware: (1) power up with proxies 2-10 simulated on: expect "gripper is home", Home (117) high. (2) Power up with a proxy off: expect search-home pulses, then fault after HOME_MAX_STEPS pulses / timeout since nothing closes the sensor on the desk; check DI 120, register 108, then coil 107 reset. (3) Patterns 1-5 then 0. Watch pin 13 (stepper 1 pulse) LED. Bench numbers change with the new layout: 30 mm gaps now give spreader 1 = 120 mm and spreader 10 = 150 mm.
- Open questions for real hardware: PST8072 minimum pulse width / direction setup (pulse is HOME_TICK_US = 20 us wide, TeensyStep waits 5 us for direction); real proxy latency over I2C; startup/homing rate and ramp tuning.

## Bench results: homing (2026-09-19, desk board, proxies 2-10 simulated by switches)

- Power up with proxies 2-10 on: CPU2 reads them over I2C and reports "gripper is home". Both boards flashed with the SPEC firmware (commit 51467dc).
- Pattern 1 (20 mm gaps in registers 111-119, pattern n = block n-1): ~2.5 s at speed 3000. Home (pattern 0) back: ~2.4 s (TeensyStep to +5 mm, then the direct-pulse stage; instant with proxies held on). Home stays high with simulated proxies held on.
- Fault path confirmed by Colin: proxy 2 left off while homing raises the homing fault (DI 120); the fault reset (coil 107 -> A3 -> CPU2) clears it and restarts the search home.
- Not yet checked: register 108 value (expect 0x0001 for spreader 1), proxy switched ON mid-pulse stops the stepper instantly ("Homing complete"), cascade following behaviour with real spreaders, measured overtravel, PST8072 pulse timing.
- `tools/modbus_home_test.js` runs the pattern 1 then home test, and `... reset` pulses the fault reset coil. Both print Home / At Target / Fault / register 108 changes with timestamps.

## File layout (after the 2026-09-19 restructure)

- `include/`: all headers. `constants.h` = every address, pin, layout, homing and I2C constant (documented). `variables.h` = variables both CPUs use. `variables_cpu1.h` = CPU1 only (Ethernet, Modbus, IO, relays).
- `src/shared/variables.cpp`: the only shared source file.
- `src/cpu1/`: `main.cpp`, `functions_comms.cpp` (Modbus/Ethernet), `functions_io_cpu1.cpp` (shift registers, relays, feedbackCheck), `functions_i2c_cpu1.cpp` (I2C slave), `variables_cpu1.cpp`.
- `src/cpu2/`: `main.cpp`, `functions_steppers.cpp` (TeensyStep, targets), `functions_homing.cpp` (direct-pulse homing, faults), `functions_i2c_cpu2.cpp` (I2C master), `functions_io_cpu2.cpp`.
- `tools/`: Node bench-test scripts (see Bench testing).
- Library versions are pinned by commit in `platformio.ini` (`#sha` after the git URL). CPU1 needs FastShiftIn, ArduinoModbus, ArduinoRS485; CPU2 needs TeensyStep4 only.

## PST8072 driver timing (2026-09-19)

The PST8072 (PrimoPal Motor) datasheet could NOT be found online (primopal.com lists no manual for it). Ask the supplier or check the paper manual for: minimum step pulse width, direction setup/hold time, active edge, input voltage levels (Teensy outputs are 3.3 V). Our timings: TeensyStep pulse = 8 us (`setPulseParams(8, ...)` in the library), TeensyStep waits 5 us after the direction pin changes; direct-pulse homing pulse = `HOME_TICK_US` = 20 us with 10 us direction setup. These are above the 2.5 us / 5 us typical of drivers in this class, but unconfirmed for this driver.

## Homing checks confirmed by Colin (2026-09-19)

Register 108 reports the failed spreader (spreader 1 for proxy 2 left off). A proxy switched on 3 s into the pulse routine ends homing immediately ("Homing complete"). Fault reset via coil 107 works.

## Diagnostics web server (2026-09-19, read only)

- Open `http://192.168.2.51/` (the IP follows the DIP switches). CPU1 serves it on port 80: `/` (page in `include/web_page.h`), `/status.json`, `/gaps.json`, `/log.json?since=N`. Code: `src/cpu1/functions_web.cpp`, `include/functions_web.h`.
- Shows Home / At Target / Fault, CPU2 state, spreader table (home sensor, live position mm, fault), 16 inputs, 16 relays, gap patterns (active one highlighted), event log, PLC link, CPU1 loop time (avg/max).
- CPU2 -> CPU1: `StatusPacket` (structures.h) via I2C command 5 every `STATUS_PERIOD_MS` (250 ms), NOT sent during the direct-pulse homing stage (`homingStage()==2`). It carries state, positions (tenths of mm), fault mask and up to 6 queued events (`statusEvent(EVT_*, arg)` in `src/cpu2/functions_status.cpp`). CPU1 turns events into text in `logCpu2Event()`. To add an event: add an `EVT_` code in constants.h, call `statusEvent()` on CPU2, add a case in `logCpu2Event()`.
- CPU1's own events use `logEvent("fmt", ...)` (main loop only, never in an interrupt). 48-entry ring buffer.
- IMPORTANT library findings (NativeEthernet): `EthernetServer::available()` busy-waits up to 10 s for a new client's first bytes, which froze the CPU1 loop 20-40 ms per web request and could freeze it for a client that sends nothing. Use `accept()` (now used for both the web server and the Modbus connection). `EthernetClient::flush()` also blocks (up to ~70 ms): do not call it. After the fix the worst CPU1 loop pass under heavy web load is ~100 us. The loop time is on the web page: watch it after any change.
- The server handles one connection at a time (a silent client can hold the slot up to 1.5 s).
- Bench results: 150+ rapid and parallel requests all answered completely; live positions match the layout (pattern 1 with 20 mm gaps: S1 80, S4 20, S6 20, S10 100 mm).
- Observation to watch: the event log showed three "Home OFF / Home ON" pairs 1 ms apart with no one touching the proxy switches (around CPU2 boot and while serial ports were being opened). If it recurs on a quiet system, add a 2-3 sample glitch filter on CPU1's input sampling (costs 2-3 ms of latency).

## Flashing pitfalls (learned the hard way, 2026-09-19)

- `pio run -e <env> -t upload` uses whatever board is in the bootloader; with both boards on USB it can flash the wrong one, and a stale `teensy.exe`/`teensy_reboot.exe` can make it reuse an old image (the CPU2 board once ended up with CPU1 firmware). ALWAYS: unplug the other board's USB, press the program button, then confirm from the serial banner ("CPU 1 online" / "CPU 2 online") which firmware is running.
- Unkillable zombie `teensy_reboot.exe` entries in tasklist are harmless.

## Input glitch investigation and filter (2026-09-19)

- CPU1 logs every proxy input change (bits 0-10) on the web event log; a change that reverses within 5 ms is counted as a glitch (page: Status > Input Glitches). Logging uses the RAW samples (`inputDataRaw`).
- Bench data (5.4 min, nothing touched): 4 glitches, each exactly one 1 ms sample, all on P1 or P11 (the unused over-travel inputs, bits 0 and 10); none on proxies 2-10. Two were exactly 500 ms apart, the same as the web page poll interval: NOT yet ruled out that web/Ethernet activity couples into the inputs. Test: close the page for a few minutes and compare the glitch counter. Earlier, before this logging existed, a few "Home OFF / Home ON" pairs 1 ms apart were seen; the log did not say which input caused them.
- Filter: `INPUT_FILTER_SAMPLES = 3` in constants.h. `inputsCheck()` (src/cpu1/functions_io_cpu1.cpp) changes an input only after its raw value has held for that many consecutive samples. `inputData` = filtered (used by everything, including I2C to CPU2 and the Home signal); `inputDataRaw` = as sampled. Costs about 3 ms latency (about 0.06 mm at the 1500 steps/s homing rate). Set to 1 to turn it off. Glitches the filter removes are logged as "Input glitch (filtered out)".
- Bench check after adding the filter: pattern 1 (~2.5 s) and home (~2.4 s) unchanged.
- 2026-09-19 follow-up: Colin confirmed input 1 and input 11 are FLOATING on the bench, which explains most of the glitches (6 of the first 8; the other 2 were on P2, a wired input, and were removed by the filter with Home staying on). Glitches also continued with the web page closed, so web polling is not the cause. `INPUT_LOG_MASK` (constants.h, default 0x03FE = proxies 2-10) now limits the event-log input lines and the Input Glitches counter to the wired inputs; set it to 0x07FF when the over-travel sensors are connected. On the real gripper: shielded sensor cables, proper grounding, tie unused inputs to their inactive level.

## Over travel protection (implemented 2026-09-19, DISABLED by default)

Belts-and-braces: the controller already refuses targets beyond the rack travel. Proxy 1 (bit 0, left, triggered by spreader 1) and proxy 11 (bit 10, right, spreader 10) are normally-open like all the other proxies. Colin knows NO wiring does not detect a broken wire.
- `OVERTRAVEL_ENABLED` (constants.h) is `false`. KEEP IT false until inputs 1 and 11 are wired: they float on the bench and would fault at random. When true, `INPUT_LOG_MASK` also widens to include them.
- CPU1 (main loop): while (filtered `inputData` & `OVERTRAVEL_MASK`) is non-zero it holds OUTPUT_A4 high (level) and logs "OVER TRAVEL sensor on". Existing 3-sample input filter applies (Colin's choice), so the path delay is ~4-5 ms.
- CPU2 (`overTravelService()`, first thing in `loop()`): if INPUT_A4 is high, not faulted and not in the direct-pulse homing stage -> `emergencyStopMoves()` (TeensyStep `emergencyStop()` on the lead stepper, no deceleration), reads the inputs over I2C to find which side (bit 0 -> spreader 1, bit 10 -> spreader 10; both if unknown), raises a fault with type `FAULT_OVERTRAVEL`, event `EVT_OVERTRAVEL`.
- The homing cascade ignores over travel (it closes the spreaders, moving them away). Recovery = fault reset (coil 107) -> search home. If the sensor is still on when homing completes, the fault is raised again.
- Reuses the fault machinery: DI 120 = fault of either type, register 108 = spreader bitmask (bit 0 = S1, bit 9 = S10), NEW register 109 = fault type (0 none, 1 homing failed, 2 over travel). I2C command 4 now carries mask + type (3 bytes). Web page shows Fault Type. Positions are unknown after a stop, so pattern moves are refused until homing completes.
- TeensyStep gotcha: only the LEAD stepper of a group owns a timer (`stpTimer` is uninitialised for the others). NEVER call `emergencyStop()` on a stepper that is not the lead of a running move, it crashes. `SpreaderGroup` (functions_steppers.cpp) exposes the lead safely.
- Not a safety-rated function. If over travel protects people or the mechanics, also wire it to the PST8072 ENA inputs / motor supply so it works with the firmware down.
- NOT yet bench tested. To test: wire switches to inputs 1 and 11 (default off), set `OVERTRAVEL_ENABLED` true, flash both CPUs, start a long slow move (for example pattern 1 with 20 mm gaps at speed 1500) and flip a switch mid-move: expect the motion to stop within ~5 ms, Fault (120) on, register 108 = 0x001 (left) or 0x200 (right), register 109 = 2, log "OVER TRAVEL FAULT". Then reset with coil 107 (switch off first, or the fault returns).

## Pre-real-gripper review (end of session 2026-09-19): gaps found, not yet fixed

Everything below was found by reviewing the whole code base; none of it is fixed yet unless marked. Order = suggested priority.

**Fix before the first real run (functional/safety)**
1. [DONE 2026-09-20, bench-tested] Pulse-stage runaway. The direct-pulse ISR keeps pulsing a stepper while its `runFlag` is set, and only the main loop clears it (from fresh I2C sensor reads). If the CPU2 main loop hangs during homing, pulses continue with no sensor checking. Fix: the ISR must stop pulses if the last successful sensor read is older than ~20 ms (a stale-data timeout in the ISR), and/or enable the Teensy hardware watchdog.
2. [DONE 2026-09-20, CPU2 bench-tested] No hardware watchdog on either CPU (Teensy 4.1 WDT). A hang leaves outputs (At Target, Fault lines) frozen and the PLC is never told.
3. [DONE 2026-09-20, bench-tested] PLC is never told a request was REFUSED. `At Target` stays on from the previous move when a request is refused (targets out of range, positions unknown, fault active, busy, gap refresh failed): the PLC would think the gripper arrived. Fix: clear At Target on every new request (CPU1, when it pulses A1) and add a status bit "request refused / move error" (with a reason code), or raise a fault.
4. PLC handshake is undocumented. At Target drops ~20-70 ms AFTER the pattern write, so the PLC must wait for At Target = 0 and then = 1 (or delay >=150 ms) or it will see the old high. Write this down for the PLC programmer. Also: pattern writes are ignored while CPU2 is busy or homing, and after power-up the PLC must wait for Home before selecting a pattern.
5. [DONE 2026-09-20, bench-tested] CPU2 health is not visible to the PLC. If CPU2 stops (crash/I2C failure) the PLC only sees At Target stuck. CPU1 already knows (status packets stop for >1.5 s): expose it as a discrete input and treat it as a fault.
6. First-run verification of direction and scaling (procedure, not code): TeensyStep positive = OPEN and homing = LOW dir pin = CLOSE are ASSUMED; STEPS_PER_MM is computed, not measured. Run the first tests at low speed, with the over travel protection enabled (or a hand on the E-stop), motors disconnected from the mechanics first if possible; measure real travel against the reported mm.
7. Hardware note: stepper step/dir pins float while CPU2 resets or is being programmed, which can make the PST8072 move. Check for pull-downs, and consider driving the drives' ENA inputs from CPU2 (also gives a hardware stop for faults and over travel). Over travel via firmware is not a safety function.

**Should do soon**
8. [PARTLY DONE: counter added; measured 2 failed sensor reads in 21,798, still check the pull-ups] I2C reliability at 1 MHz: 7 read failures in ~12 s were seen during homing. Check for external I2C pull-ups (pins 24/25), add a failure counter to the web page, measure with the page open/closed.
9. [DONE 2026-09-20, bench-tested] Send the status/events during the direct-pulse stage too (one 0.5 ms write every 250 ms): today CPU2 events are batched until homing ends, so the log timeline is misleading.
10. [DONE 2026-09-20, bench-tested with a real cable pull] Modbus stale connection: only ONE Modbus client is served; an unclean disconnect (cable pulled) may leave the old socket "connected" until TCP times out and block the PLC from reconnecting. Test by pulling the cable; if it happens, accept the new client and drop the old one.
11. Manual mode / jog is not implemented (coils 104-106, register 107, DIP switch, INPUT_A2/OUTPUT_A2 exist). It is very useful for commissioning and for recovering from a jam.
12. Gap resolution is whole mm (16-bit registers). Decide whether 0.1 mm scaling is needed before the PLC program is written (changing it later changes the PLC).
13. Stuck-on proximity sensor is not detected (spreader thinks it is home). Optional plausibility check: at a pattern with a gap of several mm the matching proxy must be OFF.
14. Unimplemented Modbus items: coil 103 relay test, holding 102 home counts, coil 101 homing (left in place on purpose, revisit), input registers 101-110 unused. Relays 1 and 2 are overwritten by Home / At Target: the PLC must not use them.
15. Power-up auto-home moves the motors without a PLC command (Colin's requirement). Make sure this is acceptable for the real machine (guarding, person nearby).

**Still to tune / verify on the real gripper**: HOME_PULSE_RATE, HOME_START_RATE, HOME_RAMP_MS, HOME_APPROACH_MM, INIT_ACCEL, MAX_SPEED, PST8072 pulse width and direction setup (datasheet not found), INPUT_FILTER_SAMPLES for the real sensor cables, enable OVERTRAVEL_ENABLED once proxies 1 and 11 are wired. There are no automated tests: everything so far was bench-tested on the desk board with simulated proxies and no motors.

## Safety and diagnostics work, session 2 (2026-09-20)

**New PLC-visible interface** (in addition to the earlier map):
- DI 121 `ADDR_MOVE_REFUSED`: the last request was refused. Holding 110 `ADDR_REFUSED_REASON` says why (1 busy, 2 fault active, 3 positions unknown, 4 invalid pattern, 5 no gap data, 6 bad targets, 7 home failed). Cleared by the next valid pattern change. Source: CPU2 line B4 (OUTPUT_B4/INPUT_B4) for CPU2 refusals, CPU1 itself for an out-of-range pattern selection.
- DI 122 `ADDR_CPU2_ONLINE`: CPU2 is running and reporting. DI 120 (Fault) also turns on with fault type 3 (`FAULT_CPU2`, register 109) when CPU2 has sent no status for `CPU2_TIMEOUT_MS` (1.5 s), or none within `CPU2_BOOT_GRACE_MS` (40 s) of CPU1 starting. It clears by itself. Home and At Target now also require CPU2 online.
- At Target (DI 118) is forced off by CPU1 the moment it sees a valid new pattern (measured: drops 0-3 ms after the write, before it took 20-70 ms) and stays off until CPU2 has really completed the request: a refused request leaves it off. CPU2 also drops its own line at every trigger. PLC handshake: write the pattern, wait for At Target = 1 (it is already 0), and check Move Refused (121).
- Discrete input range configured 101-130.

**Watchdog** (`src/shared/functions_watchdog.cpp`, WDOG1, `WATCHDOG_TIMEOUT_MS` = 2000): both CPUs start it at the end of `setup()` and feed it once per `loop()`. A hung CPU resets itself; the boot log says "started after a WATCHDOG RESET". Reset cause is read from `WDOG1_WRSR` (the SRC_SRSR flag may be cleared by the Teensy CrashReport code first). Never put a wait longer than ~1 s in `loop()`; long waits in `setup()` must come before `watchdogStart()`. A watchdog reset in the middle of a cycle makes CPU2 do a power-up search home if proxies are not all on.
**Pulse stale-data stop**: the homing pulse interrupt stops ALL pulses if the last good sensor read is older than `HOME_SENSOR_TIMEOUT_US` (20 ms) and restarts them with the ramp; logged as "sensor data too old".
**Status during homing**: CPU2 sends status and events every 250 ms during the direct-pulse stage too, with positions estimated from the pulses counted (`cascadePositionSteps()`). **I2C counters**: `ioReads`, `ioFails`, `otherFails` in the status packet, shown on the page (CPU2 I2C Errors).
**Test switches** in constants.h (keep OFF): `DEBUG_HANG_TEST_CPU1_S`, `DEBUG_HANG_TEST_CPU2_S` (hang the main loop after N seconds to prove the watchdog), `DEBUG_STALL_TEST` (delays the homing loop 40 ms every second to prove the stale-data stop).

**Bench results (2026-09-20)**: refused pattern with 50 mm gaps: At Target stayed off, Refused on, reason 6; pattern 7: Refused on, reason 4 (CPU1); home cleared it. Stall test: "sensor data too old" logged once a second during homing. CPU2 hang test: CPU1 declared CPU2 lost 1.5 s later (Fault, type 3, CPU2 Online 0), CPU2 restarted by the watchdog ("started after a WATCHDOG RESET"), fault cleared by itself; ~8 s total outage. I2C: 2 failed sensor reads in 21,798 (+2 other failed transfers) during one 12 s homing run. NOT tested: CPU1's own watchdog reset (same shared code), a watchdog reset during a real move, unplugging the Ethernet cable while CPU1 runs (check it does not reset), hardware behaviour of the drive pins during a reset.
Tools: `tools/modbus_monitor.js [seconds] [home]` (prints every change of DI 117-122 and registers 108-110), `tools/modbus_refuse_test.js`.
- Found on the bench and fixed: after a CPU1 restart while CPU2 held a fault, registers 108/109 came back empty (CPU2 only sent the fault details once, at the moment of the fault). CPU1 now also takes `faultMask` and `faultType` from every status packet (`cpu2StatusService()`), so they are restored within 250 ms. Verified: after reflashing CPU1 during an active fault, Modbus showed Fault=1, 108=0x1, 109=1 at once.
- The I2C "other failures" counter rises by about 4 per second while CPU1 is restarting (CPU2's status writes get no answer): 110 after two CPU1 reflashes is normal, not a bus problem. Only `ioFails` (sensor reads, 4 of 21,653) reflects real bus errors.
- Ethernet cable pull test (2026-09-20): cable out ~20 s with no PLC connected: CPU1 uptime stayed continuous (340,513 ms -> 412,214 ms over 71 s), no watchdog reset, no CPU2-offline event, web page answered again at once. NOT yet tested: cable pulled while a PLC/Modbus client is connected, then reconnect (item 10, stale socket).

## Modbus reconnect after a vanished PLC (2026-09-20)

Problem (reproduced): a Modbus client that vanishes without closing its TCP connection (PLC power loss, cable pulled) kept the ONLY Modbus slot for ever; a returning client connected at TCP level but got no answers for 40+ s.
Fix (`ethernetConnect()` in src/cpu1/functions_comms.cpp, called every loop pass): a new client replaces the current one if the current one is closed, or has sent no request for `MODBUS_IDLE_TAKEOVER_MS` (3 s, constants.h). An ACTIVE client is never displaced: the newcomer's connection is closed and "Modbus connection refused: another client is active" is logged (max once per 5 s). `lastModbusRequestMs` is set whenever `modbusServer.poll()` answers a request. So a returning PLC gets back in within ~3 s of its last request; a PLC that retries connecting every few seconds will succeed on a retry.
Bench results: silent client replaced (served on the first attempt), active client (20 ms polling) untouched while a second client was refused, real cable pull (silent client + 10 s unplug) then a new client served on the first attempt. Worst CPU1 loop pass ~80 us. Tools: `tools/modbus_stale_test.js hold|probe`.
Note for the PLC programmer: if the PLC connects while another Modbus master (for example a Modbus simulator) is actively polling, the PLC is refused. Disconnect other masters first.

## OPEN ITEMS (consolidated 2026-09-20, supersedes the older lists above for what is still to do)

**A. Before the first run on a real gripper (hardware and procedure)**
1. Verify motor direction and scaling: TeensyStep positive = OPEN and homing (dir pin LOW) = CLOSE are assumed; `STEPS_PER_MM` is computed, not measured. Motors off the mechanics first if possible, low speed, hand on the E-stop.
2. Wire proxies 1 and 11, then set `OVERTRAVEL_ENABLED = true` and bench-test over travel (steps in the "Over travel protection" section). Until then inputs 1 and 11 float and the protection is OFF.
3. Drive pins during reset/programming: check the PST8072 inputs for a defined level (pull-downs) so the drives cannot move while a CPU resets or is flashed; consider driving the drives' ENA from CPU2 (hardware stop for faults/over travel). Firmware over travel is not a safety function.
4. PST8072 timing: datasheet not found; supplier note drafted (min pulse width, direction setup/hold, active edge, 3.3 V input levels, half-step and 3.0 A settings). Confirm our 8 us (TeensyStep) and 20 us (homing) pulses and 5/10 us direction setup.
5. Tune on the real gripper: `HOME_PULSE_RATE`, `HOME_START_RATE`, `HOME_RAMP_MS`, `HOME_APPROACH_MM` (0 = TeensyStep only, fallback if hybrid homing is too slow), `INIT_ACCEL`, `MAX_SPEED`, `INPUT_FILTER_SAMPLES` for the real sensor cables.
6. Confirm that auto-home at power-up (moves motors without a PLC command) is acceptable for the real machine (guarding, people nearby).

**B. Decisions still needed from Colin**
7. Gap resolution: whole mm (16-bit registers) or 0.1 mm scaling. Decide BEFORE the PLC program is written; changing later changes the PLC.
8. Coil 101 `ADDR_HOMING`: unused, left in place on purpose; revisit and remove or repurpose.
9. Manual mode / jog (coils 104-106, register 107, DIP switch, INPUT_A2/OUTPUT_A2): on hold by Colin. Useful for commissioning and jam recovery.

**C. Code still to write**
10. Stuck-on proximity sensor detection (optional plausibility check: with a gap of several mm the matching proxy must be OFF).
11. Unimplemented Modbus items: coil 103 relay test, holding 102 home counts, input registers 101-110 (unused). The PLC must not use relays 1 and 2 (overwritten by Home / At Target).

**D. Still to investigate or test**
12. I2C reliability: external pull-ups on pins 24/25 (1 MHz bus)? Measured 4 failed sensor reads in ~21,650; the Wire library also printed "Timed out waiting for transfer to finish".
13. Input glitches on WIRED inputs (P2, P3) still appear about every few minutes, some during the homing pulse stage: possible coupling from step pulse wires. The filter removes them; check wiring/shielding on the real gripper.
14. CPU1's own watchdog reset never provoked (same shared code as CPU2, which was). A watchdog reset in the middle of a real move: after it CPU2 does a power-up search home if proxies are not all on.
15. PLC handshake for the PLC programmer (write the pattern, wait for At Target = 1 which is already 0, check Move Refused 121, Home requires CPU2 online, Fault 120 types 1/2/3, other Modbus masters must be disconnected) is only written in CLAUDE.md, not in a document for the PLC side.

**E. Housekeeping**
16. Git: local commits after `1ce0f39` are NOT pushed to GitHub (`git push origin master`).
17. No automated tests exist; all testing was on the desk board with simulated proxies and no motors.

## Decisions by Colin, 2026-09-20 (answers to open items 6-9)
- Item 6: auto home at power-up is ACCEPTABLE AND DESIRABLE. Keep it.
- Item 7: gap resolution = WHOLE mm (the 16-bit holding registers stay as they are, no 0.1 mm scaling). Closed.
- Item 8: coil 101 (`ADDR_HOMING`) is used to tell the PLC that the gripper is busy with a homing routine. It is NOT a PLC request. Plan: CPU1 writes coil 101 = 1 while CPU2 is homing (any stage, including power-up homing and fault-reset homing), 0 otherwise. The PLC only reads it.
- Item 9: manual mode next. Spec from Colin: with the manual mode DIP switch on, the gripper takes movement instructions over Modbus to move spreaders ONE AT A TIME instead of all together: holding register 107 = which spreader, coil 105 = move forward (open), coil 106 = move backward (close). A soft manual mode can be requested by the PLC with coil 104. The DIP switch works in parallel with the PLC request and overrides it at hardware level. Manual movement uses the same direct pulse generation as homing (NOT TeensyStep). If the gripper is homed and spreader 8 is moved forward, spreaders 9 and 10 also move forward so 8 cannot crash into 9. Manual mode must be shown in ORANGE on the dashboard Status card.

## Manual mode bench results (2026-09-20)

Implemented per the spec above (Colin's answers: DIP = `DIP_SW3` pin 41 (HIGH = ON), hold-to-move coils, physical spreader numbers 1-10 (5 refused), PLC request waits for idle while the DIP switch stops a running move/homing (ramped), DIP on at power-up = manual mode without auto-home, dead man 1 s, fixed manual speed (`MANUAL_PULSE_RATE`), leaving manual mode ALWAYS starts an automatic home, requests during manual are refused with reason 8, faults do not block jogging, closing stops when the spreader's home sensor is on, opening pushes along the touching spreaders further out). Code: CPU2 `functions_manual.cpp`, `functions_pulses.cpp` (shared pulse engine), CPU1 `functions_manual_cpu1.cpp`; I2C command 6 (`ManualCommand`). Coil 101 is now written by CPU1 = homing in progress (any stage). DI 119 = manual mode active.
Bench test via Modbus (`tools/modbus_manual_test.js`, all passed): enter manual (DI 119 in ~270 ms, At Target off); open spreader 8 for 2 s: S8, S9, S10 all +41.4 mm, S6/S7 unchanged; close with all sensors on: no movement; dead man: the spreader stopped ~1 s after the PLC went silent; spreader 5: no movement; pattern request in manual: Refused, reason 8; leaving manual: coil 101 on during the automatic home, all spreaders back to 0.0 mm.
DIP switch: works (ON = HIGH on pin 41; log "Manual DIP switch ON" / "CPU2 MANUAL MODE ON (DIP switch)"; OFF starts the automatic home). PCB labelling found on the bench: what Colin calls "DIP switch 1" is pin 41 (manual mode, `DIP_SW3` in the code), "DIP switch 2" is pin 40 (`DIP_SW2`, IP address bit), and "DIP switch 3" did not change any of pins 39-41 (pin 39 = `DIP_SW1`, IP address bit): check it. The dashboard row "DIP Switch Pins (Raw)" shows the three raw levels.
Not yet tested: the DIP switch stopping a running TeensyStep move or homing (ramped stop, no At Target, `EVT_MOVE_ABORTED`), the DIP switch overriding a PLC request (DIP on then off while coil 104 is on: manual must stay on), power-up with the DIP switch on, an over-travel sensor blocking opening, a fault during manual, the push-along with real proximity sensors (the bench proxies are static, so the pushed spreaders stay "touching" as they move).
Note: after leaving manual mode CPU1's own Move Refused (reason 8, for a pattern written during manual) stays on until the next valid pattern change.
- DIP switches re-assigned by Colin (2026-09-20), superseding the mapping above. Names are the labels on the PCB; a switch is ON when its pin is HIGH: PCB DIP 1 = pin 41 = IP address bit 0, PCB DIP 2 = pin 40 = IP address bit 1, PCB DIP 3 = pin 39 = MANUAL MODE. IP number = DIP 2 x 2 + DIP 1 (0 = 192.168.2.51, 1 = .52, 2 = .53, 3 = .54), read at power up. Constants `DIP_PCB1_PIN`, `DIP_PCB2_PIN`, `DIP_PCB3_PIN` (the old `DIP_SW1..3` names are gone). Assumption to check: DIP 1 is the least significant IP bit. The dashboard row "DIP Switches (Raw)" shows the three levels.
- Verified on the bench (2026-09-20): PCB DIP 3 = manual mode (raw reading 4, "CPU2 MANUAL MODE ON (DIP switch)", OFF starts the automatic home). IP address by PCB DIP 1 (low bit) and DIP 2 (high bit), all four combinations tested after a power cycle: off/off = 192.168.2.51, DIP 1 = .52, DIP 2 = .53, both = .54. The test tools in `tools/` use 192.168.2.51 (set the DIP switches to off/off).
