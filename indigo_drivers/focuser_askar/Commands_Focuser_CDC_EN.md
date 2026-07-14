# Askar-WAF USB CDC Focuser Serial Protocol

Implemented in `Askar-WAF_v1.0.0.c` (`process_cdc_cmd()`), on the USB CDC virtual serial port.  
This is **not** the FreeRTOS CLI protocol (`help`, `wifi`, etc.). Focuser commands start with `F` and end with `#`.

---

## General conventions


| Item               | Description                                                                                                                 |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| Physical interface | USB CDC (e.g. Linux `/dev/ttyACM0`)                                                                                         |
| Frame format       | `F` + payload + `#`; host may send `\n` or `\r\n`                                                                           |
| Response format    | payload + `#` + `\r\n`                                                                                                      |
| Coordinates        | **Logical position** `n` in `0 … max_step` (default `max_step = 100000`; read with `Fm#`, write with `FMn#`, stored in NVS) |
| Read/write naming  | Same letter, lowercase = read (`Fm#`, `Fb#`, `Fr#`), uppercase + value = write (`FMn#`, `FBn#`, `FR0#`/`FR1#`) |
| Host vs firmware   | Host only uses logical coordinates; offset is internal (drivers need not implement it).                                     |
| Out of range       | Move commands **clamp** target to `[0, max_step]`; usually no `FE#`                                                         |
| Command length     | Single frame ≤ 31 characters (including leading `F` and trailing `#`); longer frames return `FE#`                           |
| Failure            | Unified response `FE#\r\n` (unknown command, parse error, invalid numeric field, etc.)                                      |


---

## Command summary


| #   | Function              | Type  | Host sends | Device replies           | Notes                                                        |
| --- | --------------------- | ----- | ---------- | ------------------------ | ------------------------------------------------------------ |
| 1   | Absolute move         | Write | `FPn#`     | `FPn#\r\n`               | `n` = target logical position                                |
| 2   | Relative move (+)     | Write | `FT+n#`    | `FT+n#\r\n`              | Target = current logical position + `n`                      |
| 3   | Relative move (−)     | Write | `FT-n#`    | `FT-n#\r\n`              | Target = current logical position − `n`                      |
| 4   | Halt                  | Write | `FS#`      | `FS#\r\n`                | Emergency stop; send `Fp#` afterward; saves position to NVS  |
| 5   | Sync logical position | Write | `FYn#`     | `FYn#\r\n`               | Set logical position to `n` **without moving** (INDIGO SYNC); writes NVS |
| 6   | Read position         | Read  | `Fp#`      | `Fpn#\r\n`               | `n` in the reply is the current logical position             |
| 7   | Read motion state     | Read  | `FQ#`      | `FQ0#\r\n` or `FQ1#\r\n` | `0` = idle, `1` = moving                                     |
| 8   | Read max travel       | Read  | `Fm#`      | `Fmn#\r\n`               | `n` = `max_step`; `FM#` accepted as alias                    |
| 9   | Set max travel        | Write | `FMn#`     | `FMn#\r\n`               | `n` in 100–1000000; stops motion, writes NVS, position unchanged; `FE#` if current position > `n`; `FXn#` legacy alias |
| 10  | Read firmware version | Read  | `FV#`      | `FVv#\r\n`               | e.g. `FV1.0.0#\r\n` (`ALPACA_VERSION`)                       |
| 11  | Read model name       | Read  | `FI#`      | `FImodel#\r\n`           | Currently `FIAskar-WAF#\r\n` (`ALPACA_ServerName`)           |
| 12  | Read backlash offset  | Read  | `Fb#`      | `Fbn#\r\n`               | Backlash `n` in `0 … 10000`; `0` means no firmware compensation |
| 13  | Set backlash offset   | Write | `FBn#`     | `FBn#\r\n`               | Clamps `n`, writes NVS; does not move focuser                |
| 14  | Read reverse motion   | Read  | `Fr#`      | `Fr0#\r\n` or `Fr1#\r\n` | `0` = normal, `1` = reversed motor direction                 |
| 15  | Set reverse motion    | Write | `FR0#`     | `FR0#\r\n`               | Normal direction; stops motion, writes NVS                   |
| 16  | Set reverse motion    | Write | `FR1#`     | `FR1#\r\n`               | Reversed direction; stops motion, writes NVS                 |
| 17  | Read motor mode       | Read  | `Fo#`      | `Fo0#\r\n` or `Fo1#\r\n` | `0` = high performance, `1` = balanced; `FO#` accepted as alias |
| 18  | Set motor mode        | Write | `FO0#`     | `FO0#\r\n`               | High performance; stops motion, writes NVS, reconfigures motor driver registers |
| 19  | Set motor mode        | Write | `FO1#`     | `FO1#\r\n`               | Balanced; stops motion, writes NVS, reconfigures motor driver registers |
| 20  | Command failure       | —     | (invalid)  | `FE#\r\n`                | See “Errors and edge cases”; e.g. `FO2#` returns `FE#`       |


