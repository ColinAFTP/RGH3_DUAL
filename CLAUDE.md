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
- [x] `INIT_ACCEL` applied, targets rounded. Still open: relay range check on a `word` is dead code.

Robustness / production
- [x] `while(!Serial)` in both `setup()`s: firmware never starts without USB attached.
- [x] Debug output (now `DEBUG_*` flags in constants.h, plotter runs in loop, no ISR printing) (`debugPrinting`, 50-line dump every 5 s, plotter output printed from an `IntervalTimer` ISR with a " mm" suffix that breaks the Serial Plotter).
- [x] `triggerMove` now non-blocking (`startMove` + `moveService`). Triggers during a move are ignored (PLC must wait for At Target). Old blocking note: `g1.move()`; a trigger during a move is lost (500 ms pulse).
- [ ] `ADDR_GAP_UPDATE` flag check commented out in `patternUpdateCheck`.

Missing features
- [x] Homing implemented per SPEC (2026-09-19). UNTESTED on hardware: build only. Tune HOME_* constants on the real gripper.
- [ ] Manual mode (coils 104–106, register 107, DIP switch).

Housekeeping
- [ ] Delete unused `gapPattern0–5`, duplicate `feedbackCheck` declaration, move Ethernet globals out of shared `variables.cpp`, pin git `lib_deps` (local copies also in `lib/`), delete `GEMINI.txt`/`GEMINI.md`.
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
