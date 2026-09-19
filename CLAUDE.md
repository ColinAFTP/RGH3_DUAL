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

- The gripper has 10 spreaders. The 5th is **static** — a fixed reference all others close against when homing. 9 movable spreaders (9 steppers), asymmetric: 5 on one side of the static one (`i = 0..4`), 4 on the other (`5..8`).
- `gapArrays[p][i]`: gap[4] = last left mover to the static spreader, gap[5] = static spreader to first right mover. Target for stepper i = cumulative gap distance from the static spreader (absolute steps from home). The maths in `stepTargetCalc` is correct; the asymmetry is real, not a bug.
- **Home** = all spreaders closed against the static one. Each spreader has a proximity sensor that fires when it touches the previous spreader (sensors land in CPU1's input word; reach CPU2 via I2C command 1 or a new hardwired line). Home is its own PLC operation (`ADDR_HOMING`), NOT pattern 0.
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
- [ ] Homing (proximity sensors, sets position 0, sets "at home"). No limit switches exist.
- [ ] Manual mode (coils 104–106, register 107, DIP switch).

Housekeeping
- [ ] Delete unused `gapPattern0–5`, duplicate `feedbackCheck` declaration, move Ethernet globals out of shared `variables.cpp`, pin git `lib_deps` (local copies also in `lib/`), delete `GEMINI.txt`/`GEMINI.md`.
- [ ] Gap resolution is whole mm (16-bit registers); decide whether 0.1 mm scaling is needed.
- [x] Fix comment in `stepTargetCalc` right side ("gap[mid..i]" should read "gap[mid+1..i]").

## Session log

- 2026-09-19: Review, checkpoint commit 12cc709, then correctness/robustness fixes (commit d74b415): stale-data fix (CPU1 refreshes on pattern change, CPU2 re-reads on trigger), target validation, non-blocking moves, at-home/at-target signals, staged gap copy, timed serial wait, debug flags. Builds clean; NOT yet bench-tested. Next: bench test on desk board, then homing (proximity sensors via I2C cmd 1 `readIO`), then manual mode (DIP switch), then housekeeping.
