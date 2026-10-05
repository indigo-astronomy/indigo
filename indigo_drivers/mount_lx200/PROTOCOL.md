# LX200 mount dialects: command reference

This is a reference of the serial commands of every mount dialect the INDIGO LX200 driver
(`indigo_mount_lx200`) supports. Each table row is one function, each column one dialect, and each
cell lists the command(s) of that dialect for that function with the reply. The tables are complete
as far as the sources below go: commands the driver never sends are listed as well, so the tables
also show what a dialect could do beyond what the driver uses.

The fifteen dialects are split into three groups of five so that every table stays readable; each
function section has one table per group, in this order:

1. Meade LX200 Classic, Meade Autostar / Autostar II / LX200GPS (Meade 2010 protocol), Losmandy Gemini
   (Level 4 and Level 5), 10micron, Astro-Physics GTO.
2. Avalon StarGO, Avalon StarGO2, OnStep / OnStepX, Pegasus NYX-101, ZWO AM5 / AM3.
3. TeenAstro, aGotino, OpenAstroTracker (OAT), ESP32Go, Generic.

A function none of the five dialects of a group has is left out of that group's table.

## Legend

| Mark | Meaning |
| --- | --- |
| ✓ | The driver sends this command to this dialect. A command without ✓ exists in the dialect but the driver does not use it. |
| ‡ | Not in the dialect's document listed below; the fact comes from the driver source. StarGO, StarGO2, ESP32Go and Generic have no public protocol document; their columns are not marked ‡. |
| → | Separates a command from its reply. |
| ∅ | The command has no reply. |
| — | The dialect has no such command (or none is documented). |
| (X) | OnStep column: OnStepX only. (L5) / (L5.1): Gemini Level 5 and later only. |

Reply notation follows the protocol documents: `HH` hours, `MM` minutes, `SS` seconds, `T` tenths of a
minute, `s` sign, `DD`/`DDD` degrees, `n` a digit. Every command starts with `:` and ends with `#`
unless stated otherwise. A reply ends with `#` only where the `#` is shown: single-character
replies such as `0`/`1` and `Ok` have **no** terminator. In a reply, `*` stands for the degree mark,
which is the byte `0xDF` (written `{0xDF}` where it matters) on the Meade family (and on a 10micron in LX200 emulation) and the ASCII
asterisk `*` on most other controllers; see the footnotes for deviations.

Where a cell shows the command format the driver sends, it is the exact format: right ascension is
written as `:SrHH:MM:SS#`, declination as `:SdsDD*MM:SS#`, latitude as `:StsDD*MM#` and longitude
as `:SgDDD*MM#` (0 to 359, positive to the west) unless the cell says otherwise.

## Sources

* `Meade-LX200_Classic_Manual.pdf`: Meade LX200 Classic instruction manual, LX200 command set chapter.
* `Meade-2010.10.pdf`: Meade Telescope Serial Command Protocol, revision 2010.10 (Autostar, Autostar II, LX200GPS).
* `gemini_manual_l4.pdf`: Gemini users manual, Level 4, serial line protocol.
* `Gemini-5-2.1.pdf`: Gemini Level 5, version 2.1, serial interface command description.
* `10micron-2.13.2.pdf`: 10micron Mount Command Protocol, software version 2.13.2.
* `AstroPhysics-D.pdf`: Astro-Physics GTOCP1/GTOCP2 command language (chip versions C to KE1).
* `AstroPhysics-G.pdf`: Astro-Physics GTOCP3 command language (chip versions G to L).
* `AstroPhysics-GTOCP4.pdf`: Astro-Physics GTOCP3 and GTOCP4 command language.
* `OnStep.pdf`: OnStep command protocol.
* `OnStepX.pdf`: OnStepX command reference.
* `NYX101.pdf`: Pegasus Astro NYX-101 command protocol 1.0.
* `ZWO.pdf`: ZWO mount serial communication protocol, version 2.1.
* `OAT Command Index.webloc`: OpenAstroTech MEADE command index (firmware V1.13.16).
* `TeenAstro Command Index.webloc`: TeenAstro LX200 command list.
* `aGotino Command Index.webloc`: the LX200 command parser of the aGotino sketch.
* Avalon Instruments StarGO user manual, version 4.1.4.
* `indigo_mount_lx200.driver` (driver source) and `REFACTOR.md` in this folder: what the driver
  sends, and the replies recorded from the controllers the driver was run against. StarGO2 and
  ESP32Go have no protocol document here and are described from these alone.

## Transport and connection

| Dialect | Serial | Network | Notes |
| --- | --- | --- | --- |
| LX200 Classic | 9600 8N1 | — | No `:GVP#`, must be selected explicitly. |
| Autostar / LX200GPS | serial | — | `:SBn#` changes the speed. |
| Gemini | serial | L5: Ethernet | Driver default TCP port 4030. |
| 10micron | 9600 8N1 | TCP 3490, 3492 | Several connections per port from 2.9.10. |
| Astro-Physics GTO | 9600 8N1 | — | Not autodetected (no `:GVP#`); detected by a `:V#` reply starting with `VCP`. |
| StarGO | 9600 8N1 | TCP 4030 (Wi-Fi model) | Sends `:Z1…#`, `:Z3…#` and `ge#` unsolicited (see section 12). |
| StarGO2 | — | TCP, default `StarGo2.local:9624` | Not autodetected. |
| OnStep / OnStepX | serial | TCP 9999 (driver default) | `:SBn#` up to 460800 on OnStepX. |
| NYX-101 | 115200 8N1 (USB) | TCP 9996 to 9999; 9999 for LX200 clients | |
| ZWO AM5 / AM3 | 9600 8N1 | TCP 4030 (hand controller Wi-Fi) | USB VID 0x03C3, PID 0x4001. |
| TeenAstro | serial | TCP 4030 (driver default) | |
| aGotino | serial | — | |
| OpenAstroTracker | 19200 8N1 | — | |
| ESP32Go | 115200 8N1 | TCP 10001 | About 12 s after a reset before it answers. |
| Generic | serial | TCP 4030 (driver default) | |

On a serial port the driver probes with `:GR#` ✓ (a reply of at least 6 characters counts) and
cycles through 9600, 19200 and 115200 baud. Over TCP it sends `:GR#` ✓ every 5 s as a keep-alive
while the mount device itself is not connected. With "Autodetect" it then sends `:GVP#` ✓ and picks
the dialect from the product name; a mount that does not answer `:GVP#` is probed with `:V#`
(Astro-Physics) and then with the core set `:GR#` `:GD#` `:GC#` `:GL#` `:GG#` `:GS#` `:Gg#` `:Gt#`
(Generic).

## 1. Identification

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Alignment / mount mode query (ACK byte `0x06`, no `:` and no `#`) | —[^classic] | `0x06` → `A` alt-az, `L` land, `P` polar, `D` downloader (no `#`) ✓[^ack] | `0x06` → `B#` startup message, `b#` waiting for startup mode, `S#` cold start, `G#` ready (equatorial), `A#` ready (alt-az, L5.1) ✓ | `0x06` → `L` tracking off, `P` tracking on | — |
| Select startup mode (while ACK answers `b#`) | — | — | `bC#` cold start, `bW#` warm start, `bR#` warm restart → ∅ ✓ | — | — |
| Clear the input buffer | — | — | — | `#` | `#` → ∅ ✓ |
| Product name | — | `:GVP#` → `<string>#` (e.g. `Autostar#`, `LX2001#` for the LX200GPS) ✓ | `:GVP#` → `Losmandy Gemini#` ✓ | `:GVP#` → `10micron GM1000HPS#`, `10micron GM2000QCI#`, … ✓ | — (no reply; autodetection then tries `:V#`) ✓ |
| Firmware number | — | `:GVN#` → `dd.d#` (e.g. `43Eg#`) ✓ | `:GVN#` → `l.vv#` (level, version) ✓ | `:GVN#` → `<string>#` | — |
| Firmware date / time | — | `:GVD#` → `mmm dd yyyy#`<br>`:GVT#` → `HH:MM:SS#` | `:GVD#` → `mm dd yyyy#`<br>`:GVT#` → `hh:mm:ss#` (L4: `hh mm ss#`) | `:GVD#` → `mmm dd yyyy#`<br>`:GVT#` → `HH:MM:SS#` | — |
| Other version and identity queries | — | `:GVF#` → `<model>`&#124;`<…>#` ✓‡ | `:GV#` → `<l><vv>#` (level digit, two-digit version) ✓ | `:GVZ#` → `Q-TYPE2012#`, `PRE2012#`, `UNKNOWN#`<br>`:V#` → `G#`<br>`:GETID#` → 20-digit id `#` | `:V#` → servo version `#` (e.g. `VCP4-P02-15#` on a GTOCP4‡) ✓ |
| Reset, restart, initialize | — | `:I#` (LX200GPS: restart initialization)<br>`0x04` (EOT) firmware downloader | native `>65533:` cold-start reboot, `>65534:` / `>65535:` reboot | `:shutdown#` → `0`/`1` | `:de#`, `:dn#` transparent feed-through to the keypad → ∅ |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Alignment / mount mode query (ACK byte `0x06`, no `:` and no `#`) | — | — | `0x06` → `A`, `P`, … (X) | — | — |
| Clear the input buffer | — | — | — | `#` | — |
| Product name | `:GVP#` → `Avalon#` ✓ | — | `:GVP#` → `On-Step#` ✓ | `:GVP#` → `NYX-101#` ✓ | `:GVP#` → `AM5#` / `AM3#`‡ ✓ |
| Firmware number | `:GVN#` → `nn.n#` | — | `:GVN#` → `3.16o#` / `M.mm…#` ✓ | `:GVN#` → `n.nn#` ✓ | — |
| Firmware date / time | `:GVD#` → `d` followed by eight characters `#` | — | `:GVD#` → `MM DD YY#` (X: `MTH DD YYYY#`)<br>`:GVT#` → `HH:MM:SS#` | `:GVD#` → `MTH DD YYYY#`<br>`:GVT#` → `HH:MM:SS#` | — |
| Other version and identity queries | `:GW#` → `PT0#` equatorial, `AT0#` alt-az<br>`:X05#` → `U#` USB, `B#` Bluetooth<br>`:X46r#` → `c1#` configured, `c0#` not configured<br>`:X29#` → firmware TCB `#`<br>`:GVF#` | — | `:GVM#` → `name version#` (X)<br>`:GVC#` → configuration `#` (X)<br>`:GVH#` → pin map `#` (X) | `:GVM#` → `NYX-101 n.nn`<br>`:GVU#` → `PEG_NYX-101:xxxxxxxx`<br>`:GVY#` → uptime in s | `:GV#` → firmware version `#` ✓‡<br>`:GVT#` → firmware compile time `#`<br>`:GVE#` → hand controller firmware version `#`<br>`:GMA#` → serial number `#`<br>`:GBl#` → `AMx_nnnnnn#` (Bluetooth name) |
| Reset, restart, initialize | `:XFF#` restart the controller, `:XFE#` restart into the firmware loader → ∅<br>`:X461#` / `:X460#` set / clear the configured flag | — | `:ERESET#` (X)<br>`:ENVRESET#` (X)<br>`:ESPFLASH#` (X) | `:ERESET#` → ∅<br>`:ENVCLEAR#` → ∅ | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Alignment / mount mode query (ACK byte `0x06`, no `:` and no `#`) | — | `0x06` → `P` (no `#`) | — | — | — |
| Product name | `:GVP#` → `TeenAstro#` ✓ | `:GVP#` → `aGotino#` ✓ | `:GVP#` → `OpenAstroTracker#` or `OpenAstroMount#` ✓ | `:GVP#` → `esp32go#` ✓ | `:GVP#` (any other reply, or none) ✓ |
| Firmware number | `:GVN#` → `1.5.7#` ✓ | `:GVN#` → `230312#` ✓ | `:GVN#` → `V1.major.minor#` ✓ | `:GVN#` → `06.9#` ✓ | — |
| Firmware date / time | `:GVD#` → `MM DD YY#`<br>`:GVT#` → `HH:MM:SS#` | every other `:GV…#` → `#` | — | `:GVD#`, `:GVT#` → build date / time `#` | — |
| Other version and identity queries | `:GVp#` → `Universal#`<br>`:GVB#` → PCB version `#`<br>`:GVb#` → stepper driver `n#` | — | `:XGM#` → `<board>,<RA stepper>,<DEC stepper>,<GPS>,<AzAlt>,<gyro>,<display>,…#`<br>`:XGMS#` → driver configuration `#` | `:GVF#` → `43Eg#` | — |
| Reset, restart, initialize | `:$$#` reset EEPROM and reboot → `1`<br>`:$!#` reboot → `1`<br>`:$X#` reinit encoders and motors → `1` | — | `:I#` serial control mode → ∅<br>`:Qq#` leave control mode, start tracking → ∅<br>`:XFR#` factory reset → `1#` | `:cRR#` saves the position and restarts the controller | — |

## 2. Precision and format

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Coordinate precision toggle / select | — | `:U#` → ∅ (toggles `HH:MM.T` `sDD*MM` ↔ `HH:MM:SS` `sDD*MM:SS`) | `:U#` → ∅ toggle (high precision after startup) ✓[^short]<br>`:u#` → ∅ double precision, signed decimals (L5)[^gemfmt] | `:U#` → ∅ toggle<br>`:U0#` low, `:U1#` high ✓, `:U2#` ultra → ∅ | `:U#` → ∅ long format; one-way, per port ✓ |
| High precision pointing / precision report | — | `:P#` → `HIGH PRECISION` or `LOW PRECISION` (no `#`) ✓[^short] | `:P#` → `HIGH PRECISION`, `LOW  PRECISION` or `DBL  PRECISION` (14 characters, no `#`) | `:P#` does nothing | — |
| Emulation mode | — | — | — | `:EMUAP#` Astro-Physics compatible (extended) ✓<br>`:EMULX#` LX200 → ∅ | — |
| Precession and refraction of transferred coordinates | — | — | `:p0#` none ✓, `:p1#` precess from J2000, `:p2#` refraction, `:p3#` both → ∅<br>`:Sp#` precess the target → `No object!#` / `1` | `:SREFn#` / `:GREF#` refraction on/off → `0`/`1` | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Coordinate precision toggle / select | `:X590#`, `:X591#` select the coordinate report mode (see section 3) | — | `:U#` → ∅ ✓[^short] | — | — |
| Precession and refraction of transferred coordinates | — | — | `:Tr#`, `:Tn#` (see tracking) | `:Tr#` (see tracking) | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Coordinate precision toggle / select | `:U#` ✓‡[^short] | — | — | — | — |
| High precision pointing / precision report | — | — | `:P#` → ∅ ✓‡[^short] | — | — |
| Precession and refraction of transferred coordinates | `:SXrp,V#`, `:SXrg,V#`, `:SXrt,V#` → `1`; `:GXrp,V#`, `:GXrt,V#` → `y#`/`n#` | — | — | — | — |