---

## Command details

### 1. Absolute move `FPn#`


| Item          | Value                                                                                |
| ------------- | ------------------------------------------------------------------------------------ |
| Example host  | `FP50000#`                                                                           |
| Success reply | `FP50000#\r\n`                                                                       |
| Behavior      | Asynchronous move to logical position `n`; poll `FQ#` and `Fp#` to detect completion |


### 2. Relative move `FT+n#` / `FT-n#`


| Item          | Value                                                                                                                       |
| ------------- | --------------------------------------------------------------------------------------------------------------------------- |
| Example host  | `FT+100#`, `FT-50#`                                                                                                         |
| Success reply | `FT+100#\r\n`, `FT-50#\r\n`                                                                                                 |
| Behavior      | Adds or subtracts `n` from the current logical position, then GOTO; reply echoes the **step count**, not the final position |


### 3. Halt `FS#`


| Item     | Value                                                                                   |
| -------- | --------------------------------------------------------------------------------------- |
| Host     | `FS#`                                                                                   |
| Reply    | `FS#\r\n`                                                                               |
| Behavior | Clears motion request and requests motor-task halt; clears stall buzzer state if active; writes current logical position to NVS key `"position"` |


### 4. Sync logical position `FYn#`


| Item          | Value                                                                                                      |
| ------------- | ---------------------------------------------------------------------------------------------------------- |
| Example host  | `FY80000#`                                                                                                 |
| Success reply | `FY80000#\r\n` (`n` clamped to `[0, max_step]`)                                                            |
| Behavior      | Halt motion; update internal offset so `Fp#` reports `n`; **no** TMC move; logical position written to NVS key `"position"` |
| Use cases     | Calibration, power-up alignment when mechanical position is known, limit-switch endpoint labeling        |


### 5. Read position `Fp#`


| Item    | Value                                                |
| ------- | ---------------------------------------------------- |
| Host    | `Fp#` (lowercase `p`)                                |
| Reply   | `Fpn#\r\n` where `n` is the current logical position |
| Example | Host `Fp#` → `Fp12345#\r\n`                          |


### 6. Read motion state `FQ#`


| Item  | Value                                    |
| ----- | ---------------------------------------- |
| Host  | `FQ#`                                    |
| Reply | `FQ0#\r\n` (idle) or `FQ1#\r\n` (moving) |


### 7. Read max travel `Fm#`


| Item  | Value                                                    |
| ----- | -------------------------------------------------------- |
| Host  | `Fm#` (lowercase `m`; `FM#` accepted as alias)           |
| Reply | `Fm100000#\r\n` (example; actual value from NVS/runtime) |


### 8. Set max travel `FMn#`


