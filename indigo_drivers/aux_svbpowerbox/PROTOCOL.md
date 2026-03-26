# SVBONY PowerBox – Serial Communication Protocol

Reverse-engineered.

---

## Connection Parameters

| Parameter    | Value    |
|-------------|----------|
| Baud rate   | 115200   |
| Data bits   | 8        |
| Parity      | None     |
| Stop bits   | 1        |
| Flow control| None     |

---

## Device Identification / Startup Sequence

The device is based on an ESP32 microcontroller.  When the serial port is opened the
host **must** clear both RTS and DTR lines to let the device reset cleanly:

```c
ioctl(fd, TIOCMGET, &flags);
flags &= ~(TIOCM_RTS | TIOCM_DTR);
ioctl(fd, TIOCMSET, &flags);
```

After the reset the ESP32 emits a multiline ASCII boot log over the serial port.  The
host must drain these lines (typically several hundred milliseconds) before sending any
binary command.  The boot text contains strings such as `rst:`, `load:`, `len:`,
`POW`, etc.  Once the port goes silent, flush the buffers and proceed with the
handshake.

**Handshake**: send command `0x08` (Read State) and verify that the device responds
with a valid 10-byte payload.  A successful response confirms the device is ready.

---

## Frame Format

All traffic uses a simple binary framing:

### Request (host → device)

```
+--------+----------+-------------------+----------+
| 0x24   | data_len | cmd [+ args]      | checksum |
+--------+----------+-------------------+----------+
  1 byte   1 byte     1..3 bytes          1 byte
```

| Field      | Description |
|-----------|-------------|
| `0x24`    | Frame header (ASCII `$`) |
| `data_len`| Total number of bytes in the frame including `0x24`, `data_len` itself, all command bytes and the trailing checksum.  `data_len = 2 + cmd_len + 1` |
| `cmd`     | Command byte, followed by optional argument bytes |
| `checksum`| `( 0x24 + data_len + cmd + args… ) % 0xFF` — sum of every preceding byte, lower 8 bits, but clamped to 0x00–0xFE |

### Response (device → host)

```
+--------+----------+-----------+------------------+----------+
| 0x24   | data_len | cmd_echo  | result bytes     | checksum |
+--------+----------+-----------+------------------+----------+
  1 byte   1 byte     1 byte      0..10 bytes        1 byte
```

| Field       | Description |
|------------|-------------|
| `0x24`     | Frame header |
| `data_len` | Same framing rule as request |
| `cmd_echo` | Echoed command byte.  `0xAA` indicates the command was rejected / failed |
| `result`   | Zero or more data bytes (command-specific) |
| `checksum` | Same algorithm applied to all preceding bytes of the response |

**The response is considered successful only if `cmd_echo ≠ 0xAA` and the checksum
matches.**

---

## Checksum Algorithm

```
checksum = 0
for each byte b in (frame_header .. last_data_byte):
    checksum += b
checksum = checksum % 0xFF
```

The result is always in range 0x00–0xFE (0xFF is never produced because the modulus is
0xFF, not 0x100).

---

## Sensor Value Encoding

All 4-byte sensor responses encode a 32-bit big-endian unsigned integer that must be
divided by a scale factor to obtain the physical value:

```
raw = (res[0] << 24) | (res[1] << 16) | (res[2] << 8) | res[3]
value = (double)raw / scale
```

Individual sensor offsets are applied after the division (see the command table below).

---

## Command Reference

### 0x01 – Set Port State

Set the state or duty-cycle of a single output port.

**Request:** 3 command bytes

```
cmd[0] = 0x01
cmd[1] = port index  (see Port Index table)
cmd[2] = value       (0x00 = off, 0xFF = fully on; or 0x00–0xFF for PWM/voltage)
```

Full frame (5 bytes total):

```
24 05 01 <port> <value> <chk>
```

**Response:** 2 result bytes (both are status/padding; currently unused by host).

Full frame (6 bytes total):

```
24 06 01 <b0> <b1> <chk>
```

#### Port Index Table

| Index | Port                            | Value meaning |
|-------|--------------------------------|---------------|
| 0     | DC1                             | 0x00 = OFF, 0xFF = ON |
| 1     | DC2                             | 0x00 = OFF, 0xFF = ON |
| 2     | DC3                             | 0x00 = OFF, 0xFF = ON |
| 3     | DC4                             | 0x00 = OFF, 0xFF = ON |
| 4     | DC5                             | 0x00 = OFF, 0xFF = ON |
| 5     | USB group A (USB-C, port 1, 2)  | 0x00 = OFF, 0xFF = ON |
| 6     | USB group B (USB 3, 4, 5)       | 0x00 = OFF, 0xFF = ON |
| 7     | Regulated DC output             | PWM 0–255 → 0–15.3 V (`V = raw × 15.3 / 255`) |
| 8     | PWM-A (dew heater 1)            | PWM 0–255 → 0–100 % |
| 9     | PWM-B (dew heater 2)            | PWM 0–255 → 0–100 % |

---

### 0x02 – Read INA219 Power