## 3. Coordinates (get)

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Current right ascension | `:GR#` → `HH:MM.T#` ✓ | `:GR#` → `HH:MM.T#` or `HH:MM:SS#` ✓ | `:GR#` → `HH:MM:SS#`, low `HH:MM.M#`, double `±hh.hhhhhh#` ✓ | `:GR#` → low `HH:MM.M#`, LX200 high `HH:MM:SS#`, extended high `HH:MM:SS.S#`, ultra `HH:MM:SS.SS#` ✓ | `:GR#` → `HH:MM.M#`, long `HH:MM:SS.S#` ✓ |
| Current declination | `:GD#` → `sDD*MM#` ✓ | `:GD#` → `sDD*MM#` or `sDD*MM'SS#` ✓ | `:GD#` → high `sDD:MM:SS#`, low `sDD°MM#` (`°` = `0xDF`), double `±dd.dddddd#` ✓[^gemfmt] | `:GD#` → LX200 `sDD*MM#`, extended `sDD*MM:SS#`, ultra `sDD:MM:SS.S#` ✓[^10mstar] | `:GD#` → `sDD*MM#`, long `sDD*MM:SS#` ✓ |
| Current position, highest precision | — | — | `:u#` mode (see section 2) | `:U2#` mode (see section 2) | — |
| Target right ascension | `:Gr#` → `HH:MM.T#` | `:Gr#` → `HH:MM.T#` or `HH:MM:SS#` | `:Gr#` (L5.1), formats as `:GR#` | `:Gr#`, formats as `:GR#` | — |
| Target declination | `:Gd#` → `sDD*MM#` | `:Gd#` → `sDD*MM#` or `sDD*MM'SS#` | `:Gd#` (L5.1), formats as `:GD#` | `:Gd#`, formats as `:GD#` | — |
| Altitude / azimuth | `:GA#` → `sDD*MM#`<br>`:GZ#` → `DDD*MM#` | `:GA#` → `sDD*MM#` or `sDD*MM'SS#`<br>`:GZ#` → `DDD*MM#` or `DDD*MM'SS#` | `:GA#` → `sDD:MM:SS#` / `sDD°MM#`<br>`:GZ#` → `DDD:MM:SS#` / `DDD°MM#` | `:GA#` → `sDD*MM#`, `sDD*MM:SS#`, `sDD:MM:SS.S#`<br>`:GZ#` → `DDD*MM#`, `DDD*MM:SS#`, `DDD:MM:SS.S#` | `:GA#` → `sDD*MM#` / `sDD*MM:SS#`<br>`:GZ#` → `sDD*MM#` / `sDD*MM:SS#` |
| Target altitude / azimuth | — | — | — | `:Ga#` → target altitude, `:Gz#` → target azimuth (formats as `:GA#`/`:GZ#`)[^conflict] | — |
| Other position queries | — | `:GE#` / `:Ge#` selenographic latitude / longitude → `sDD*MM#` / `sDDD*MM#` (LX200GPS, RCX) | `:GH#` hour angle → `[-]HH:MM:SS#`<br>native `<235:`…`<239:` axis positions | `:GaXa#` / `:GaXb#` axis angles → `sXXX.XXXX#`<br>`:QaXa#` / `:QaXb#` target axis angles | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Current right ascension | `:GR#` → `HH:MM:SS#` ✓ | `:GR#` ✓ | `:GR#` → `HH:MM:SS#` (low `HH:MM.T#`) ✓ | `:GR#` → `HH:MM:SS#` | `:GR#` → `HH:MM:SS#` ✓ |
| Current declination | `:GD#` → `sDD*MM:SS#` ✓ | `:GD#` ✓ | `:GD#` → `sDD*MM'SS#` (X: `sDD*MM:SS#`; low `sDD*MM#`) ✓ | `:GD#` → `sDD*MM:SS#` | `:GD#` → `sDD*MM:SS#` ✓ |
| Current position, highest precision | `RDHHhhhhhhsDDddddd#` (right ascension in decimal hours, declination in decimal degrees) and `ZAzzzzzzzzaaaaaaaa#` (azimuth, altitude in 10⁻⁵ °), the high precision replies of the `:X59x#` mode | — | `:GRH#` → `HH:MM:SS.SSSS#` (X)<br>`:GDH#` → `sDD*MM:SS.SSS#` (X) | `:GRH#` → `HH:MM:SS.SSSS#` ✓<br>`:GDH#` → `sDD*MM:SS.SSS#` ✓ | `:GMEQ#` → `HH:MM:SS&sDD*MM:SS#` |
| Target right ascension | — | — | `:Gr#` → `HH:MM:SS#`; `:GrH#` (X) | `:Gr#` → `HH:MM:SS`; `:GrH#` → `HH:MM:SS.SSS#` | `:Gr#` → `HH:MM:SS#` |
| Target declination | — | — | `:Gd#` → `sDD*MM'SS#`; `:GdH#` (X) | `:Gd#`; `:GdH#` → high precision | `:Gd#` → `sDD*MM:SS#` |
| Altitude / azimuth | `:GA#` → `sDD*MM'SS#`<br>`:GZ#` → `DDD*MM'SS#` | — | `:GA#` → `sDD*MM'SS#`, `:GZ#` → `DDD*MM'SS#`; `:GAH#`, `:GZH#` (X) | `:GA#` → `sDD*MM:SS#`, `:GZ#` → `DDD*MM.SS#`; `:GAH#`, `:GZH#` | `:GA#` → `sDD*MM:SS#`<br>`:GZ#` → `DDD*MM:SS#`<br>`:GMZA#` → `DDD*MM:SS&sDD*MM:SS#` |
| Target altitude / azimuth | — | — | `:Gal#`, `:GaH#`, `:Gz#`, `:GzH#` (X) | `:Gal#` → `sDD:MM:SS#`, `:GaH#`, `:Gz#` → `DDD*MM:SS#`, `:GzH#` | `:GMeq#` → target `HH:MM:SS&sDD*MM:SS#` |
| Other position queries | `:TTGMs0#` / `:TTGMs1#` → RA / DEC encoder steps `hhhhhhhhr#` / `hhhhhhhhd#` (hexadecimal, digits A … F sent as `:` … `?`) | — | `:GX40#`/`:GX41#` axis angles `DDD*MM:SS#`, `:GX42#`/`:GX43#` decimal, `:GX44#`/`:GX45#` encoder counts (X) | `:GX94#` motor position in steps | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Current right ascension | `:GR#` → `HH:MM:SS#` ✓ | `:GR#` → `HH:MM:SS#` ✓[^agotino] | `:GR#` → `HH:MM:SS#` ✓ | `:GR#` → `HH:MM:SS.S#` ✓[^esp32go] | `:GR#` ✓ |
| Current declination | `:GD#` → `sDD*MM'SS#` ✓ | `:GD#` → `sDD{0xDF}MM:SS#` ✓[^agotino] | `:GD#` → `sDD*MM'SS#` ✓[^oat] | `:GD#` → `sDD{0xE1}MM:SS#` ✓[^esp32go] | `:GD#` ✓ |
| Current position, highest precision | `:GRL#` → `VVV.VVVVV#`<br>`:GDL#` → `sVV.VVVVV#` | — | — | `:Gx#` → `:GR#`, `:GD#`, `:GZ#`, `:GA#`, `:GK#` and focuser in one reply | — |
| Target right ascension | `:Gr#` → `HH:MM:SS#`; `:GrL#` → `sDDD.VVVVV#` | — | `:Gr#` → `HH:MM:SS#` | — | — |
| Target declination | `:Gd#` → `sDD*MM'SS#`; `:GdL#` → `sVV.VVVVV#` | — | `:Gd#` → `sDD*MM'SS#` | — | — |
| Altitude / azimuth | `:GA#` → `sDD*MM'SS#`<br>`:GZ#` → `DDD*MM'SS#` | — | — | see `:Gx#` | — |
| Other position queries | — | — | `:XGAA#` → `azpos`&#124;`altpos#`<br>`:XGCn.nn*m.mm#` → `ralong,declong#` | — | — |

## 4. Target, slew and sync

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Set target right ascension | `:SrHH:MM.T#` → `1`/`0`; driver sends `:SrHH:MM:SS#` ✓ | `:SrHH:MM.T#`, `:SrHH:MM:SS#` → `1`/`0` ✓ | `:SrHH:MM.M#`, `:SrHH:MM:SS#` → `1`/`0` ✓ | `:SrHH:MM.T#`, `:SrHH:MM:SS#`, `:SrHH:MM:SS.S#`, `:SrHH:MM:SS.SS#` → `1`/`0` ✓ | `:Sr HH:MM:SS#`, `:Sr HH:MM:SS.S#` → `1`; driver sends `:SrHH:MM:SS.SS#` to a GTOCP4 from P01-04 and to GTOCP5/6‡ ✓ |
| Set target declination | `:SdsDD*MM#` → `1`/`0`; driver sends `:SdsDD*MM:SS#` ✓ | `:SdsDD*MM#`, `:SdsDD*MM:SS#` → `1`/`0` ✓ | `:SdsDD*MM#`, `:SdsDD*MM:SS#` (`*`, `°` or `:` after the degrees) → `1`/`0` ✓ | `:SdsDD*MM#`, `:SdsDD*MM:SS#`, `:SdsDD*MM:SS.S#` → `1`/`0` ✓ | `:Sd sDD*MM#`, `:Sd sDD*MM:SS#` → `1`; driver sends `:SdsDD*MM:SS.S#` to the same controllers‡ ✓ |
| Goto target (RA/Dec) | `:MS#` → `0`, or `1`, `2`, `4` followed by a message ✓ | `:MS#` → `0`, `1<string>#` below horizon, `2<string>#` below higher limit ✓ | `:MS#` → `0`, `1Object below horizon.#`, `2No object selected.#`, `3Manual Control.#`, `4Position unreachable.#`, `5Not aligned.#`, `6Outside Limits.#`, `7Rejected - Mount is parked!#` ✓[^gemms] | `:MS#` → `0`, `1Object Below Horizon        #`, `2Object Below Higher         #`, `3Cannot Perform Slew         #`, `4Mount Parked                #`, `5Object on the  other side   #` ✓ | `:MS#` → `0`, ∅ when refused, `1Object is below horizon        #` (horizon check on) ✓ |
| Set target altitude / azimuth | `:SasDD*MM#`, `:SzDDD*MM#` → `Ok` | `:SasDD*MM#` → `1`/`0`; `:SzDDD*MM#` → `0`/`1` | `:Sa…#`, `:Sz…#` (both formats) → `0`/`1` | `:SasDD*MM#` … `:SasDD*MM:SS.S#`, `:SzDDD*MM#` … `:SzDDD*MM:SS.S#` → `0`/`1` | `:Sa sDD*MM#`, `:Sz DDD*MM#` (short or long) → `1` (selects alt-az mode) |
| Goto target (alt/az) | `:MA#` → `0` (land and alt-az modes) | `:MA#` → `0`/`1` | `:MA#` → `0`, `1…#`, `2…#`, `3…#` | `:MA#` → as `:MS#` (no tracking afterwards) | `:MS#` in alt-az mode |
| Other gotos | — | — | `:MM#` goto with meridian flip if possible, `:Mf#` flip and slew to current coordinates → as `:MS#`<br>`:MF<n>#` meander search, `:ML#` / `:Ml#` lock / unlock gotos → `#` | `:MSfsn#` goto on side n (2 west, 3 east), `:MSnf#` goto ignoring the fine-movement limit, `:MaX#` goto axis angles → as `:MS#`<br>`:NUDGEsXXXX,sYYYY#` offset slew → `0`, `1…#`, `2…#`, `3Cannot Perform Nudge #` | — |
| Sync to target | `:CM#` → `<object>#` ✓ | `:CM#` → `<object>#` (Autostar: static ` M31 EX GAL MAG 3.5 SZ178.0'#`) ✓[^cmlate] | `:CM#` → `No object!#` or `<object name>#` ✓[^gemcm] | `:CM#` → `Coordinates     matched        #` or `Match fail: dist. too large#` ✓ | `:CM#` → `Coordinates     matched.        #` (32 characters + `#`) ✓ |
| Other syncs | — | `:CL#` sync on selenographic coordinates (Autostar II) | `:Cm#` additional alignment, `:CI#` initial alignment → `No object!#` / `<name>#` | `:CMR#` same as `:CM#`<br>`:CMS#` add alignment point → `V#` / `E#`<br>`:CMCFGn#` `:CM#` behaviour (0 offset, 1 refine) → `0#`/`1#` | `:CMR#` recalibrate, keeps the side of pier → `Coordinates     matched.        #` ✓[^apsync] |
| Stop all motion | `:Q#` → ∅ ✓[^classicq] | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓<br>`:STOP#` also stops tracking | `:Q#` → ∅ ✓ |
| Slew progress (distance bars) | `:D#` → bar string `#` | `:D#` → one bar while slewing, empty `#` when complete ✓[^dpark] | — | `:D#` → `{0x7F}#` while slewing (or within the settle time), `#` otherwise | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Set target right ascension | `:SrHH:MM:SS#`, `:Sr HH:MM:SS#` → `1` ✓ (driver: the first) | `:SrHH:MM:SS#` → `1` ✓ | `:SrHH:MM:SS#` → `0`/`1` ✓ (X: also `HH:MM.T`, `HH:MM:SS.SSSS`) | `:SrHH:MM:SS#` → `0`/`1` ✓ | `:SrHH:MM:SS#` → `1`/`0` ✓ |
| Set target declination | `:SdsDD*MM:SS#`, `:Sd sDD*MM:SS#` → `1` ✓ (driver: the first) | `:SdsDD*MM:SS#` → `1` ✓ | `:SdsDD:MM:SS#` (X: `:SdsDD*MM#`, `:SdsDD*MM:SS#`, `:SdsDD*MM:SS.SSS#`) → `0`/`1` ✓ | `:SdsDD*MM:SS#` → `0`/`1` ✓ | `:SdsDD:MM:SS#` → `1` / `e2#` ✓ |
| Goto target (RA/Dec) | `:MS#` → `0` ✓; `ge#` arrives unsolicited when the goto ends | `:MS#` → `0` ✓ | `:MS#` → `0` … `6` (X: `0` … `9`) ✓[^onsterr] | `:MS#` → `0` … `9` ✓[^onsterr] | `:MS#` → `0` or `e<n>#` ✓[^zwoerr] |
| Set target altitude / azimuth | `:SasDD*MM:SS#`, `:SzDDD*MM:SS#` | — | `:SasDD:MM:SS#`, `:SzDDD:MM:SS#` → `0`/`1` (X: `*` forms) | `:Sa[sDD*MM'SS]#`, `:Sz[DDD*MM'SS]#` → `0`/`1` | — |
| Goto target (alt/az) | `:X4Dzzzaa#` (azimuth `zzz`, altitude `aa`, whole degrees) → ∅ | — | `:MA#` → as `:MS#` | `:MA#` → `0` … `9` | — |
| Other gotos | `:X33mEEEEEEEE#` move axis m (`0` RA, `1` DEC) to an encoder position<br>`:X50DDMMYY#` date, then `:X51HHMMSS#` time and start of a goto to the Sun | — | `:MN#` same position on the other pier side, `:MNe#` / `:MNw#` force east / west, `:MP#` polar-align goto → `0` … `9` (X) | `:MN#`, `:MNe#`, `:MNw#` → `0` … `9` | `:SMeqHH:MM:SS&sDD*MM:SS#` set target and goto → `0` / `e<n>#` |
| Sync to target | `:CM#` → `<string>#` ✓ | `:CM#` → `<string>#` ✓ | `:CM#` → `N/A#` (X: or `E1#` … `E9#`) ✓ | `:CM#` → `N/A#` or `E<n>#` (code as `:MS#`) ✓ | `:CM#` → `N/A#` or `e<n>#` ✓ |
| Other syncs | `:X31HHMMSS#` (local sidereal time) then `:X351#` sync at the home position | — | `:CS#` → ∅ (refused syncs fail silently) | `:CS#` → ∅ | `:SMMCHH:MM:SS&sDD*MM:SS#` set target and sync → `N/A#` / `e<n>#` |
| Stop all motion | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ |
| Slew progress (distance bars) | `:D#` | — | `:D#` → `{0x7F}#` while moving, `#` otherwise | — | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Set target right ascension | `:SrHH:MM:SS#`, `:SrHH:MM.T#`, `:SrL,VVV.VVVVV#` → `0`/`1` ✓ | `:SrHH:MM:SS#`, `:SrHH:MM.T#` → `1` (not validated) ✓ | `:SrHH:MM:SS#` → `1`/`0` ✓ | `:SrHH:MM:SS#` → `1` ✓ | `:SrHH:MM:SS#` → `1` ✓ |
| Set target declination | `:SdsDD*MM:SS#`, `:SdsDD*MM#`, `:SdLsVV.VVVVV#` → `0`/`1` ✓ | `:SdsDD*MM:SS#`, `:SdsDD*MM#` → `1` ✓ | `:SdsDD*MM:SS#` → `1`/`0` ✓ | `:SdsDD*MM:SS#` → `1` ✓ | `:SdsDD*MM:SS#` → `1` ✓ |
| Goto target (RA/Dec) | `:MS#` → `0` … `9` ✓[^teenerr] | `:MS#` → `0`, later `1Range_too_big#` if the slew fails ✓ | `:MS#` → `0` ✓ | `:MS#` → `0#` ✓ | `:MS#` → `0` ✓ |
| Set target altitude / azimuth | `:SzDDD:MM:SS#`, `:SasDD:MM:SS#` → `0`/`1` | — | — | — | — |
| Goto target (alt/az) | `:MA#` → as `:MS#` | — | — | — | — |
| Other gotos | `:MU#` goto user target, `:MF#` meridian flip → as `:MS#`<br>`:SU#` store current position as user target → `1`<br>`:M@V#` spiral search → `0`/`1` | — | `:MXxnnnnn#` move a stepper by n steps → `1`/`0` | — | — |
| Sync to target | `:CM#` → `N/A#` ✓ | `:CM#` → `0#` ✓ | `:CM#` → `NONE#` ✓ | `:CM#` → `sync#` ✓ | `:CM#` → `<string>#` ✓ |
| Other syncs | `:CS#` → ∅<br>`:CA#` sync to alt/az target, `:CU#` sync to user target → `N/A#` | — | `:SYsDD*MM:SS.HH:MM:SS#` sync to the given coordinates → `1`/`0` | — | — |
| Stop all motion | `:Q#` → ∅ ✓ | `:Q#` (any `:Q…#`) → ∅ ✓[^agotino] | `:Q#` → ∅, stops all motors including tracking ✓ | `:Q#` → ∅ ✓ | `:Q#` → ∅ ✓ |
| Slew progress (distance bars) | `:D#` → `{0x7F}#` | `:D#` → `{0x7F}#` while slewing, `#` idle ✓ | `:D#` → &#124;`#` slewing, `#` idle | `:D#` → &#124;`#` while an axis slews, `#` idle | — |