| Item          | Value                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------- |
| Example host  | `FM80000#`                                                                                              |
| Success reply | `FM80000#\r\n`                                                                                          |
| Legacy alias  | `FX80000#` → `FX80000#\r\n` (deprecated; use `FMn#`)                                                    |
| Valid range   | `100 ≤ n ≤ 1000000`, otherwise `FE#\r\n`                                                                |
| Behavior      | Halt, write NVS `max_step`, **logical position unchanged** (no offset or motor register reset); re-read `Fm#` and `Fp#` after success |
| Rejection     | Returns `FE#\r\n` when current logical position (`Fp#`) > `n`; `max_step` is not modified               |
| Recommended setup | After full retraction send `FY0#` → extend to mechanical limit → `FMn#` (`n` is usually current position) |


### 9. Read firmware version `FV#`


| Item  | Value          |
| ----- | -------------- |
| Host  | `FV#`          |
| Reply | `FV1.0.0#\r\n` |


### 10. Read model name `FI#`


| Item  | Value              |
| ----- | ------------------ |
| Host  | `FI#`              |
| Reply | `FIAskar-WAF#\r\n` |


### 11. Read backlash offset `Fb#`


| Item     | Value                                                                                      |
| -------- | ------------------------------------------------------------------------------------------ |
| Host     | `Fb#` (lowercase `b`)                                                                      |
| Reply    | `Fbn#\r\n` where `n` is the backlash stored in NVS                                         |
| Range    | `0 ≤ n ≤ 10000`                                                                            |
| Note     | On direction reversal, firmware compensates by exactly `n` steps; `n = 0` disables compensation |


### 12. Set backlash offset `FBn#`


| Item          | Value                                                                                      |
| ------------- | ------------------------------------------------------------------------------------------ |
| Example host  | `FB50#`                                                                                    |
| Success reply | `FB50#\r\n` (echoes clamped value)                                                         |
| Valid range   | `0 ≤ n ≤ 10000` (values outside range are clamped)                                         |
| Behavior      | Updates runtime backlash and NVS key `"backlash"`; does not trigger motion                 |


### 13. Read reverse motion `Fr#`


| Item     | Value                                                                 |
| -------- | --------------------------------------------------------------------- |
| Host     | `Fr#` (lowercase `r`; `FR#` accepted as alias)                        |
| Reply    | `Fr0#\r\n` (normal) or `Fr1#\r\n` (reversed)                          |
| Behavior | Read-only; does not change motor direction or stored setting          |


### 14. Set reverse motion `FR0#` / `FR1#`


| Item          | Value                                                                                                                       |
| ------------- | --------------------------------------------------------------------------------------------------------------------------- |
| Example host  | `FR0#` (normal), `FR1#` (reversed)                                                                                          |
| Success reply | `FR0#\r\n` or `FR1#\r\n`                                                                                                    |
| Behavior      | Halts motion; toggles TMC5130 motor direction (shaft bit); writes NVS key `"cdc_dir_rev"`                                   |
| Semantics     | Logical coordinates unchanged (`Fp#`, `FPn#`, `FYn#`); **all** moves (`FP`, `FT`) use inverted motor rotation when enabled |
| Use case      | Helical focusers mounted in either orientation; e.g. at logical `0` (retracted), `FP100#` extends the tube when reversed    |


### 15. Command failure `FE#`


| Condition                                                  | Reply                                                 |
| ---------------------------------------------------------- | ----------------------------------------------------- |
| Unrecognized `F…#` command                                 | `FE#\r\n`                                             |
| Invalid numeric field in `FP` / `FT` / `FY` / `FB` / `FM`  | `FE#\r\n`                                             |
| `FMn#` / `FXn#` with `n` not in `100 … 1000000`            | `FE#\r\n`                                             |
| `FMn#` / `FXn#` when current logical position > `n`         | `FE#\r\n`                                             |
| `FMn#` / `FXn#` parse failure or `focuser_apply_max_step()` failure | `FE#\r\n`                                    |
| Frame length ≥ 32 bytes                                    | `FE#\r\n` (first character dropped, resync continues) |


---

## Logical position persistence (NVS)

The firmware persists the **logical position** (`Fp#` reply) in NVS key `"position"`. After power-up the encoder reads zero; the firmware restores logical coordinates via an internal offset.