Read the total input power measured by the INA219 sensor.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 02 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 02 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw_mW = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
power_mW = raw_mW / 100.0
power_W  = power_mW / 1000.0
```

---

### 0x03 – Read INA219 Load Voltage

Read the bus/load voltage measured by the INA219 sensor.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 03 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 03 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
voltage_V = raw / 100.0
```

---

### 0x04 – Read DS18B20 Temperature

Read the lens/ambient temperature from the DS18B20 one-wire probe.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 04 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 04 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
temp_C = round((raw / 100.0 - 255.5) * 100) / 100
```

The offset of **−255.5** compensates for the device's internal fixed-point bias.

---

### 0x05 – Read SHT40 Temperature

Read the ambient temperature from the SHT40 combined temperature/humidity sensor.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 05 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 05 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
temp_C = round((raw / 100.0 - 254.0) * 10) / 10
```

---

### 0x06 – Read SHT40 Humidity

Read the relative humidity from the SHT40 sensor.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 06 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 06 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
humidity_pct = round((raw / 100.0 - 254.0) * 10) / 10
```

---

### 0x07 – Read INA219 Current

Read the total input current measured by the INA219 sensor.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 07 <chk>
```

**Response:** 4 result bytes.  Full frame (8 bytes):

```
24 08 07 <b0> <b1> <b2> <b3> <chk>
```

Decode:

```
raw_mA = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
current_mA = raw_mA / 100.0
current_A  = current_mA / 1000.0
```

---

### 0x08 – Read All Port States

Read the on/off and PWM states of all output ports in a single query.  Also used as
the **handshake** command on connection.

**Request:** 1 command byte.  Full frame (4 bytes):

```
24 04 08 <chk>
```

**Response:** 10 result bytes.  Full frame (14 bytes):

```
24 0E 08 <r0> <r1> <r2> <r3> <r4> <r5> <r6> <r7> <r8> <r9> <chk>
```

#### Response Byte Map

| Byte | GPIO    | Description                  | Interpretation |
|------|---------|------------------------------|----------------|
| r0   | GPIO1   | DC1 state                    | 0x00 = OFF, non-zero = ON |
| r1   | GPIO2   | DC2 state                    | 0x00 = OFF, non-zero = ON |
| r2   | GPIO3   | DC3 state                    | 0x00 = OFF, non-zero = ON |
| r3   | GPIO4   | DC4 state                    | 0x00 = OFF, non-zero = ON |
| r4   | GPIO5   | DC5 state                    | 0x00 = OFF, non-zero = ON |
| r5   | GPIO6   | USB group A (USB-C, 1, 2)    | 0x00 = OFF, non-zero = ON |
| r6   | GPIO7   | USB group B (USB 3, 4, 5)    | 0x00 = OFF, non-zero = ON |
| r7   | pwmReg  | Regulated output PWM         | `V = r7 × 15.3 / 255.0`  |
| r8   | pwmA    | Dew heater 1 (PWM-A) duty    | `duty% = round(r8 × 100 / 255)` |
| r9   | pwmB    | Dew heater 2 (PWM-B) duty    | `duty% = round(r9 × 100 / 255)` |

When `r7 == 0` the regulated output is considered OFF.  When `r7 > 0` the output is
ON at the corresponding voltage.

---

## Annotated Frame Examples

### Example 1 – Set DC1 ON

```
TX:  24 05 01 00 FF 24
     ^^                  frame header
        ^^               data_len = 5
           ^^            cmd = 0x01 (set port)
              ^^         port index = 0 (DC1)
                 ^^      value = 0xFF (ON)
                    ^^   checksum = (0x24+0x05+0x01+0x00+0xFF) % 0xFF = 0x28     (example)

RX:  24 06 01 00 00 2B
     ^^                  frame header
        ^^               data_len = 6
           ^^            cmd_echo = 0x01 (success, not 0xAA)
              ^^ ^^      result bytes (padding)
                    ^^   checksum
```

### Example 2 – Read State (0x08)

```
TX:  24 04 08 30
     ^^             frame header
        ^^          data_len = 4
           ^^       cmd = 0x08
              ^^    checksum = (0x24+0x04+0x08) % 0xFF = 0x30

RX:  24 0E 08 FF FF FF FF FF FF FF C2 00 00 CC
     ^^                                          frame header
        ^^                                       data_len = 14
           ^^                                    cmd_echo = 0x08
              ^^ ^^ ^^ ^^ ^^ ^^ ^^              DC1–DC5 ON, USB-A ON, USB-B ON
                                ^^ ^^           reg=0xC2 → ~11.4V, pwmA=0x00, pwmB=0x00
                                         ^^     checksum
```

---

## Timing

| Interval | Value |
|---------|-------|
| Inter-command delay (after write) | 100 ms |
| Response read timeout | 500 ms |
| State polling interval | 1 s |

---

## Error Handling

- If `cmd_echo` in the response equals `0xAA`, the command was rejected by the firmware.
- If the response checksum does not match, the frame is discarded.
- If the response times out (500 ms), the command is considered failed.