## 5. Motion and rates

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Move north / south / east / west (until stopped) | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ (guide or centering rate) ✓ |
| Stop one direction | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` = `:Qs#`, `:Qe#` = `:Qw#` (per axis) → ∅ ✓ |
| Rate: guide / centering / find / slew | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` ✓, `:RC#`, `:RS#` (goto speed only, not the buttons) → ∅ |
| Rate presets by number | — | — | — | `:RG0#` 0.25x, `:RG1#` 0.5x, `:RG2#` 1x<br>`:RC0#` 16x, `:RC1#` 64x, `:RC2#` 600x, `:RC3#` 1200x<br>`:RS0#` 1200x, `:RS1#` 900x, `:RS2#` 600x → ∅ | `:RG0#` 0.25x, `:RG1#` 0.5x, `:RG2#` 1x<br>`:RC0#` 12x, `:RC1#` 64x ✓, `:RC2#` 600x ✓, `:RC3#` 1200x ✓<br>`:RS0#` 600x, `:RS1#` 900x, `:RS2#` 1200x → ∅ |
| Custom rates | — | `:RADD.D#` RA/Az, `:REDD.D#` Dec/Alt slew rate in °/s (Autostar II) → ∅ | `:Rm[s][n]#` move rate (default 50)<br>native `>120:`…`>147:` manual, goto and move speeds, `>170:`…`>172:` centering speed | `:RADD.D#`, `:REDD.D#` °/s<br>`:RcXXX#` centering 1–255x, `:RsXXXX#` slew 1–1200x → ∅<br>`:RMsXX#` automated slew rate °/s → `0`/`1` | `:Rcxxx#` centering 1–255x<br>`:Rsxxxx#` slew 1–1200x → ∅ |
| Maximum slew rate | `:SwN#` (N = 2 … 4 °/s) → `Ok` | `:SwN#` (N = 2 … 8 °/s) → `0`/`1` | `:Sw<n>#` → `1` | `:SwN#` → `0`/`1` | — |
| Rate queries | — | — | `:R?#` → `G#`, `C#`, `M#` or `S#` (L5) | `:GMs#` current, `:GMsa#` minimum, `:GMsb#` maximum slew rate → `XX#` | — |
| Other motion | — | — | `:mi<RA steps>;<DEC steps>#` move by encoder ticks, `:mm<multiplier>#` | `:NS#` swap north/south, `:EW#` swap east/west → ∅ | `:NS#`, `:EW#` swap buttons → ∅ |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Move north / south / east / west (until stopped) | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ |
| Stop one direction | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ |
| Rate: guide / centering / find / slew | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ (guide 0.05x … 0.95x, centering 2x … 10x, find 10x … 150x, slew depends on the mount) | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` 1x, `:RC#` 4x (X: 8x), `:RM#` 8x (X: 20x), `:RS#` 24x (X: half goto rate) → ∅; `:RF#` 48x (X) | `:RC#` 8x, `:RM#` 20x, `:RF#` 64x → ∅ | `:RG#` 0.5x, `:RC#` 1x, `:RM#` 720x, `:RS#` 1440x → ∅ |
| Rate presets by number | — | — | `:R0#` … `:R9#` → ∅: 0.25x, 0.5x, 1x, 2x, 4x, 8x, 16x, 24x, 40x, 60x; driver: `:R1#` guide, `:R4#` centering, `:R7#` find, `:R9#` slew ✓ | `:R0#` … `:R9#` → ∅: 0.25x, 0.5x, 1x, 2x, 8x, 20x, 64x, 128x, half max, max; driver as OnStep ✓ | `:R0#` … `:R9#` → ∅: 0.25x, 0.5x, 1x, 2x, 4x, 8x, 20x, 60x, 720x, 1440x; driver as OnStep ✓ |
| Custom rates | `:X03CCCCFFFF#` centering and find rates<br>`:X06AAADDD#` RA / DEC acceleration ramp | — | `:RAn.n#` / `:REn.n#` axis rate in °/s (X) → ∅ | `:RA[n.n]#` / `:RE[n.n]#` °/s → ∅; driver sends `:RE00.03#:RA00.03#` at connect ✓ | `:Rvnnnn.nn#` 0.00 … 1440.00x → ∅; times sidereal = °/s × 3600 / 15.041 |
| Maximum slew rate | `:TTMXRRDD#` RA / DEC maximum speed | — | `:SX92,n.nnn#` slew period in µs/step, `:SX93,n#` slew preset (X) | — | `:SRl720#` / `:SRl1440#` (times sidereal) → `1`/`0` ✓‡ |
| Rate queries | `:TTGMX#` → `RRaDD#` maximum speed | — | `:GX92#` current, `:GX93#` default, `:GX99#` fastest slew period; `:GX97#` → current rate `n.n#` °/s (X) | `:GX91#` → arrow rate in ° `n.nn#`<br>`:GX97#` → max slew rate `n.n#` | `:GRl#` → `720#` or `1440#` maximum slew speed ✓‡ |
| Other motion | `:X1Ard#` motor directions; `:X1B#` → `w<DEC><RA>#` (`1` reversed)<br>`:X40n#` DEC direction (`0` clockwise, `1` counterclockwise)<br>`:hN#` motors off, `:hW#` motors on | — | `:Mp#` spiral search (X) → ∅ | `:Mp#` spiral search → ∅ | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Move north / south / east / west (until stopped) | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ (position not updated while moving) | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ | `:Mn#` `:Ms#` `:Me#` `:Mw#` → ∅ ✓ |
| Stop one direction | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | any `:Q…#` acts as `:Q#` | `:Qn#` `:Qs#` `:Qe#` `:Qw#`, `:Qa#` (all) → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ | `:Qn#` `:Qs#` `:Qe#` `:Qw#` → ∅ ✓ |
| Rate: guide / centering / find / slew | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | — | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ | `:RG#` `:RC#` `:RM#` `:RS#` → ∅ ✓ |
| Rate presets by number | `:R0#` … `:R4#` → ∅ (`:R0#` = `:RG#` … `:R3#` = `:RS#`, `:R4#` max) | — | — | — | — |
| Custom rates | `:SXR0,VVV#` guide (0.01x), `:SXR1,VVV#` centering, `:SXR2,VVV#` move, `:SXR3,VVV#` slew → `0`/`1` | — | `:XSMn#` manual slewing mode, `:XSXn.nnn#` RA, `:XSYn.nnn#` DEC speed in °/s → ∅ | — | — |
| Rate queries | `:GXR0#` … `:GXR3#` → `VVV#` | — | — | — | — |
| Other motion | `:M1svv.vvv#` / `:M2svv.vvv#` move axis 1 / 2 at a sidereal multiple → `1`, `i`, `h`, `s`, `e`, `g` | — | `:MAZn.nn#` / `:MALn.nn#` move azimuth / altitude adjuster by arcminutes → ∅; `:MAAH#` → `1` | — | — |

## 6. Guiding

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Pulse guide for a time in ms | — ; the driver guides with `:RG#`, then `:Mn#` `:Ms#` `:Me#` `:Mw#` and the matching `:Qn#` … `:Qw#` after the pulse time ✓[^hosttimed] | `:MgnDDDD#` `:MgsDDDD#` `:MgeDDDD#` `:MgwDDDD#` → ∅ ✓ (firmware before 31Ee: as Classic ✓[^hosttimed]) | `:Mg<d><time>#` (d = `n` `s` `e` `w`) → ∅ ✓[^gemmg] | `:MgnXXXX#` `:MgsXXXX#` `:MgeXXXX#` `:MgwXXXX#` → ∅ (up to 9999 ms from 2.10) ✓ | — |
| Timed move at guide rate, short form | — | — | — | `:MnXXX#` `:MsXXX#` `:MeXXX#` `:MwXXX#` → ∅ | `:Mnxxx#` `:Msxxx#` `:Mexxx#` `:Mwxxx#` → ∅ (GTOCP3 and later; 0 = continuous); driver sends three digits, up to 99999 ms on a GTOCP4 and later‡ ✓ |
| Other guide moves | — | — | `:Ma<d><arcsec>#` move by arcseconds, `:Mi<d><ticks>#` move by 1 … 255 encoder ticks → ∅ | — | — |
| Set guide rate | — | `:RgSS.S#` (″/s, at most 15.0417) → ∅ (Autostar II) ✓[^rg] | native `>150:0.2`…`0.8` both axes ✓, `>151:` RA, `>152:` DEC (L5) | `:RgSS.S#` (″/s) → ∅<br>`:RGn#` presets (see section 5) | `:RG0#`, `:RG1#`, `:RG2#` (see section 5) |
| Get guide rate | — | — | native `<150:` → `0.n` ✓ | `:Ggui#` → `S.SS#` (″/s) | — |
| Guiding status | — | — | `:Gv#` → `G` while guiding (see section 12) | `:Gpgc#` → `0#` none, `1#` RA, `2#` DEC, `3#` both | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Pulse guide for a time in ms | `:Mg<d>DDDD#`, `:Mg<d>DDDDD#` (d = `n` `s` `e` `w`) → ∅; driver sends four digits ✓ | `:Mgdnnnn#` → ∅ ✓ | `:Mgdnnnn#` (20 … 16399 ms) → ∅ ✓ | `:Mg[d][n]#` → ∅ ✓ | `:Mgdnnnn#` (0000 … 3000 ms) → ∅ ✓ |
| Pulse guide with reply | — | — | `:MGdn#` → `0`/`1` (X) | `:MG[d][n]#` → `0`/`1` | — |
| Set guide rate | `:X20pp#` RA ✓, `:X21pp#` DEC ✓ (percent of sidereal, 05 … 95 in steps of 5) → ∅<br>`:TTSFh#` ✓ / `:TTRFh#` ST4 port on / off (driver: on at connect) | — | `:R0#` … `:R2#` presets (see section 5) | — | `:Rg0.nn#` (0.10 … 0.90, one rate for both axes) → ∅ ✓ |
| Get guide rate | `:X22#` → `RRbDD#` (RA, DEC percent) ✓ | — | `:GX90#` → pulse guide rate `n.nn#` (X) | `:GX90#` → `0.25`, `0.50`, `1.00` `#` | `:Ggr#` → `0.nn#` ✓ |
| Guiding status | `:Z3…#` motion `3` (see section 12) | — | `:GU#` flags `G` pulse guide, `g` guide (see section 12) | `:GU#` flags `G`, `g` | `:GU#` flags `T`, `t` (ST4 guiding) |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Pulse guide for a time in ms | `:Mgdnnnn#` → ∅ ✓ | — (no pulse guiding; the driver refuses the guider) | `:MGdnnnn#` (d = `N` `E` `W` `S`) → `1`; driver sends `:Mgdnnnn#` ✓ | `:Mgdnnnn#` → ∅ ✓ | `:Mgdnnnn#` → ∅ ✓ |
| Pulse guide with reply | — | — | (see above) | — | — |
| Set guide rate | `:SXR0,VVV#` → `0`/`1` | — | — | — | — |
| Get guide rate | `:GXR0#` → `VVV#` | — | — | — | — |
| Guiding status | `:GXJP#` → `1#`/`0#` | — | `:GIG#` → `1#`/`0#` | — | — |

## 7. Tracking and tracking rates

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Tracking on | `:AP#` polar, `:AA#` alt-az alignment → ∅ | `:AP#` polar ✓, `:AA#` alt-az ✓ → ∅ (the alignment reported at connect is restored)[^meadetrack] | native `>192:` RA motor moving ✓ | `:AP#` → ∅ ✓ (driver: `:GW#` first, `:AA#` if it answers `A`‡)[^10mtrack] | `:RT0#` / `:RT1#` / `:RT2#` (the selected rate) → ∅ ✓ |
| Tracking off | `:AL#` land → ∅ | `:AL#` land → ∅ ✓ | native `>191:` RA motor stopped ✓ | `:AL#` → ∅ ✓<br>`:RT9#`, `:STOP#` → ∅ | `:RT9#` zero rate → ∅ ✓ |
| Sidereal rate | `:TQ#` quartz → ∅ ✓ | `:TQ#` → ∅ ✓ | native `>131:` ✓ | `:TQ#` → ∅ ✓; `:RT2#` → ∅ | `:RT2#` → ∅ ✓ (King off first with `:RT3#`‡ ✓) |
| Solar rate | — ; driver: `:ST60.0#` then `:TM#` ✓ | `:TS#` → ∅ ✓ | native `>134:` ✓ | `:TSOLAR#` → ∅ ✓; `:RT1#` → ∅ | `:RT1#` → ∅ ✓ |
| Lunar rate | — ; driver: `:ST57.9#` then `:TM#` ✓ | `:TL#` → ∅ ✓ | native `>133:` ✓ | `:TL#` → ∅ ✓; `:RT0#` → ∅ | `:RT0#` → ∅ ✓ |
| King rate | — | — | native `>132:` ✓ | — | `:RT8#` King on, then `:RT2#` → ∅ (GTOCP4 from P02-08, GTOCP5/6)‡ ✓ |
| Custom rate and adjustment | `:STTT.T#` → `Ok` ✓<br>`:TM#` manual ✓, `:T+#` / `:T-#` ±0.1 Hz → ∅ | `:STdddd.ddddddd#` → `0` / `2` (Autostar II)<br>`:ST+#` / `:ST-#`, `:T+#` / `:T-#` ±0.1 Hz, `:TM#` → ∅ | native `>135:` terrestrial, `>136:` closed loop, `>137:` comet; `>411:` … `>416:` rate divisors | `:TDDD.DDD#` then `:TM#`; `:STDDD.DDD#` → `0`/`1`; `:T+#` / `:T-#` ±0.025″/s<br>`:RRsXXX.XXXX#` / `:RDsXXX.XXXX#` RA / DEC offset → `1` | `:RR sxxx.xxxx#` / `:RD sxxx.xxxx#` → `1` (driver: `:RD0#` before a firmware park‡ ✓) |
| Get tracking rate | `:GT#` → `TT.T#` (56.4 … 60.1 Hz) ✓ | `:GT#` → `TT.T#` ✓[^autostargt] | native `<130:` → `131` … `137` ✓ | `:GT#` → `TT.T#` (four times the rate in ″/s) ✓ | from `:GOS#` (see section 12)‡ ✓ |
| Tracking status | — | `:GW#` second character `T`/`N` ✓; without `:GW#`: ACK `L` = not tracking ✓ | `:Gv#` (see section 12) ✓ | `:GTRK#` → `0`/`1`; `:GTTRK#` target trackable → `0`/`1` | from `:GOS#`‡ ✓ |
| Compensation | — | `:STA±`, `:STZ±` altitude / azimuth SmartDrive (Autostar II) | `:p2#`, `:p3#` refraction (see section 2) | `:SREFn#`, `:GREF#`; `:SRPRSPPPP.P#` / `:GRPRS#` pressure, `:SRTMPsTTT.T#` / `:GRTMP#` temperature → `0`/`1`; `:SSCn#` / `:GSC#` RA speed correction | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Tracking on | `:X122#` ✓ (equatorial), `:X123#` (alt-az) → ∅; `:X5A#` follows a tracking change | — | `:Te#` → `0`/`1`; driver sends the rate and `:Te#` in one write, e.g. `:TQ#:Te#` ✓ | `:Te#` → `0`/`1`; driver as OnStep ✓ (also before a goto with tracking off) | `:Te#` → `1`/`0` ✓ |
| Tracking off | `:X120#` → ∅ ✓ | — | `:Td#` → `0`/`1` ✓ | `:Td#` → `0`/`1` ✓ | `:Td#` → `1`/`0` ✓ |
| Sidereal rate | `:TQ#` → ∅ ✓ | `:TQ#` → ∅ ✓ | `:TQ#` → ∅ ✓ | `:TQ#` → `0`/`1` (60.164 Hz) ✓ | `:TQ#` → ∅ ✓ |
| Solar rate | `:TS#` → ∅ ✓ | `:TS#` → ∅ ✓ | `:TS#` → ∅ ✓ | `:TS#` → `0`/`1` (60 Hz) ✓ | `:TS#` → ∅ ✓ |
| Lunar rate | `:TL#` → ∅ ✓ | `:TL#` → ∅ ✓ | `:TL#` → ∅ ✓ | `:TL#` → `0`/`1` (57.9 Hz) ✓ | `:TL#` → ∅ ✓ |
| King rate | — | — | `:TK#` → ∅ ✓ | `:TK#` → `0`/`1` (60.136 Hz) ✓ | — |
| Custom rate and adjustment | `:X00SSSS#` sidereal rate<br>`:X1ETTTT#` / `:X1FTTTT#` RA / DEC tracking rate<br>`:X12xxxx#` custom tracking rate (old form) | — | `:STdd.ddddd#` → `0`/`1` (X: `:STn.n#`, 0 stops tracking)<br>`:T+#` / `:T-#` ±0.02 Hz, `:TR#` reset → ∅<br>`:SXTR,n.n#` / `:SXTD,n.n#` RA / DEC offset, `:GXTR#` / `:GXTD#` (X) | `:ST[H.H]#` → `0`/`1`<br>`:T+#` / `:T-#` ±0.02 Hz, `:TR#` reset → ∅ | `:STRn.nnnn#` user defined tracking rate → `1`/`0`<br>`:STannsnn#` / `:GTa#` meridian behaviour (see section 10) ✓ |
| Get tracking rate | — | — | `:GT#` → `dd.ddddd#` (X: `n.nnnnn#`, `0#` while not tracking) ✓[^onstepgu] | `:GT#` → `n.n` (`0` while not tracking) | `:GT#` → `0#`, `1#`, `2#` ✓[^zwogt] |
| Tracking status | `:X34#` (see section 12) ✓; `:Z1…#`, `:Z3…#` | — | `:GU#` flag `n` = not tracking ✓ | `:GU#` flag `n` ✓ | `:GU#` flag `n` ✓; `:GAT#` → `0#`, `1#` or `e<n>#` ✓ (firmware 1.1.1 and later)[^zwoerr] |
| Compensation | — | — | `:Tr#` refraction, `:Tn#` none → `0`/`1`; (X) `:To#` full model, `:T1#` single axis, `:T2#` dual axis | `:Tr#` → `0`/`1` | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Tracking on | `:Te#` → `0`/`1` ✓ | — | `:MT1#` → `1` ✓ | `:AP#` → ∅ (no effect on the motor)[^esp32go] | — |
| Tracking off | `:Td#` → `0`/`1` ✓ | — | `:MT0#` → `1` ✓ | `:AL#` → ∅ (no effect on the motor)[^esp32go] | — |
| Sidereal rate | `:TQ#` → ∅ ✓ | — | `:XSS1.000#` tracking factor → ∅ ✓ | `:TQ#` → ∅ ✓ | `:TQ#` → ∅ ✓ |
| Solar rate | `:TS#` → ∅ ✓ | — | `:XSS0.997#` → ∅ ✓ | `:TS#` → ∅ ✓ | `:TS#` → ∅ ✓ |
| Lunar rate | `:TL#` → ∅ ✓ | — | `:XSS0.965#` → ∅ ✓ | `:TL#` → ∅ ✓ | `:TL#` → ∅ ✓ |
| King rate | — | — | — | `:TK#` → ∅ ✓ | — |
| Custom rate and adjustment | `:STdd.ddddd#` → `0`/`1`<br>`:T+#` / `:T-#` ±0.02 Hz, `:TR#` reset → ∅ | — | `:XSSn.nnn#` factor → ∅ ✓; `:XGS#` → factor `#`; `:XGT#` → speed `#`; `:XSTnnnn#` | — | — |
| Get tracking rate | `:GT#` → `dd.ddddd#` ✓ | — | `:GT#` → `60.0#` | `:GT#` → `50.0#` whatever the rate; the rate is in `:GU#` ✓ | — |
| Tracking status | `:GXI#` (see section 12)‡ ✓; `:GXJT#` → `1#`/`0#` | — | `:GX#` (see section 12) ✓; `:GIT#` → `1#`/`0#` | `:GU#` (see section 12) ✓ | — |
| Compensation | `:T0#` off, `:T1#` RA only, `:T2#` both axes, `:Tr#` refraction, `:Tn#` none → `0`/`1` | — | — | — | — |