| Event | Writes NVS? |
| ----- | ----------- |
| Absolute/relative move completes | Yes |
| `FS#` halt | Yes |
| `FYn#` SYNC | Yes |
| `FMn#` set max travel | No (updates `max_step` only; does not change `position`) |
| Stall detection stop | Yes |
| During motion | **No** (power loss mid-move may restore last stationary position) |

Boot restore order: read `max_step` first (`Fm#` / NVS `"max_step"`), then read `"position"`. If no saved value exists, default logical position is **`max_step/2`**; values outside the current travel range are clamped to `[0, max_step]`.

---

## Typical sequences

### Connection handshake (recommended for INDIGO driver)

```
Host → FV#
Device ← FV1.0.0#

Host → FI#
Device ← FIAskar-WAF#

Host → Fm#
Device ← Fm100000#

Host → Fp#
Device ← Fp50000#

Host → Fr#
Device ← Fr0#
```

### Absolute move and wait for completion

```
Host → FP52000#
Device ← FP52000#

Loop:
  Host → FQ#
  Device ← FQ1#          ; still moving
  …
  Host → FQ#
  Device ← FQ0#

  Host → Fp#
  Device ← Fp52000#      ; logical position matches target (small mechanical tolerance OK)
```

### Relative move

```
Host → Fp#
Device ← Fp50000#

Host → FT+200#
Device ← FT+200#

; Poll FQ# / Fp# until FQ0# and Fp ≈ 50200
```

### Abort

```
Host → FS#
Device ← FS#

Host → Fp#
Device ← Fp50123#        ; position after stop
```

### Position restore after power-up

```
Host → Fp#
Device ← Fp52000#        ; restores last logical position from NVS, or max_step/2 if none

; If logical position does not match mechanics, calibrate manually:
Host → FY12000#          ; user knows true logical position
Device ← FY12000#

Host → Fp#
Device ← Fp12000#

Host → FQ#
Device ← FQ0#            ; focuser did not move
```

---

## INDIGO property mapping (reference)


| INDIGO property                              | CDC usage                                                           |
| -------------------------------------------- | ------------------------------------------------------------------- |
| `CONNECTION` / `DEVICE_PORT`                 | Open CDC port; on connect send `FV#`, `FI#`, `Fm#`                  |
| `FOCUSER_POSITION`                           | GOTO: `FPn#` + poll `Fp#`, `FQ#`; SYNC: `FYn#` (no motion)          |
| `FOCUSER_ON_POSITION_SET`                    | SYNC item → `FYn#`; GOTO item → `FPn#`                              |
| `FOCUSER_STEPS` + `FOCUSER_DIRECTION`        | `FT+n#` / `FT-n#`                                                   |
| `FOCUSER_ABORT_MOTION`                       | `FS#`                                                               |
| `FOCUSER_LIMITS`                             | `Fm#` read max; `FMn#` write max (100–1000000); min fixed at 0    |
| `INFO`                                       | `FI#`, `FV#`                                                        |
| Failure                                      | Reply `FE#` → set property `ALERT`                                  |
| `FOCUSER_BACKLASH`                           | `Fb#` read backlash; `FBn#` write backlash (0–10000); `0` disables compensation; requires firmware ≥ 1.0.3 |
| `FOCUSER_REVERSE_MOTION`                     | `Fr#` read; `FR0#` / `FR1#` write (persistent); inverts motor for `FP` and `FT` |
| `FOCUSER_SPEED` / `TEMPERATURE`              | Not implemented on CDC; hide in driver                            |


All positions are **logical steps** in `[0, max_step]`. On power-up, the firmware restores the last saved logical position from NVS if present; otherwise the default is **mid-travel** (`max_step/2`). Use `FYn#` to align with mechanics when needed (e.g. `FY0#` when fully retracted). Setting max travel keeps the logical position unchanged; recommended setup is `FY0#` first, extend to the limit, then `FMn#`. Reverse motion does not remap coordinates — it only inverts physical motor rotation for moves. Poll `FQ#` and `Fp#` after moves.