## 8. Park and home

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Park | `:hP#` slew to the home position → ∅ | `:hP#` slew to the park position → ∅ ✓[^dpark] | `:hP#` park at the home position ✓, `:hC#` park at the startup (CWD) position ✓, `:hZ#` park at the zenith (L5) ✓ → ∅ | `:KA#` → ∅ ✓; `:hP#` → ∅ | `:KA#` → ∅ ✓; firmware park positions: `$Kn#` (n = 1 … 5) after `:Q#`, `:RD0#`, `:RT9#`‡ ✓ |
| Unpark | — | `:hW#` wake up (Autostar II) | `:hW#` wake up, resume tracking → ∅ ✓ | `:PO#` → ∅ ✓ | `:PO#` park-off, restores calibration → ∅ ✓ |
| Park status | — | — | `:h?#` → `2` in progress, `1` parked, `0` no park received or park failed ✓[^gemh] | `:Gstat#` (see section 12) ✓ | `:GOS#` (see section 12)‡ ✓ |
| Set park position | — | `:hS#` current position becomes park (Autostar II, LX 16″) → ∅ | — | `:PyX#` save current axis angles as park → `0`/`1` | — |
| Other park commands | — | — | native `>92:` park behaviour (what wakes the mount, L5.1) | `:PaX#` park at the target axis angles, `:PsX#` park at the saved angles → `0#` … `4#`; `:PiP#` park here → `0#`/`1#` | — |
| Sleep / wake | — | `:hN#` sleep, `:hW#` wake (Autostar II) | `:hN#` sleep (stop tracking, blank display), `:hW#` wake | — | — |
| Goto home | `:hF#` home search, sets the position from the saved values → ∅ | `:hF#` seek home and align from the stored encoder values → ∅<br>`:hC#` calibrate home (Autostar II) | (`:hC#` parks at the startup position, see Park) | `:hF#` → ∅ (mounts with home sensors) ✓ | — |
| Set or reset home | `:hS#` home search, saves the position → ∅ | `:hIYYMMDDHHMMSS#` initialize without the handbox → `1` (Autostar II) | — | `:hS#` seek home and store the alignment → ∅ | — |
| Home status | `:h?#` → `0` failed / not attempted, `1` found, `2` in progress | `:h?#` → `0`, `1`, `2` | — | `:h?#` → `0`, `1`, `2`; `:Gstat#` `4#` homing, `7#` stopped ✓ | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Park | `:X362#` → `pB#` ✓; `:hP#` | — | `:hP#` → `0`/`1` ✓ | `:hP#` → `0`/`1` ✓ | `:hP#` → ∅ default or custom park position, equatorial mode, firmware 1.1.9 and later; the driver goes home with `:hC#` first unless `:GU#` has `H` ✓[^zwopark] |
| Unpark | `:X370#` → `p0#` ✓ | — | `:hR#` → `0`/`1` ✓ | `:hR#` → `0`/`1`‡ ✓ | `:Spu#` → `1` released, `0` or `e<n>#` refused; no motion ✓[^zwopark] |
| Park status | `:X38#` → `p0#` not parked, `p1#` at home, `p2#` parked, `pB#` parking ✓ | — | `:GU#` flags `P` parked, `I` parking, `p` not parked, `F` park failed ✓ | `:GU#` flags as OnStep ✓ | `:Gps#` → `0#` not parked, `1#` parking, `2#` parked, `3#` park error ✓[^zwopark] |
| Set park position | `:X352#` current position becomes park → ∅ | — | `:hQ#` → `0`/`1` ✓ | `:hQ#` → `0`/`1` ✓ | `:Sp01#` → `1` stored, `2` already set, `3` only one park position, `4` equatorial mode only, `5` not homed, `9` moving; firmware 1.3.0 and later ✓[^zwopark] |
| Other park commands | `:X372#` park where the mount stands, taken as the stored park position | — | — | — | — |
| Sleep / wake | `:hN#` motors off, `:hW#` motors on | — | — | — | — |
| Goto home | `:X361#` → `pA#` ✓ | — | `:hC#` → ∅ ✓ | `:hC#` → ∅ ✓ | `:hC#` → ∅ (mechanical zero position) ✓ |
| Set or reset home | `:X31HHMMSS#` then `:X351#` (see section 4) | — | `:hF#` reset the mount at home (cold start) → ∅ ✓<br>`:hA0#` / `:hA1#` automatic home at boot, `:hC1,n#` / `:hC2,n#` home offsets, `:hC1,R#` / `:hC2,R#` sense reversal (X) | `:hF#` reset at home (cold start) → ∅ ✓ | — |
| Home status | `:X34#` (see section 12) ✓; `:X38#` `p1#` | — | `:GU#` flags `H` at home, `h` homing ✓; `:h?#` → `hasSense,axis1Offset,axis2Offset#` (X) | `:GU#` flags `H`, `h` ✓ | `:GU#` flag `H` ✓ (a home slew ends with `H` from firmware 1.8.1, before that with `N`); `:Gh#` → `1` homing has succeeded, `0` it never has[^conflict] |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Park | `:hP#` → `0`/`1` ✓ | — | `:hP#` home, then park offset, stop all motors → ∅ ✓[^oat] | `:hP#` → ∅: goes home and raises the parked flag; driver uses it as home ✓[^esp32go] | — |
| Unpark | `:hR#` → `0`/`1` ✓ | — | `:hU#` → `1` (turns tracking on) ✓[^oat] | — (a goto or a sync clears the parked flag) | — |
| Park status | `:GXI#` (see section 12)‡ ✓ | — | `:GX#` state `Parked`, `Parking` ✓[^oat] | `:PP#` → `1#`/`0#`; `:GU#` second character ✓ | — |
| Set park position | `:hQ#` → `0`/`1` ✓ | — | — | — | — |
| Other park commands | `:hO#` reset the park definition → `0`/`1` | — | — | `:cRR#` saves the position and restarts the controller | — |
| Goto home | `:hC#` → `0`/`1` ✓ | — | `:hF#` → ∅ (keeps tracking) ✓<br>`:MHRxn#` / `:MHDxn#` Hall sensor homing → `1`/`0` | `:hP#` (see Park) ✓ | — |
| Set or reset home | `:hB#` current position becomes home ✓, `:hF#` sync at home, `:hb#` reset the home definition → `0`/`1` | — | `:SHP#` current position becomes home → `1`; `:hZ#` AZ/ALT home → `1`; `:XSHRnnn#` / `:XSHDnnn#` Hall offsets | `:hS#` stores the current position as home → ∅ ✓ | — |
| Home status | `:GXI#` fourth character `H`‡ ✓ | — | `:GX#` state `Homing` ✓; `:XGAH#` → `rastate`&#124;`decstate#` | `:GU#` second character `P` ✓ | — |

## 9. Site, date and time

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Get date | `:GC#` → `MM/DD/YY#` ✓ | `:GC#` → `MM/DD/YY#` ✓ | `:GC#` → `mm/dd/yy#` ✓ | `:GC#` → LX200 `MM/DD/YY#`, extended `MM:DD:YY#`, ultra `YYYY-MM-DD#` ✓ | `:GC#` → `MM:DD:YY#` (GTOCP3 and later) ✓ |
| Get local time (24 h) | `:GL#` → `HH:MM:SS#` ✓ | `:GL#` → `HH:MM:SS#` ✓ | `:GL#` → `hh:mm:ss#`, double precision `±hh.hhhhhh#` ✓[^gemfmt] | `:GL#` → `HH:MM:SS#`, extended `HH:MM.T#` / `HH:MM:SS.S#`, ultra `HH:MM:SS.SS#` ✓ | `:GL#` → `HH:MM.M#`, long `HH:MM:SS.S#` ✓ |
| Get local time (12 h), clock format | `:Ga#` → `HH:MM:SS#` | `:Ga#` → `HH:MM:SS#`<br>`:Gc#` → `12#` / `24#`; `:H#` toggle → ∅ | `:Gc#` → `24#` (L4: `12#` or `24#`) | (`:Ga#` is the target altitude) | — |
| Get UTC offset | `:GG#` → `sHH#` ✓ | `:GG#` → `sHH#` or `sHH.H#` ✓ | `:GG#` → `±hh#`, L5 also `±hh:mm:ss#` ✓ | `:GG#` → LX200 `sHH.H#`, extended `sHH:MM.M#` / `sHH:MM:SS.S#` ✓ | `:GG#` → `HH:MM.M#`, long `HH:MM:SS.S#` ✓[^apgg] |
| Set date | `:SCMM/DD/YY#` → `1`, then `Updating planetary data#`, then blanks `#` ✓ | `:SCMM/DD/YY#` → `0`, or `1Updating Planetary Data#` + blanks `#` (Autostar II: UTC date) ✓ | `:SCmm/dd/yy#` → `0`, or `1Updating planetary data#<24 blanks>#` ✓ | `:SCMM/DD/YY#`, `:SCMM/DD/YYYY#`, `:SCYYYY-MM-DD#` → `0`, `1Updating        Planetary Data. #<32 blanks>#`; extended `1<32 blanks>#<32 blanks>#`; ultra `1` ✓ | `:SC MM/DD/YY#` → `<32 blanks>#<32 blanks>#` ✓[^apsc] |
| Set local time | `:SLHH:MM:SS#` → `Ok` ✓ | `:SLHH:MM:SS#` → `0`/`1` ✓ | `:SLhh:mm:ss#` → `1` ✓ | `:SLHH:MM:SS#`, `.S`, `.SS` → `0`/`1` ✓ | `:SL HH:MM:SS#` → `1` ✓ |
| Set UTC offset | `:SGsHH#` → `Ok` ✓ | `:SGsHH.H#` → `0`/`1` ✓ | `:SG±hh#` → `1`; must come before `:SC` and `:SL` ✓[^gemsg] | `:SGsHH.H#`, `:SGsHH:MM.M#`, `:SGsHH:MM:SS#` → `0`/`1` ✓ | `:SG sHH#`, `:SG sHH:MM.M#`, `:SG sHH:MM:SS#` → `1` ✓ |
| Daylight saving | — | `:GH#` → `1#`/`0#`, `:SHD#` (Autostar II); driver sends `:SHD#` when `:GH#` answers ✓[^conflict] | — | (in the UTC offset) | — |
| Sidereal time | `:GS#` → `HH:MM:SS#`; `:SSHH:MM:SS#` → `Ok` | `:GS#` → `HH:MM:SS#`; `:SSHH:MM:SS#` → `0`/`1` | `:GS#` → `hh:mm:ss#` (double `±hh.hhhhhh#`) | `:GS#` → `HH:MM.M#`, `HH:MM:SS#`, `HH:MM:SS.S#`, `HH:MM:SS.SS#` | `:GS#` → `HH:MM.M#`, long `HH:MM:SS.S#` |
| Get latitude | `:Gt#` → `sDD*MM#` ✓ | `:Gt#` → `sDD*MM#` ✓ | `:Gt#` → `±dd°mm#` (double `±dd.dddddd#`) ✓ | `:Gt#` → `sDD*MM#`, extended high `sDD*MM:SS#`, ultra `sDD:MM:SS.S#` ✓ | `:Gt#` → `sDD*MM#`, long `sDD*MM:SS#` ✓ |
| Get longitude | `:Gg#` → `DDD*MM#` ✓ | `:Gg#` → `sDDD*MM#` (east negative) ✓ | `:Gg#` → `±ddd°mm#` ✓ | `:Gg#` → `sDDD*MM#`, extended high `sDDD*MM:SS#`, ultra `sDDD:MM:SS.S#` ✓ | `:Gg#` → `+DDD*MM#`, long `+DDD*MM:SS#` ✓ |
| Set latitude | `:StsDD*MM#` → `Ok` ✓ | `:StsDD*MM#` → `0`/`1` ✓ | `:St±dd*mm#` → `1` ✓ | `:StsDD*MM#`, `:SS`, `.S` forms → `0`/`1` ✓ | `:St sDD*MM#`, `:St sDD*MM:SS` → `1` ✓ |
| Set longitude | `:SgDDD*MM#` → `Ok` ✓ | `:SgDDD*MM#` → `0`/`1` ✓ | `:Sg±ddd*mm#` (west positive; east negative or +360) → `1` ✓ | `:SgsDDD*MM#`, `:SS`, `.S` forms (east negative) → `0`/`1` ✓ | `:Sg DDD*MM#`, `:Sg DDD*MM:SS#` → `1` ✓ |
| Site elevation | — | — | — | `:Gev#` → `sXXXX.X#`; `:SevsXXXX.X#` → `0`/`1` | — |
| Site slots and names | `:W1#` … `:W4#` → ∅<br>`:GM#` … `:GP#` → `XYZ#`; `:SM…#` … `:SP…#` → `Ok` | `:W1#` … `:W4#` → ∅<br>`:GM#` … `:GP#` → `<string>#`; `:SM…#` … `:SP…#` → `0`/`1` | `:W0#` … `:W3#` → ∅, `:W?#` → `n`<br>`:GM#` … `:GP#`; `:S0…#` (L5), `:SM…#` … `:SP…#` → `1` | — | — |
| Other time functions | — | GPS (Autostar II): `:gT#` → `0`/`1`, `:g+#`, `:g-#`, `:gps#` → NMEA `#` | `:GE#` / `:SE…#` alarm time; `:WQ#` query GPS | `:GJD#`, `:GJD1#`, `:GJD2#` Julian date; `:SJD…#`; `:GLDT#` / `:SLDT…#` local, `:GUDT#` / `:SUDT…#` UTC date and time; `:GDUT#`, `:GDGPS#`, `:GULEAP#`; `:NUtimsXXX#` adjust ms; GPS `:gT#`, `:gps#`, `:gtg#` | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Get date | — (no calendar) | — | `:GC#` → `MM/DD/YY#` ✓ | `:GC#` → `MM/DD/YY#` ✓ | `:GC#` → `MM/DD/YY#` ✓ |
| Get local time (24 h) | — (no clock) | — | `:GL#` → `HH:MM:SS#` ✓; `:GLH#` (X) | `:GL#` → `HH:MM:SS#` ✓; `:GLH#` → `HH:MM:SS.SSSS#` | `:GL#` → `HH:MM:SS#` ✓ |
| Get local time (12 h), clock format | — | — | `:Ga#` → `HH:MM:SS#`; `:Gc#` → `24#` (X) | `:Ga#` → `HH:MM:SS#`; `:Gc#` → `24#` | — |
| Get UTC offset | — | — | `:GG#` → `sHH#` (X: `sHH:MM#`) ✓ | `:GG#` → `sHH:MM#` ✓ | `:GG#` → `sHH:MM#` ✓ |
| Set date | — (no calendar; see `:X32HHMMSS#`) | `:SCMM/DD/YY#` → `1` ✓ | `:SCMM/DD/YY#` (X: also `/YYYY`) → `0`/`1` ✓ | `:SC[MM/DD/YYYY]#` → `0`/`1`; driver sends `MM/DD/YY` ✓ | `:SCMM/DD/YY#` → `1`/`0` ✓ |
| Set local time | `:SLHH:MM:SS#` | `:SLHH:MM:SS#` → `1` ✓ | `:SLHH:MM:SS#` → `0`/`1` ✓ | `:SL[HH:MM:SS]#` → `0`/`1` ✓ | `:SLHH:MM:SS#` → `1`/`0` ✓ |
| Set UTC offset | `:SGsHH#`, `:SGsHH.H#` | `:SGsHH#` → `1` ✓ | `:SGsHH#` (X: also `:SGsHH:MM#`) → `0`/`1` ✓ | `:SG[sHH:MM]#` → `0`/`1`; driver sends `:SGsHH#` ✓ | `:SGsHH#`, `:SGsHH:MM#` → `1`/`0` ✓ |
| Daylight saving | — | — | — | — | `:GH#` → `1`/`0`; `:SHn#` → `1`[^conflict] |
| Sidereal time | `:GS#`<br>`:X32HHMMSS#` sets the local sidereal time → ∅ ✓ (driver: at connect, on every site change and every 22 s while no goto runs) | — | `:GS#` → `HH:MM:SS#`; `:SSHH:MM:SS#` → `0`/`1`; `:GSH#` (X) | `:GS#` → `HH:MM:SS#`; `:GSH#` → `HH:MM:SS.ss#` | `:GS#` → `HH:MM:SS#` |
| Get latitude | `:Gt#` → `sDDtMM:SS#` ✓[^stargo] | — | `:Gt#` → `sDD*MM#` ✓; `:GtH#` (X) | `:Gt#` → `sDD*MM#` ✓; `:GtH#` → `sDD*MM:SS.SSS#` | `:Gt#` → `sDD*MM#` ✓ |
| Get longitude | `:Gg#` → `sDDDgMM:SS#` (east positive) ✓[^stargo] | — | `:Gg#` → `DDD*MM#` (X: `sDDD*MM#`, `:GgH#`) ✓ | `:Gg#` → `sDDD*MM#` (east negative) ✓; `:GgH#` | `:Gg#` → `sDDD*MM#` ✓ |
| Set latitude | `:StsDD*MM:SS#` ✓, `:StsDD*MM#` | `:StsDD*MM#` → `1` ✓ | `:StsDD*MM#` → `0`/`1` ✓ (X: `:SS`, `.SSS` forms) | `:St[sDD*MM:SS]#` → `0`/`1`; driver sends `:StsDD*MM#` ✓ | `:StsDD*MM:SS#` → `1`/`0`; driver sends `:StsDD*MM#` ✓ |
| Set longitude | `:SgsDDD*MM:SS#` (east positive, −180 … +180) ✓, `:SgDDD*MM#` | `:SgDDD*MM#` → `1` ✓ | `:SgDDD*MM#` → `0`/`1` ✓ (X: optional sign, `:SS`, `.SSS` forms) | `:Sg[(s)DDD*MM:SS]#` → `0`/`1`; driver sends `:SgDDD*MM#` ✓ | `:SgsDDD*MM:SS#` → `1`/`0`; driver sends `:SgDDD*MM#` ✓ |
| Site elevation | — | — | `:Gv#` → `sn.n#`; `:Svsn.n#` → `0`/`1` (X)[^conflict] | `:Gv#` → `±n.n`; `:Sv[sn.n]#` → `0`/`1`; driver sends `:Svn.n#` ✓ | — |
| Site slots and names | — | — | `:W0#` … `:W3#` → ∅ (X: `:W?#` → `n#`)<br>`:GM#` … `:GP#`; `:SM…#` … `:SP…#` → `0`/`1` | — | — |
| Other time functions | `:X50DDMMYY#` date of the goto to the Sun (see section 4) | — | (X) `:GX80#` UT1 time, `:GX81#` UT1 date, `:GX89#` date/time ready → `0` ready, `1` not; `:SUs.s#` DUT1 | `:GX89#` → `0` ready, `1` not | `:SMTIMM/DD/YY&HH:MM:SS&sHH:MM#` / `:GMTI#` date, time and zone; `:SMGEsDD*MM:SS&sDDD*MM:SS#` / `:GMGE#` latitude and longitude |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Get date | `:GC#` → `MM/DD/YY#` ✓ | — | `:GC#` → `MM/DD/YY#` ✓ | — | `:GC#` ✓ |
| Get local time (24 h) | `:GL#` → `HH:MM:SS#` ✓ | — | `:GL#` → `HH:MM:SS#` ✓ | — | `:GL#` ✓ |
| Get local time (12 h), clock format | `:Ga#` → `HH:MM:SS#` | — | `:Ga#` → `HH:MM:SS#`; `:Gc#` → `24#` | — | — |
| Get UTC offset | `:GG#` → `sHH#` ✓ | — | `:GG#` → `sHH#` ✓ | `:GG#` → `sHH#` | `:GG#` ✓ |
| Set date | `:SCMM/DD/YY#` → `0`/`1` ✓ | — | `:SCMM/DD/YY#` → `1Updating Planetary Data#<blanks>#` ✓ | `:SCMM/DD/YY#` ✓ | `:SCMM/DD/YY#` ✓ |
| Set local time | `:SLHH:MM:SS#` → `0`/`1` ✓ | — | `:SLHH:MM:SS#` → `1` ✓ | `:SLHH:MM:SS#` ✓ | `:SLHH:MM:SS#` ✓ |
| Set UTC offset | `:SGsHH.H#` → `0`/`1` ✓ | — | `:SGsHH#` → `1` ✓ | `:SGsHH#` ✓ | `:SGsHH#` ✓ |
| Sidereal time | `:GS#` → `HH:MM:SS#`; `:GSL#` → `HH.VVVVVV#`; `:SSHH:MM:SS#` → `0`/`1` | — | `:XGL#` → `HHMMSS#`; `:SHLHH:MM#` → `1`/`0` | — | `:GS#` (detection only) ✓ |
| Get latitude | `:Gt#` → `sDD*MM#` ✓; `:Gtf#` → `sDD*MM'SS#` | — (silently dropped)[^agotino] | `:Gt#` → `sDD*MM#` ✓ | `:Gt#` → `sDD{0xE1}MM#` ✓[^esp32go] | `:Gt#` ✓ |
| Get longitude | `:Gg#` → `DDD*MM#` ✓; `:Ggf#` → `DDD*MM'SS#` | — (silently dropped) | `:Gg#` → `sDDD*MM#` (east negative) ✓ | `:Gg#` → `sDDD{0xE1}MM#` ✓[^esp32go] | `:Gg#` ✓ |
| Set latitude | `:StsDD*MM#`, `:StsDD*MM:SS#` → `0`/`1` ✓ | — | `:StsDD*MM#` → `1`/`0` ✓ | `:StsDD*MM#` ✓ | `:StsDD*MM#` ✓ |
| Set longitude | `:SgDDD*MM#`, `:SgsDDD*MM#` and `:SS` forms → `0`/`1` ✓ | — | `:SgsDDD*MM#` (signed: as given, east negative; unsigned: 0 … 360 west, 180 at Greenwich) → `1`/`0`; driver sends `:SgsDDD*MM#` ✓[^oat] | `:SgDDD*MM#` ✓ | `:SgDDD*MM#` ✓ |
| Site slots and names | `:W0#` … `:W3#` → ∅<br>`:GM#` … `:GO#`; `:SM…#` … `:SO…#` → `0`/`1` | — | `:GM#` → `OAT1#`, `:GN#` → `OAT2#`, `:GO#` → `OAT2#`, `:GP#` → `OAT4#` | — | — |
| Other time functions | `:SXT0HH:MM:SS#` / `:GXT0#` UTC time; `:SXT1MM/DD/YY#` / `:GXT1#` UTC date; `:SXT2n#` / `:GXT2#` Unix seconds | — | `:gT#`, `:gTnnn#` time and site from GPS → `1`/`0` | — | — |

## 10. Side of pier, meridian flip and limits

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Side of pier | — | — (`:Gm#` is the distance to the meridian on a Max, `sDD*MM'SS#`)[^conflict] | `:Gm#` → `E#` / `W#` ✓ | `:pS#` → `East#` / `West#` | `:pS#` → `East#` / `West#` ✓ |
| Destination side of a goto | — | — | — | `:GTsid#` → `0` none, `2` west, `3` east | — |
| Meridian flip now | — | — | `:Mf#`, `:MM#` (see section 4) | `:FLIP#` → `1`/`0` | — |
| Automatic flip, meridian behaviour | — | — | native `>223:` western goto limit (goto includes a flip to stay inside it) | `:Guaf#` → `0`/`1`; `:SuafN#` unattended flip → ∅<br>`:GMF#` → `1` both sides, `2` west only, `3` east only; `:SMFn#` → `0`/`1` | `:FM#` no flip (fork behaviour), `:EM#` German equatorial flip → ∅ |
| Meridian limits | — | — | native `<220:`…`>222:` safety limits `ddddmm`, `<225:` / `<226:` steps / time to the goto limit, `<230:` / `<231:` | `:Glmt#` / `:SlmtNN#` tracking limit, `:Glms#` / `:SlmsNN#` slew limit in °; `:Gmte#` → minutes to tracking end | — |
| Altitude limits | `:Gh#` → `DD*#` higher limit; `:ShDD#` → `Ok`<br>`:Go#` → `DD*#` lower limit; `:SoDD*#` → `Ok`[^ghgo] | `:Gh#` → `sDD*` high limit; `:ShDD#` → `0`/`1`<br>`:Go#` → `DD*#` lower limit; `:SoDD*#` → `0`/`1` | — | `:Gh#` → `sDD*#` high limit; `:ShsDD#` → `0`/`1`<br>`:Go#` → `sDD*#` low limit; `:SosDD#` → `0`/`1` | `:ho#` / `:hq#` horizon check on / off → ∅ |
| Axis limits | — | — | native `<235:`…`<239:` axis positions and half circle | `:GaXa#`, `:GaXb#` (see section 3) | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Side of pier | `:X39#` → `PE#` / `PW#` the side of the meridian the telescope points to (not the side of the pier), `PX#` not aligned<br>`:Z3…#` side digit (see section 12) | — | `:Gm#` → `N#`, `E#`, `W#`; driver reads `:GU#` (`o` none, `T` east, `W` west) ✓; `:GX94#` → `0`/`1`/`2` (X) | `:Gm#` → `E#`, `W#`, `N#`; driver reads `:GU#` as OnStep ✓[^nyx] | `:Gm#` → `E`, `W`, `N` (home / zero position) `#` ✓ |
| Destination side of a goto | — | — | `:MD#` → `0`, `1`, `2` (X) | `:MD#` → `0` east, `1` west, `2` error | — |
| Meridian flip now | — | — | `:MN#` (see section 4) (X) | `:MN#` (see section 4) | — |
| Automatic flip, meridian behaviour | `:TTSFd#` / `:TTRFd#` force meridian flip on / off (the next goto flips before the meridian)<br>`:TTSFS#` / `:TTRFS#` only one side (no flip) on / off; `:TTGFS#` reads it[^stargoflags] | — | `:GX95#` → `0`/`1`; `:SX95,0#` / `:SX95,1#` → `0`/`1` ✓<br>`:GX96#` → `E`, `W`, `B`, `A`; `:SX96,E#` / `,W#` / `,B#` / `,A#` preferred side → `0`/`1` ✓<br>`:SX98,n#` pause at home, `:SX99,1#` continue (X) | — | `:STannsnn#` (flip at the limit, track past the meridian, limit −15 … +15°) → `1`/`0` ✓; `:GTa#` → `nnsnn#` ✓ (firmware 1.2.4 and later) |
| Meridian limits | — | — | `:GXE9#` / `:GXEA#` → east / west limit in minutes `n#`; `:SXE9,n#` / `:SXEA,n#` → `0`/`1` ✓ | `:GXE9#` / `:GXEA#` → `n#`; `:SXE9,[n]#` / `:SXEA,[n]#` → `0`/`1` | — |
| Altitude limits | — | — | `:Gh#` → `sDD#` (X: `sDD*#`) horizon; `:ShsDD#` → `0`/`1` ✓<br>`:Go#` → `sDD#` (X: `DD*#`) overhead; `:SoDD#` → `0`/`1` ✓[^ghgo] | `:Gh#` → `sDD*#`; `:Sh[sDD]#` → `0`/`1`<br>`:Go#` → `DD*#`; `:So[DD]#` → `0`/`1` | `:SLE#` / `:SLD#` enable / disable → `1`; `:GLC#` → `1`/`0`<br>`:SLHnn#` upper (60 … 90), `:SLLnn#` lower (0 … 30) → `1`/`0`; `:GLH#`, `:GLL#` → `nn#` (firmware 1.2.3 and later) |
| Axis limits | — | — | `:GXEe#`, `:GXEw#`, `:GXEB#`, `:GXEC#`, `:GXED#` (X) | — | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Side of pier | `:Gm#` → `N#`, `E#`, `W#`; driver reads `:GXI#`‡ ✓ | — | — | `:pS#` → `EAST#` / `WEST#`; driver reads `:GU#` fourth character ✓ | — |
| Destination side of a goto | `:M?#` → `?`, `W`, `E` or `!` | — | — | — | — |
| Meridian flip now | `:MF#` → as `:MS#` | — | — | — | — |
| Automatic flip, meridian behaviour | `:smE#`, `:smW#`, `:smN#` target pier side → `0`/`1` | — | — | — | — |
| Meridian limits | `:GXLE#` / `:SXLE,sVV.V#`, `:GXLW#` / `:SXLW,sVV.V#` east / west; `:GXLU#` / `:SXLU,VV#` under-pole | — | `:XGST#` → hours until the RA ring reaches its end | — | — |
| Altitude limits | `:SXLH,sVV#` horizon, `:SXLO,VV#` overhead → `0`/`1`; `:So#` → `VV#` | — | `:XGDLx#` / `:XSDLUnnnnn#` / `:XSDLu#` / `:XSDLLnnnnn#` / `:XSDLl#` DEC limits | — | — |
| Axis limits | `:GXLA#` … `:GXLD#` / `:SXLA,VVVV#` … `:SXLD,VVVV#` (0.1°) | — | — | — | — |

## 11. Focuser

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Move in / out (until stopped) | `:F+#` / `:F-#` → ∅[^focdir] | `:F+#` inward ✓ / `:F-#` outward ✓ → ∅ | `:F+#` in / `:F-#` out → ∅ | — | `:F+#` / `:F-#` → ∅ ✓ |
| Stop | `:FQ#` → ∅ | `:FQ#` → ∅ ✓ | `:FQ#` → ∅ | — | `:FQ#` → ∅ ✓ |
| Speed | `:FF#` fast, `:FS#` slow → ∅ | `:FF#` ✓, `:FS#` ✓ → ∅; `:F<n>#` (1 … 4) | `:FF#` fast, `:FM#` medium, `:FS#` slow → ∅ | — | `:FF#` ✓, `:FS#` ✓ → ∅ |
| Move by an amount | — | `:FPsDDDDD#` pulse for ±65000 ms (Autostar II); relative count (Max, RCX) → ∅ | — | — | — |
| Position and status | — | `:Fp#` → position `#`, `:FB#` → `0`/`1` busy (Max, RCX400) | — | — | — |
| Other focuser commands | — | `:FLD<n>#`, `:FLN<n><name>#`, `:FLS<n>#` presets; `:FC<d>#` collimation (Max, RCX); `:FE#`, `:FL<p>#`, `:Fz…#` zero-shift digital focuser (Autostar II) | — | — | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Move in / out (until stopped) | `:X08AUX1UP#` / `:X09AUX1DN#` AUX1, `:X0DAUX2UP#` / `:X0EAUX2DN#` AUX2 → ∅<br>`:F+#` / `:F-#` SteelDrive | — | `:F+#` in / `:F-#` out → ∅ | — | — |
| Stop | `:X0AAUX1ST#`, `:X0FAUX2ST#` → ∅; `:FQ#` | — | `:FQ#` → ∅ ✓ | — | — |
| Speed | `:X1D…#` AUX1, `:X1C…#` AUX2 speed; `:X30xxx#` AUX goto speed correction | — | `:FF#` fast ✓, `:FS#` slow ✓ → ∅; `:F[n]#` (1 … 4; X: 1 … 9 presets) | — | — |
| Move by an amount | `:X16pppppp#` / `:X17pppppp#` AUX1 / AUX2 goto, `pppppp` = position + 500000 → ∅; `g1#` arrives when it ends<br>`:X23#` / `:X24#` AUX1, `:X25#` / `:X26#` AUX2 single step; `:X28xxyy#` step size | — | `:FR[sn]#` relative → ∅ ✓; (X) `:Fr[sn]#` in steps<br>`:FS[n]#` absolute → `0`/`1`; (X) `:Fs[n]#` | — | — |
| Position and status | `:X0BAUX1AS#` → `AX1…#`, `:X10AUX2AS#` → `AX2…#` (12 characters, the six digits from the sixth are position + 500000)<br>`:X0Cpppppp#` / `:X11pppppp#` preset the AUX1 / AUX2 position (+ 500000)<br>`:X13BAADAS#` SteelDrive position, `:X14…#` reset it; `:Fp#` | — | `:FT#` → `M#` moving / `S#` stopped (X: with a rate digit, `M1#`, `S3#`) ✓<br>`:FG#` → position `n#` (X: `:Fg#`) | — | — |
| Other focuser commands | `:FCn#` / `:FCs#` camera shutter open / close, `:FCe#` camera focus on, `:FCw#`<br>`:X27DD#` Baader device bit time<br>`:X4B1#` default focuser motor, `:X4B3#` reads it | — | `:Fa#` → `1` focuser present ✓[^onstepfoc]; `:FA#`, `:FA[n]#` select (X: 1 … 6); `:fA#`<br>`:FI#`, `:FM#`, `:Fe#`, `:Ft#`, `:Fu#`, `:FB#`, `:FB[n]#`, `:FC#`, `:FC[sn.n]#`, `:Fc#`, `:Fc[n]#`, `:FD#`, `:FD[n]#`, `:FP#`, `:FP[n]#`, `:FZ#`, `:FH#`, `:Fh#`; (X) `:Fp#`, `:FW#`, `:hP#` / `:hR#` in standalone focuser builds | — | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Move in / out (until stopped) | — | — | `:F+#` in ✓ / `:F-#` out ✓ → ∅ | — | — |
| Stop | — | — | `:FQ#` → ∅ ✓ | — | — |
| Speed | — | — | `:FF#` ✓, `:FS#` ✓, `:Fn#` (1 … 4) → ∅ | — | — |
| Move by an amount | — | — | `:MXfnnnnn#` (see section 4) | — | — |
| Position and status | — | — | `:Fp#` → position `nnn#`; `:FPnnn#` set position → `1`<br>`:FB#` → `0` idle / `1` moving | — | — |

## 12. Mount status

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Alignment and tracking status | — | `:GW#` → `<mount><tracking><alignment>#`: mount `A` alt-az, `P` equatorial, `G` German; tracking `T` / `N`; alignment `0` … `3` stars ✓ | (`:GW#` is the RA velocity, L5) | driver sends `:GW#` before switching tracking on‡ ✓[^10mtrack] | — |
| Status word | — | — | native `<99:` status (aligned, model, object selected, goto, limit, J2000); `<97:` change counters (L5); `<81:` ENQ macro (L5.1) | `:Gstat#` → `0#` tracking, `1#` stopped, `2#` slewing to park, `3#` unparking, `4#` slewing home, `5#` parked, `6#` slewing, `7#` tracking off, `8#` motors inhibited (cold), `9#` outside tracking limits, `10#` satellite, `11#` needs `:USEROK#`, `98#` unknown, `99#` error ✓; `:GSTAT#` deprecated | `:GOS#` → status string: [0] `P` parked, [1] tracking `0` lunar, `1` solar, `2` sidereal, `T` King, `C`/`c` custom, `9` stopped, [3] `S` slewing, [10] fault (`1`/`Z` stall, `2`/`Y` low voltage, `4`/`X` servo, `N` `S` `E` `W` limits, `z` kill) (GTOCP3 from S)‡ ✓<br>`:G_E#`, `:G_S#` (firmware park capability)‡ ✓ |
| Velocity / motion state | — | — | `:Gv#` → `N` no movement, `T` tracking, `G` guiding, `C` centering, `S` slewing, `!` stall ✓[^gemv]; `:GW#` RA, `:Gw#` DEC, `:Gu#` both (two characters) (L5) | `:GDW#` / `:GDw#` dome slew status (see section 14) | — |
| Other status | — | `:h?#` home status | `:GI#` information buffer (L5); `:OO#` / `:Oo#` display line (L5) | `:Gstm#` / `:Sstm…#` slew settle time; `:USEROK#`, `:USERWAIT#` | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Alignment and tracking status | `:GW#` → `PT0#` / `AT0#` (axis mode only) | — | `:GW#` → four characters: mount type, tracking, parked/home, alignment (X) | — | — |
| Status word | `:X34#` → `m<RA><DEC>#`, per axis `0` stopped, `1` tracking, `2` … `5` moving (ramp, slew) ✓<br>`:X38#` (see section 8) ✓<br>`:X3C#` → `:Z1mts#` | — | `:GU#` → ordered flags `#`: `n` not tracking, `N` no goto, `p` not parked, `I` parking, `P` parked, `F` park failed, `H` at home, `h` homing, `G` / `g` guiding, `S` PPS, `(` lunar, `O` solar, `k` King, `a` auto flip, `R` PEC recorded, `E` `K` `A` `L` mount type, `o` `T` `W` pier side, rate and error digits ✓[^onstepgu]; `:Gu#` packed bytes (X); `:GE#` → last error `CC#` (X) | `:GU#` → flags as OnStep ✓[^nyx] | `:GU#` → `n` not tracking, `N` stopped or tracking, `L` low voltage, `H` at home, `G` / `Z` equatorial / alt-az mode, `S` / `s` RA / DEC stall, `T` / `t` ST4 guiding, RA and DEC flags, rates, state (e.g. `nNG001000060#`) ✓ |
| Velocity / motion state | Unsolicited frames: `:Z3ms#` m `0` stopped, `1` tracking, `2` slewing, `3` pulse guiding; s `0` not aligned, `1` east, `2` west side of pier<br>`:Z1mts#` t `0` not tracking, `1` … `3` tracking<br>`ge#` goto ended | — | `:GXF3#` / `:GXF4#` axis step frequency, `:GXFF#` / `:GXFG#` (X) | — | `:GAT#` → `0#`, `1#`, `e<n>#` ✓ |
| Other status | `:X46r#` (see section 1); `:X3E1#` / `:X3E0#` host program connected / disconnected → ∅ | — | `:GXUa#` stepper driver flags `ST`, `OA`, `OB`, `GA`, `GB`, `OT`, `PW`, `GF` (X) | `:GXU1#` / `:GXU2#` → driver flags as OnStepX plus motor load | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Status word | `:GXI#` → [0] `0` idle, `1` tracking, `2`/`3` slewing; [2] `P` parked, `I` parking; [3] `H` at home; [13] `E`/`W` pier side‡ ✓ | — | `:GX#` → `<state>,<motion>,<RA steps>,<DEC steps>,<TRK steps>,<RA>,<DEC>,<focus>,#`; state `Idle`, `Parked`, `Parking`, `Guiding`, `SlewToTarget`, `FreeSlew`, `ManualSlew`, `Tracking`, `Homing` ✓[^oat] | `:GU#` → `<T/t><P/p><S/s><W/E><rate 1…4>#` (tracking, parked, slewing, pier side, rate) ✓[^esp32go]; `:GK#` two-digit bitmask, `:Gk#` tracking | — |
| Velocity / motion state | `:GXJS#` slewing, `:GXJM1#` / `:GXJM2#` axis moving, `:GXJB#`, `:GXJC#`, `:GXJm#` → `1#`/`0#` | `:D#` (see section 4) ✓ | `:GIS#` → `1#`/`0#` slewing | — | — |
| Other status | — | — | `:XGAH#` (see section 8) | — | — |

## 13. Periodic error correction (PEC)

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| PEC on / off | — | `$Q#` toggle both axes (not Autostar)<br>`:$QZ+#` / `:$QZ-#` RA, `:$QA+#` / `:$QA-#` DEC (Autostar II) → ∅ | native `>531:` replay on, `>532:` off (L5); `>508:` replay at boot (L5.2) | `:$Q#` toggle, `:pP#` on, `:p#` off → ∅ | `:pP#` playback, `:p#` off → ∅ |
| Record | — | — | native `>530:` start training, `>535:` abort (L5) | `:pR#`; `:pRX#` (X = 0 short, 1 medium, 2 long); `:pRaX#` / `:pRzX#` altitude / azimuth (alt-az) → ∅ | `:pR#` (one worm cycle) → ∅ |
| Status | — | — | native `<509:` → status bits | — | — |
| Data | — | `:VRNNNN#` / `:VDNNNN#` RA / DEC table entry → `D.DDDD` (Autostar II, Classic 16″) | native `<501:` counter, `<502:` training speed, `<503:` / `<504:`, `<511:` / `<512:` data, `<521:` statistics, `>550:` / `>551:` SD card files | — | — |
| Smart drive / smart mount | — | `:$QS±#` SmartMount, `:$QU±#` update mode, `:$QC#` → `NNNNN#`, `:$QGNNNNN#`, `:$QP…#`, `:$QW#` → `1`/`0`, `:$QV±#` speech; `:Sm±#` flexure correction (Autostar II) | — | — | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| PEC on / off | — | — | `:$QZ+#` on ✓, `:$QZ-#` off ✓ → ∅ | — | — |
| Record | — | — | `:$QZ/#` arm recording, `:$QZZ#` clear, `:$QZ!#` save to EEPROM → ∅ | — | — |
| Status | — | — | `:$QZ?#` → `I#` off, `p#` ready to play, `P#` playing, `r#` ready to record, `R#` recording (X: optional `.` index) ✓ | — | — |
| Data | — | — | `:VRnnnn#` → `sddd#`; `:VR#` → `sddd,ddd#`; `:WRnnnn,sddd#` → `0`/`1`; (X) `:VH#`, `:Vrn#`, `:VS#`, `:VW#`, `:WR+#`, `:WR-#`, `:GX91#`, `:GXE6#` … `:GXE8#`, `:SXE7,n#` | — | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |

## 14. Auxiliary and other commands

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Reticle, display | `:B+#`, `:B-#`, `:B0#` … `:B3#` → ∅ | `:B+#`, `:B-#`, `:B<n>#` flash rate, `:BDn#` duty cycle (LX200GPS) → ∅ | `:GB#` → `n#`; `:SB<n>#` LED brightness[^conflict] | — | `:B+#`, `:B-#` → ∅ |
| Serial speed | — | `:SBn#` (1 56.7K … 9 1200) → `1` | — | `:SBn#` (0 115.2K … 9 1200) → `0`/`1` | — |
| Fan, heater, power | `:f+#` / `:f-#` fan → ∅ | `:f+#` / `:f-#` fan or accessory power, `:fH<ddd>#` corrector heater, `:fp+#` / `:fp-#` 12 V panel → ∅ | native `>311:` / `<312:` feature and encoder port bits; `<321:` / `<322:` main and lithium battery voltages (L5.2) | `:GRLYn#` → `0`/`1`; `:SRLYn,m#` → `0`/`1` relays and motor heaters (special mounts); `:GTMPOHn#`, `:GTMPTHn#`, `:STMPOHn,…#`, `:STMPTHn,…#` motor temperature thresholds | — |
| Sensors | — | `:fT#` tube, `:fC#` corrector plate temperature → `sdd.ddd#` (Autostar II, Max/RCX) | — | `:GTMPn#` → `+TTT.T#` or `Unavailable#`; `:GRTMP#` / `:GRPRS#` refraction temperature / pressure | — |
| Field rotator / de-rotator | `:r+#` / `:r-#` → ∅ | `:r+#`, `:r-#`, `:rn#`, `:rh#`, `:rC#`, `:rc#`, `:rq#` → ∅ | — | — | — |
| Network | — | — | native `>801:` … `>818:`, `<826:` IP settings, DHCP, NTP, MAC (L5) | `:GIP#`, `:GIPW#`, `:GINQ#`, `:SIP…#`, `:GWAV#`, `:GWRSC#`, `:GWRAP#`, `:GWRAP2#`, `:GWID#`, `:GWUP#`, `:SWRL…#`, `:SWRLC#` | — |
| Mount type and mode | `:AL#`, `:AP#`, `:AA#` land / polar / alt-az → ∅ | `:AL#`, `:AP#`, `:AA#` → ∅; `:GW#` first character ✓ | native `<0:` / `>0:` mount type (0 … 8); `>700:` equatorial / alt-az (L5.1) | — | `:FM#` / `:EM#` (see section 10) |
| Alignment model | — | `:Aa#` automatic alignment → `1`/`0` (Autostar II); `:G0#` … `:G2#` alignment menu entries | `:C<n>#` select model, `:Cc#`, `:C?#`, `:CR#`, `:CU#` → `<n>#`; `:CE<c>#` echo → `<c>#` (L5); native `<201:` … `<211:` model parameters | `:getalst#`, `:getaliN#`, `:getalpN#`, `:delalig#`, `:delalstN#`, `:newalig#`, `:newalptMRA,MDEC,MSIDE,PRA,PDEC,SIDTIME#`, `:endalig#` | — |
| Object library and FIND | `:LF#`, `:LN#`, `:LB#`, `:Lf#`, `:LC NNNN#`, `:LM NNNN#`, `:LS NNNN#`, `:LI#`, `:Lo N#`, `:Ls N#`; `:Gy#` / `:Sy…#`, `:Gq#` / `:Sq#`, `:Gb#` / `:Sb…#`, `:Gf#` / `:Sf…#`, `:Gl#` / `:Sl…#`, `:Gs#` / `:Ss…#`, `:GF#` / `:SF…#` | the same set as Classic, plus `:LoD#` (catalogues 0 … 5), `:LsD#` | `:OI<cat><id>#` select object, `:ON<name>#` name the target, `:OC#`, `:OR#`, `:OS#` observing log, `:Oc#`, `:Od…#`, `:On#`, `:Or#`, `:Os#` user catalogue | — | — |
| Backlash | — | `:$BAdd#` / `:$BZdd#` → ∅; `:GpB#` / `:SpB…#` (Autostar, Autostar II); `:GpH#` / `:SpH…#` home data, `:GpS#` / `:SpS…#` sensor offsets (Autostar II) | — | `:BdDD*MM:SS#` DEC, `:BrDD*MM:SS#` RA → `1` | `:Bd DD*MM:SS#` DEC → `1`; `:Br DD*MM:SS#` / `:Br HH:MM:SS#` RA → `1`; driver sends `:Br 00:00:00#` ✓ |
| Dome | — | — | — | `:GDA#`, `:GDH#`, `:GDS#`, `:GDW#`, `:GDw#`, `:GDstm#`; `:SDH#`, `:SDMn#`, `:SDSn#`, `:SDRXXXX#`, `:SDTn#`, `:SDUSS#`, `:SDXMsXXXX#`, `:SDYMsXXXX#`, `:SDZMsXXXX#`, `:SDXsXXXX#`, `:SDYsXXXX#`, `:SDAXXXX#`, `:SDAr#`, `:SDstm…#` | — |
| Logs, help, diagnostics | — | `:??#`, `:?+#`, `:?-#` handbox help text → `<string>#` | — | `:startlog#`, `:stoplog#`, `:getlog#`, `:evlog#` | — |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Reticle, display | `:TTSFr#` / `:TTRFr#` keypad off / on[^stargoflags] | — | `:B+#`, `:B-#` → ∅ | — | — |
| Serial speed | — | — | `:SBn#` → `0`/`1` (X: `:SBB#` 460800, `:SBA#` 230400, `:SB0#` 115200 … `:SB9#` 1200 → `1`) | `:SBn#` (0 115.2K … 9 1200) → `1` | — |
| Fan, heater, power | `:TTTttt#` motor current; `:TTGT#` → `tnnn#`<br>`:X2BmDDDDD#` brake (m `0` RA, `1` DEC) | — | auxiliary feature slots 1 … 8: `:GXY0#` → eight-character bitmap `#` ✓; `:GXYn#` → `name,purpose#` ✓; `:GXXn#` → state ✓; `:SXXn,Vv#` → `0`/`1` ✓; `:SXXn,Zf#`, `,Sf#` dew heater, `,Ef#`, `,Df#`, `,Cf#` intervalometer (X)[^onstepaux] | `:GX9V#` → input voltage `n.n` ✓; `:GX9F#` → MCU temperature | — |
| Sensors | — | — | `:GX9A#` temperature, `:GX9B#` pressure, `:GX9C#` humidity, `:GX9E#` dew point, `:GX9F#` MCU temperature; `:SX9A,…#`, `:SX9B,…#`, `:SX9C,…#` (X)[^conflict] | `:GX9A#` temperature ✓, `:GX9B#` pressure ✓, `:GX9C#` barometric altitude, `:GX9D#` → `pitch:roll` leveler ✓, `:GX9E#` compass ✓[^nyx] | `:Xb#` → supply voltage `#` |
| Field rotator / de-rotator | — | — | `:rA#`, `:rT#`, `:rI#`, `:rM#`, `:rD#`, `:rb#`, `:rQ#`, `:r1#` … `:r9#`, `:rW#`, `:rc#`, `:r>#`, `:r<#`, `:rG#`, `:rr…#`, `:rS…#`, `:rZ#`, `:rF#`, `:rC#`, `:r+#`, `:r-#`, `:rP#`, `:rR#`; `:GX98#` (X) | — | — |
| Network | `:TTBT1#` / `:TTBT0#` Bluetooth on / off, `:TTRBT#` reset its parameters; `:X05#` (see section 1) | — | — | `:WSxxxxx#` client SSID ✓, `:WPxxxxx#` client password ✓, `:WAxxxxx#` access point SSID ✓, `:WBxxxxx#` access point password ✓ → `0`/`1`; `:WLC#` reload ✓, `:WLZ#` reset to defaults ✓ → `0`/`1`; `:WL?#` status; `:WL>#` → access point `ssid:password` ✓; `:WLI#`; `:WLD#` → `ssid:address` ✓; `:WLS#` scan[^nyx] | `:BSnnnnnn#` → `OK#` / `NO#`; `:SBlnnnnnn#` Bluetooth name → `0`/`1`; `:GBl#` |
| Mount type and mode | `:AA#` alt-az, `:AP#` equatorial → ∅; `:GW#` (see section 12)<br>`:TTSMn#` mount type; `:TTGM#` → `n#`<br>`:TTHS0#` north, `:TTHS1#` south hemisphere; `:TTGHS#` → `h0#` / `h1#`<br>`:X2CmIIIIDDD#` custom gear ratio (m `0` RA, `1` DEC); `:X480#` / `:X481#` read it<br>`:TTSMs0hhhhhhhh#` / `:TTSMs1hhhhhhhh#` set the RA / DEC encoder | — | `:SXEM,n#` mount type for the next restart, `:GXEM#` (X) | `:SXEM,[n]#` (1 GEM, 3 alt-az) → `0`/`1`; driver sends `:SXEM,1#` ✓; `:GXEM#`<br>`:SX91,U#` unlock / `:SX91,L#` lock the RA brake → `0`/`1`; driver sends `:SX91,U#` ✓ | `:AP#` equatorial, `:AA#` alt-az (restart needed) → ∅; `:GU#` `G` / `Z` ✓ |
| Alignment model | `:X473#` number of alignment points; `:X55Tnn#` alignment point nn (T = `R`, `D`, `L`, `Z`, `E`) → `<T><nn><8 characters>#`<br>`:X45n#` → `f<n><8 digits>#` model offsets (`0` / `1` single star RA / DEC, `2` / `3` multistar)<br>`:X470#` none, `:X471#` three star mode; `:X474#` / `:X475#` N star off / on, `:X476#` reads it; `:X477#`, `:X478#` N point triangle modes; `:X479#` main offset<br>`:X4F00#` clear, `:X4F95#` save, `:X4F96#` restore the model<br>flags `f`, `C`, `R`, `B`[^stargoflags] | — | `:A1#` … `:A9#` start, `:A+#` accept, `:AW#` write → `0`/`1`; `:A?#` → `mno#` (X); `:GX00#` … `:GX0E#`, `:SX00,n#` … `:SX0E,n#` model upload (X) | `:A?#` → `mno#`; `:A[n]#`, `:A+#` → `0`/`1`; `:AW#` → `1` | `:NSC#` clear multi-star calibration → `1` ✓ (firmware 1.2.4 and later) |
| Object library and FIND | — | — | `:Lonn#`, `:LB#`, `:LN#`, `:LCnnnn#`, `:L$#`, `:LI#`, `:LR#`, `:LWssss,ttt#`, `:LD#`, `:LL#`, `:L!#`; (X) `:LIG#`, `:L?#` | — | — |
| Backlash | — | — | `:$BRnnn#` RA, `:$BDnnn#` DEC → `0`/`1`; (X) `:%BR#`, `:%BD#` → `n#` | `:$BR[n]#`, `:$BD[n]#` → `0`/`1`; `:%BR#`, `:%BD#` → `n#` | — |
| Logs, help, diagnostics | — | — | `:ECtext#` echo to debug output (X) | — | — |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Reticle, display | `:B+#`, `:B-#` → ∅ | — | — | — | — |
| Sensors | — | — | `:XL0#` / `:XL1#` digital level off / on; `:XLGR#`, `:XLGC#` → `<pitch>,<roll>#`; `:XLGT#` → `<temp>#`; `:XLSR#`, `:XLSP#` | — | — |
| Network | — | — | `:XGN#` → `1,<mode>,<status>,<hostname>,<address>:<port>,<SSID>,<OATHostname>#` or `0,#` | — | — |
| Mount type and mode | `:S!1#` … `:S!4#` mount type → `1`; `:SXOI,V#` / `:GXOI#`, `:SXON,…#` / `:GXON#`, `:SXOS,NNN#` / `:GXOS#` | — | — | — | — |
| Alignment model | `:AW#`, `:A0#`, `:A*#`, `:A2#`, `:AE#`, `:AC#`, `:AA#` → `0`/`1` | — | `:SHHH:MM#` hour angle of Polaris → `1`/`0`; `:XGH#` → `HHMMSS#` | — | — |
| Backlash | `:SXMBn,VVVV#` / `:GXMBn#`, `:SXMbn,VVVV#` / `:GXMbn#` (n = `R` or `D`) | — | `:XSBn#` → ∅; `:XGB#` → `integer#` | — | — |
| Logs, help, diagnostics | motor setup: `:SXMGn,…#` / `:GXMGn#` gear, `:SXMSn,…#` / `:GXMSn#` steps, `:SXMMn,…#` / `:GXMMn#` microsteps, `:SXMRn,…#` / `:GXMRn#` reverse, `:SXMcn,…#` / `:GXMcn#`, `:SXMCn,…#` / `:GXMCn#` currents; encoders `:EAS#`, `:EAE#`, `:ECT#`, `:ECE#`, `:ECS#`, `:ED#`, `:EMS#`, `:EMU#`, `:EMA#`, `:EMQ#` | — | `:XDnnn#` drift alignment; `:XGR#` / `:XSRn.n#` RA steps, `:XGD#` / `:XSDn.n#` DEC steps; `:XGHR#`, `:XGHD#`, `:XGHS#` → `N#`/`S#`; `:XGDP#`, `:XSDPnnnn#` (obsolete) | — | — |

## 15. Native and extended command sets

| Function | LX200 Classic | Autostar / LX200GPS | Gemini L4/L5 | 10micron | AP GTO |
| --- | --- | --- | --- | --- | --- |
| Native get | — | — | `<id:<checksum>#` → `<value><checksum>#`; undefined id → `#` ✓ (driver: ids 21, 27, 130, 150)[^gemnative] | — | — |
| Native set | — | — | `>id:<value><checksum>#` → ∅ ✓ (driver: ids 131 … 134, 150, 191, 192) | — | — |
| Vendor extensions the driver uses | — | `:GVF#`‡, `:Rg…#` | ACK startup, `:Gv#`, `:Gm#`, `:h?#`, `:hZ#`, `:hW#`, `:p0#`, `:GV#` | `:EMUAP#`, `:U1#`, `:Gstat#`, `:KA#`, `:PO#`, `:TSOLAR#` | `:V#`, `:GOS#`‡, `:G_E#`‡, `:G_S#`‡, `$Kn#`‡, `:RT…#`, `:RC1#` … `:RC3#`, `:CMR#`, `:pS#`, `:Br …#` |
| Other extended families (unused) | — | `$B…`, `$Q…`, `:f…`, `:g…`, `:L…`, `:r…`, `:V…` (Autostar II) | `:C…`, `:M…`, `:O…`, native ids (table below) | alignment model, dome, Wi-Fi, LAN, relays, temperatures, Julian date, leap seconds, `:NUDGE…#`, axis-angle gotos `:SaXa…#`, `:SaXb…#`, `:MaX#`, `:PaX#` | `:Rc…`, `:Rs…`, `:RR…`, `:RD…`, `:ho#`, `:hq#`, `:de#`, `:dn#` |

| Function | StarGO | StarGO2 | OnStep / OnStepX | NYX-101 | ZWO AM |
| --- | --- | --- | --- | --- | --- |
| Native get | — | — | `:GX..#` families (see the sections above) | `:GX..#` families | compound gets `:GMTI#`, `:GMGE#`, `:GMEQ#`, `:GMeq#`, `:GMZA#` |
| Native set | — | — | `:SX..,…#` families → `0`/`1`; checksummed frame `;CC…CCS#` (X) | `:SX..,…#` families → `0`/`1` | compound sets `:SMTI…#`, `:SMGE…#`, `:SMeq…#`, `:SMMC…#` |
| Vendor extensions the driver uses | `:X20pp#`, `:X21pp#`, `:X22#`, `:X120#`, `:X122#`, `:X32HHMMSS#`, `:X34#`, `:X38#`, `:X361#`, `:X362#`, `:X370#`, `:TTSFh#` | — | `:GU#`, `:GT#`, `:Te#`, `:Td#`, `:TK#`, `:R1#`/`:R4#`/`:R7#`/`:R9#`, `:hQ#`, `:hR#`, `:hF#`, `:hC#`, `:$QZ…#`, `:GX95#`/`:SX95,…#`, `:GX96#`/`:SX96,…#`, `:GXE9#`/`:GXEA#`/`:SXE9,…#`/`:SXEA,…#`, `:Gh#`/`:Go#`/`:Sh…#`/`:So…#`, `:Fa#`, `:FR…#`, `:FT#`, `:GXY…#`, `:GXX…#`, `:SXX…#` | `:GRH#`, `:GDH#`, `:GU#`, `:Sv…#`, `:SXEM,1#`, `:SX91,U#`, `:RE…#:RA…#`, `:W…#`, `:GX9A#`, `:GX9B#`, `:GX9D#`, `:GX9E#`, `:GX9V#`, OnStep set | `:GV#`‡, `:GU#`, `:Gm#`, `:Ggr#`, `:Rg…#`, `:GBu#`‡, `:SBu0#`/`:SBu1#`/`:SBu2#` buzzer off/low/high‡, `:Te#`, `:Td#`, `:hC#`, `:R…#`, `:GTa#`/`:STa…#`, `:GRl#`/`:SRl…#`, `:GAT#`, `:NSC#`, `:hP#`, `:Gps#`, `:Spu#`, `:Sp01#` |
| Other extended families (unused) | `:X…#` (AUX, SteelDrive, gears, model, park and home, Sun goto); `:TTSF<f>#` set, `:TTRF<f>#` clear, `:TTGF<f>#` read flag f[^stargoflags]; `:TTSM…`, `:TTGM…`, `:TTMV…`, `:TTMX…`, `:TTT…`, `:TTHS…`, `:TTBT…`; unknown purpose: `:X07x#`, `:X18mmm#`, `:X19txx#`, `:X2Dt#`, `:X2Eaaabbbccc#`, `:X2F#`, `:X3Asdddddddde#`, `:X3Bsdddddddde#`, `:X3Fb#`, `:X41sRRR#`, `:X42#`, `:X43sDDD#`, `:X44#`, `:X49#`, `:X4Axx#`, `:X4C…#`, `:X4EppTDDDppppp#`, `:X52ss#`, `:X53#`, `:X54HH#`, `:X56xhhhhhhhh#`, `:X57xx#`, `:X58x#`, `:X5B…#`, `:XFC#` | — | axis service `:GXAa,p#` → `value,min,max,type,name#`, `:GXAa,M#`, `:GXAa,0#`, `:GXSa#`, `:SXAC,n#`, `:SXAa,R#`, `:SXAa,p,value#`; encoder sync `:SEO#`, `:SX40,n#` … `:SX44,…#`; `:GXEE#`, `:GXEF#`, `:GXEG#`, `:GXEM#`, `:GXFA#`; buzzer `:SX97,n#` (X) | `:GX97#`, `:GVU#`, `:GVY#` | Bluetooth `:BS…#`, `:SBl…#`, `:GBl#`; firmware update `:UP#`, `:WFn#` (image size), `:HCUPn#`, `:WEBUPn#`, `:OK#` |

| Function | TeenAstro | aGotino | OAT | ESP32Go | Generic |
| --- | --- | --- | --- | --- | --- |
| Native get | `:GX..#` families | — | `:XG…#` families | `:GU#`, `:GK#`, `:Gk#`, `:Gx#`, `:PP#`, `:pS#` | — |
| Native set | `:SX..,…#` families → `0`/`1` | — | `:XS…#` families | `:hS#`, `:cRR#` | — |
| Vendor extensions the driver uses | `:GXI#`‡, `:Te#`, `:Td#`, `:hQ#`, `:hR#`, `:hB#`, `:hC#`, `:U#`‡ | `:D#` | `:GX#`, `:MT0#`/`:MT1#`, `:XSS…#`, `:hU#`, `:P#`‡ | `:GU#`, `:TK#`, `:hS#` | — |
| Other extended families (unused) | refraction `:SXr…#`, limits `:SXL…#`, motors `:SXM…#`, mount `:SXO…#`, `:GXJ…#` | — | digital level `:XL…#`, homing `:XGAH#`, configuration `:XGM#`, `:XGMS#`, `:XGN#` | `:GVD#`, `:GVT#`, `:GVF#` | — |

### Gemini native command ids

Get with `<id:<checksum>#`, set with `>id:<value><checksum>#`. The checksum is the XOR of all
transmitted characters including `<` or `>` and `:`, with the top bit cleared and 64 added. A get
answers `<value><checksum>#`; an id the controller does not know answers a bare `#`. Ids marked ✓
are used by the driver.

| Id | Get | Set | Meaning |
| --- | --- | --- | --- |
| 0 | 0 … 8 | 0 … 8 | Mount type: 0 custom, 1 GM-8, 2 G-11, 3 HGM-200, 4 MI-250, 5 Titan, 6 Titan50, 7 G-10, 8 G-12 |
| 10 … 15 | 10 | 11 … 15 | Axis encoder port status: use / test / ignore encoders, use / do not use end switches |
| 21 ✓, 22 | ±80 … 720 | ±80 … 720 | RA / DEC worm gear ratio (sign = direction) |
| 23, 24, 33, 34 | 10 … 150 | 10 … 150 | RA / DEC spur gear ratio (33, 34 in double format) |
| 25, 26 | 100 … 2048 | 100 … 2048 | RA / DEC motor encoder resolution |
| 27 ✓, 28 | 2000 … 25600 | — | RA / DEC steps per worm revolution |
| 51 | 0 / 1 | — | Mainboard version (L5.2) |
| 81 | ENQ macro | — | Coordinates and states string (L5.1) |
| 91 | 0 / 1 | 0 / 1 | Native checksum behaviour (L5) |
| 92 | 0 … 2 | 0 … 2 | Park behaviour: what wakes the mount (L5.1) |
| 96 | bits | — | Startup circumstances (L5.1) |
| 97 | eight characters | — | State check counters (L5) |
| 99 | bits | — | Status: aligned, modelling, object selected, goto, RA limit, J2000 |
| 100, 110 | ±2048 … 32768 | same | RA / DEC encoder resolution |
| 101, 111 | value | — | RA / DEC encoder value |
| 120 … 122 | 20 … 2000 | 20 … 2000 | Manual slewing speed (both, RA, DEC) |
| 130 ✓, 131 … 137 ✓ | 130 | 131 … 137 | Tracking rate: 131 sidereal, 132 King, 133 lunar, 134 solar, 135 terrestrial, 136 closed loop, 137 comet / user |
| 140 … 142 | 20 … 2000 | 20 … 2000 | Goto slewing speed (both, RA, DEC) |
| 145 … 147 | 20 … 2000 | 20 … 2000 | Move speed (both, RA, DEC) (L5) |
| 150 ✓, 151, 152 | 0.2 … 0.8 | 0.2 … 0.8 | Guiding speed (both, RA, DEC) |
| 160, 161 … 163 | 160 | 161 … 163 | Hand controller mode: visual, photo, all speeds |
| 170 … 172 | 1 … 255 | 1 … 255 | Centering speed (both, RA, DEC) |
| 180, 181, 182 | 180 | 181, 182 | Alarm off / on |
| 190, 191 ✓, 192 ✓ | 190 | 191, 192 | RA motor stopped / moving (tracking off / on) |
| 200 | 0 … 255 | 0 … 255 | TVC step count |
| 201 … 209, 211 | ±0 … 65535 | same | Modelling parameters A, E, NP, NE, IH, ID, FR, FD, CF, TF (″) |
| 220 … 223 | `ddddmm;…` | same | Safety limits (east, west) and western goto limit |
| 225, 226 | value | — | Steps / seconds to the western goto limit (L5) |
| 230, 231 | `east;west` | — | Physical safety limits (clusters of 256 ticks / ticks) |
| 235 … 239 | `ra;dec` | — | Axis positions, remainders, half circle |
| 245, 246 | `ra;dec` | — | Servo lag and duty cycle (L5.1) |
| 311, 312 | 0 … 63 | 0 … 15 | Feature port / encoder port bits |
| 321, 322 | volts | — | Main / lithium battery voltage (L5.2) |
| 411 … 416, 421, 422 | divisor | divisor | Comet and guiding rate divisors, sidereal divisors |
| 501 … 504, 508, 509, 511, 512, 521 | various | various | PEC counter, training speed, maximum, step limit, boot playback, status, data, statistics |
| 530, 531, 532, 535, 550, 551 | — | command | PEC training, replay on / off, abort, load / store (L5) |
| 601 … 604 | — | command | Display language (L5) |
| 700 | 0 / 1 | 0 / 1 | Mount design: equatorial / alt-az (L5.1) |
| 801 … 805, 810 … 816, 818, 826 | network | network | IP address, mask, gateway, name servers, DHCP, NTP, MAC (L5) |
| 910 … 912 | file | file | Open, read, delete a file |
| 43610, 43611, 43690, 43691 | — | command | Load / store configuration, reset to defaults |
| 65533 … 65535 | — | command | Reboot (65533 with a cold start) |

## Commands whose meaning depends on the dialect

The same letters mean different things on different controllers. The driver keeps them apart by
dialect; a client that talks to a mount directly has to do the same.

| Command | Meanings |
| --- | --- |
| `:Ga#` | 12-hour local time (Meade, OnStep, NYX, TeenAstro, OAT); target altitude (10micron) |
| `:GH#` | Daylight saving flag (Autostar II, ZWO); hour angle (Gemini) |
| `:Gh#` / `:Go#` | High and low altitude limits (Meade, 10micron); horizon and overhead limits (OnStep, NYX); `:Gh#` is "has homed" on ZWO |
| `:Gm#` | Distance to the meridian (Meade Max); side of pier (Gemini, OnStep, NYX, ZWO, TeenAstro) |
| `:Gv#` | Velocity (Gemini); site elevation (OnStepX, NYX) |
| `:GW#` | Alignment status (Autostar); RA velocity (Gemini L5); four-character status (OnStepX); axis mode `PT0`/`AT0` (StarGO) |
| `:GX9E#` | Compass heading (NYX); dew point (OnStepX) |
| `:SB…#` | Serial speed (Meade, 10micron, OnStep, NYX); LED brightness (Gemini); `:SBu…#` buzzer (ZWO) |
| `:hC#` | Park at the startup position (Gemini); calibrate home (Autostar II); goto home (OnStep, NYX, ZWO, TeenAstro) |
| `:hF#` | Home search and align (Classic, Autostar, 10micron); goto home while tracking (OAT); reset the mount at home (OnStep, NYX); sync at home (TeenAstro) |
| `:hS#` | Home search saving the position (Classic); set park position (Autostar II); seek home and store (10micron); set home (ESP32Go) |
| `:hP#` | Park (most); goto home and mark parked (ESP32Go) |
| `:hZ#` | Park at the zenith (Gemini L5); set AZ/ALT home (OAT) |
| `:P#` | High precision pointing toggle (Meade, Gemini); no-op (10micron) |
| `:RG#` … `:RS#` | Button rates (most); `:RS#` only sets the goto speed on Astro-Physics; ZWO maps `:RG#` 0.5x, `:RC#` 1x, `:RM#` 720x, `:RS#` 1440x |
| `:U#` | Toggle (Meade, Gemini, 10micron); one-way long format (Astro-Physics) |
| `:FM#` | Focus medium (Gemini); no meridian flip (Astro-Physics); maximum focuser position (OnStep) |

## Notes

[^classic]: The Classic column follows the LX200 Classic manual alone. The 2010 protocol lists the ACK query and some other commands for the LX200 models below 16″ and the 16″ as well; they are in the Autostar column. The Classic has no `:GVP#`, so the dialect has to be selected explicitly.

[^classicq]: The driver sends `:Q#` to an LX200 Classic as soon as the port is open, so that a guide motion started by an earlier connection cannot keep running.

[^ack]: Firmware that does not answer `:GW#` is identified by the ACK reply; `L` (land) is an alt-az mount that does not track. The driver also sends ACK after `:CM#` to an Autostar to push out a late sync reply (see the note on the Autostar `:CM#` reply).

[^short]: The driver sends the precision command only when a `:GR#` reply is shorter than 8 characters, i.e. in short format: `:P#` to a Meade (not to an LXD600, which has no long format) and to an OpenAstroTracker, `:U#` to Gemini, Astro-Physics, OnStep and TeenAstro, `:U1#` to a 10micron. Astro-Physics also gets `:U#` and 10micron `:U1#` at connect.

[^gemfmt]: Gemini in high precision separates the degrees of a declination with `:` (`sDD:MM:SS#`), in low precision with `°` (`0xDF`). Double precision (`:u#`, L5) answers every coordinate, and `:GL#`, as a signed decimal with six digits. The driver never selects it, but parses such replies when another client has left the mount in that mode.

[^10mstar]: On a 10micron the degree mark in replies is `0xDF` in LX200 emulation and ASCII `*` in extended (Astro-Physics) emulation; ultra precision uses `:`. The driver selects the extended emulation (`:EMUAP#`) and high precision (`:U1#`).

[^conflict]: See "Commands whose meaning depends on the dialect" above.

[^agotino]: aGotino separates the degrees in `:GD#` with `0xDF` (the driver rewrites the fourth byte). `:GR#` and `:GD#` keep reporting the start position until a slew ends, and after an aborted slew the sketch reports the goto target as its position. `:Q#` (and any `:Q…#`) stops declination and returns right ascension to tracking. Commands it does not know, including `:Gt#`, `:Gg#`, `:GC#`, `:GL#`, `:GG#`, `:GS#`, `:GW#` and `:Mg…#`, are dropped without any reply, so the driver never sends them; it has no tracking switch, park, home, rates, pier side, clock or pulse guiding.

[^esp32go]: ESP32Go marks the degrees with the byte `0xE1` in `:GD#`, `:Gt#` and `:Gg#` (and in the altitude and azimuth); the driver replaces it with `*`. `:GR#` has tenths of a second. `:AP#` and `:AL#` are accepted but do not change the tracking, so the driver offers no tracking switch. `:hP#` goes to the home position and raises a parked flag that only a goto or a sync clears; the driver uses it as "home" and offers no park. `:GT#` always answers `50.0`; the tracking rate is the last character of `:GU#` (1 sidereal, 2 solar, 3 lunar, 4 King). `:GW#` has no reply. With the clock unset `:GR#` can answer a malformed negative value and a right ascension goto does not converge, so the clock is set at connect.

[^oat]: OpenAstroTracker (firmware V1.13.16 document, V1.13.20 controller): `:GD#` puts an arcminute mark before the seconds (`+45*00'00#`; the driver replaces `'` with `:`); `:Gt#` and `:Gg#` have whole arcminutes; an unsigned `:Sg…#` is answered `0`, the signed form is accepted. `:GX#` reports `Parked` for every idle mount, so the driver keeps the park state itself; `:hU#` only switches tracking on and answers `1`; `:hP#` has no reply; tracking stops while a slew runs. `:GW#`, `:Gv#` and `:GVF#` have no reply. The guide command is documented as `:MGdnnnn#`; the driver sends `:Mgdnnnn#`. Rates are tracking factors: `:XSS1.000#` sidereal, `:XSS0.997#` solar, `:XSS0.965#` lunar.

[^gemms]: A Gemini refusal is the code digit followed by its text and `#` (e.g. `6Outside Limits.#`); success is `0` alone. Code 7 is Level 5. The driver reads the text and reports it.

[^cmlate]: An older Autostar sends the `:CM#` reply only when the next command arrives. The driver follows `:CM#` with ACK when nothing came back and takes any answer as a completed sync; a bare `#` is an empty reply.

[^gemcm]: `No object!#` means the sync was refused (mount not aligned or no object selected) and the position is unchanged; a successful sync answers the object name.

[^apsync]: `:CM#` takes the side of the pier from where the mount points and redefines it; `:CMR#` keeps the side known since the last `:CM#` or `:MS#`. The driver uses `:CMR#` unless configured otherwise.

[^dpark]: An Autostar `:D#` answers one bar until the slew is complete and then an empty string; the driver uses it to see a park complete. An Autostar that has parked with `:hP#` stops answering until it is switched off and on, and the driver then sends it nothing more.

[^onsterr]: OnStep `:MS#`: `0` accepted, `1` below horizon, `2` no object, `4` position unreachable, `5` not aligned, `6` outside limits. OnStepX and NYX-101: `0` accepted, `1` below the horizon limit, `2` above the overhead limit, `3` controller in standby, `4` parked, `5` goto in progress, `6` outside limits, `7` hardware fault, `8` already in motion, `9` unspecified. OnStepX answers `6` while the site is not set. A NYX-101 `:CM#` refusal is `E<n>#` with the same codes.

[^zwoerr]: ZWO error codes: `e1` parameter out of range, `e2` format error, `e3` homing / slewing / goto in progress, `e4` moving, `e5` below the horizon, `e6` below the altitude limit, `e7` time and site not set, `e8` tracking past the meridian, `e9` sync point on the other side of the meridian, `e10` alt-az altitude inverted, `e11` sync near the pole refused, `e12` sync too far from the current position, `e13` target above the altitude limit. A set command is answered with one character, or with `e` followed by the code up to `#`.

[^zwopark]: Park and home are different on a ZWO AM. `:hC#` slews to the fixed mechanical zero (counterweight down, pointing at the pole) and latches nothing. `:hP#` slews to the park position and latches a parked state (`:Gps#` → `2#`) that only `:Spu#` releases; unpark does not move the mount. Park is available in equatorial mode only (`:GU#` without `Z`) and from firmware 1.1.9. The driver parks from home: it sends `:hC#` first when `:GU#` has no `H`, waits for the home slew to end (`H` from firmware 1.8.1, `N` before) and `:hP#` follows; a stage that has not started to move within 5 s is given up. A parked mount (`:Gps#` not `0`) is not sent goto, manual motion or go-home. `:Sp01#` stores the current position of an idle mount (last digit of `:GU#` `0`). On firmware below 1.8.1 a parked mount stands at azimuth `270*00:00#` and altitude `±00*00:00#` or `±00*00:01#` by default. There is no command that reads or clears the custom park position. The driver shows `MOUNT_PARK` from firmware 1.1.9 and `MOUNT_PARK_SET` from 1.3.0, the firmware that brought separate park and home positions.

[^teenerr]: TeenAstro `:MS#` / `:MA#`: `0` none, `1` below horizon, `2` no target, `4` parked, `5` slewing, `6` outside limits, `7` guiding, `8` above overhead, `9` motor fault.

[^hosttimed]: The LX200 Classic, and Autostar firmware before 31Ee, have no `:Mg` command. The driver guides them by selecting the guide rate with `:RG#`, starting `:Mn#`, `:Ms#`, `:Me#` or `:Mw#`, and sending the matching `:Qn#` … `:Qw#` when the pulse time has passed; manual motion and gotos are refused meanwhile.

[^gemmg]: Gemini Level 4 converts the pulse time into motor encoder ticks and cuts anything above 255 ticks modulo 256. The driver splits a longer Level 4 pulse into parts below that limit, computed from the RA worm ratio (native 21), the steps per worm turn (27) and the guiding speed (150).

[^rg]: `:RgSS.S#` exists on Autostar II models; the driver uses it on products `LX2001` (LX200GPS), `LX800` and `RCX…`, as percent × 15.0417 / 100 written `:Rg%04.1f#`. The rate is added to or subtracted from the tracking rate and must not exceed 15.0417″/s.

[^meadetrack]: `:AA#` and `:AP#` set the alignment and start tracking in it. The driver restores the alignment the mount reported at connect (`A` → `:AA#`, `P` or `G` → `:AP#`) and does not switch a mount whose alignment it does not know.

[^10mtrack]: For a 10micron the driver uses the generic tracking path: it sends `:GW#` (not in the 10micron document) and then `:AA#` if the reply starts with `A`, otherwise `:AP#`. The 10micron document defines only `:AP#` and `:AL#`.

[^autostargt]: An Autostar answers with one decimal: `60.1` sidereal, `60.0` solar, `57.9` lunar; it has no King rate. On the Classic `60.0` is solar and `57.9` lunar as well (manual frequency table).

[^onstepgu]: In `:GU#` the tracking rate is `(` lunar, `O` solar, `k` King and no character for sidereal; `N` (no goto) and `n` (not tracking) are independent. OnStepX 10.28x never reports `k`, so the driver tells sidereal from King by `:GT#`: 60.164 Hz sidereal, 60.136 Hz King, `0` while tracking is off.

[^zwogt]: The ZWO document is inconsistent: the English text gives `0` sidereal, `1` solar, `2` lunar (and lists the replies as `1#`, `2#`, `3#`), the Chinese text `0` sidereal, `1` lunar, `2` solar. The driver uses the Chinese mapping.

[^gemh]: `:h?#` answers `0` both for a park the controller never received and for one that failed. The driver allows 5 s for the controller to acknowledge a park with `2` before it reads `0` as a failure. `1` stays after `:hW#`, so the driver treats the mount as parked only while `:Gv#` also answers `N`.

[^apgg]: GTOCP1/GTOCP2 boxes answer `:GG#` for offsets east of Greenwich with coded values: `:A5` … `:A1` for −1 … −5 h, `:00` for −6 h, `:@9` … `:@4` for −7 … −12 h (followed by `:MM.M#` or `:MM:SS.S#`). The driver decodes them.

[^apsc]: The Astro-Physics `:SC#` reply has no leading `1`: it is 32 blanks and `#`, twice. The driver reads both strings.

[^gemsg]: The Gemini clock runs at UTC, so `:SG#` must come before `:SC#` and `:SL#`; the driver sends them in that order to a Gemini and as `:SC#`, `:SG#`, `:SL#` to every other dialect. Level 5 also answers `:GG#` as `±hh:mm:ss#`.

[^stargo]: The StarGO marks the degrees with `t` in the latitude (`sDDtMM:SS#`) and with `g` in the longitude (`sDDDgMM:SS#`); the driver replaces the letter with `*`. The longitude is signed and positive to the east (−180 … +180) in both `:Sg…#` and `:Gg#`. The StarGO has no calendar and no clock to read (no `:GC#`, `:GL#`, `:SC…#`); it computes gotos from the local sidereal time set with `:X32HHMMSS#`. The driver ignores the replies to `:St…#` and `:Sg…#`.

[^stargoflags]: `:TTSF<f>#` sets, `:TTRF<f>#` clears and `:TTGF<f>#` reads a StarGO configuration flag: `d` force meridian flip, `S` only one side (no meridian flip), `F` shortest path in alt-az (set by default), `h` ST4 port enabled, `r` keypad disabled, `f` three star (set) or one star (cleared) alignment, `C` single star (set) or double star (cleared) mode, `R` replace a single star, `B` special sync, `3` tracking, `G` automatic status reports. The flags `W`, `c`, `e` and `s` also exist. The driver sends only `:TTSFh#`, at connect, and leaves `d` to the mount configuration.

[^ghgo]: The pair `:Gh#` / `:Go#` is reversed between families: Meade and 10micron `:Gh#` is the highest altitude and `:Go#` the lowest, OnStep and NYX `:Gh#` is the horizon (lowest) and `:Go#` the overhead (highest) limit.

[^nyx]: NYX-101 firmware 1.32.1 differs from the document: `:GU#` uses lowercase `k` for King and lowercase `o` for pier side none (documented as `K` and `O`); `:hR#` exists and answers `0`/`1`; `:hQ#` answers `0` while parked; `:hF#` puts the controller into cold-start standby, after which `:hP#` answers `0`; `:GT#` answers `0` while tracking is off; `:WL>#` answers `ssid:password`; `:GX9D#` is signed, e.g. `34.5:-0.7`. From firmware 1.32.0 the client SSID and password sent with `:WS…#` and `:WP…#` are base64 encoded.

[^focdir]: The Classic manual says `:F+#` starts focus out and `:F-#` focus in; the 2010 protocol says `:F+#` moves inward (toward the objective). The driver sends `:F+#` for positive steps and offers a reverse switch.

[^onstepfoc]: OnStep answers `:Fa#` with `1` only in a build with a focuser; without one every focuser command, `:FT#` included, answers `0`, and the driver does not bring up the focuser device. A move is `:FR±n#` followed by `:FT#` polls until `S`, with a 120 s limit.

[^gemv]: `!` (stalled axis) is reported from Level 5; Level 4 knows `N`, `T`, `G`, `C`, `S`. `:Gv#` is the faster of the two axes, so `S` or `C` hides the tracking that continues underneath.

[^onstepaux]: `:GXY0#` answers `0` (one character) in a build without auxiliary features. `:GXYn#` purpose `1` is a switch (driver: power outlet, `:GXXn#` → `0`/`1`), `2` an analog output (driver: heater outlet, `:GXXn#` → `0` … `255`).

[^gemnative]: See "Gemini native command ids" for the checksum and the ids. A set command has no reply, so a successful write only means the bytes were sent.
