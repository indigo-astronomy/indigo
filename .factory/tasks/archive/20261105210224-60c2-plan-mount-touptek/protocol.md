# ToupTek SH20 serial protocol (reconstructed)

ToupTek has not published a protocol document for the ATLAS SH20. This reference is reconstructed from
ToupTek's own INDI driver and the INDI LX200 base classes it inherits from, and is cross-checked
against the ZWO AM5 protocol. The SH20 dialect is clearly derived from the ZWO AM5 dialect.
Every row says where the fact comes from and how sure we are of it. Facts marked **unconfirmed** must be
checked on hardware or with ToupTek before the INDIGO driver depends on them (see
[Open questions](#open-questions)).

When the driver is implemented, this file is planned to move to `indigo_drivers/mount_touptek/PROTOCOL.md`
(plan step 1).

## Sources

| Id | Source | Revision |
| --- | --- | --- |
| S1 | INDI `drivers/telescope/toup_sh20.cpp`, `toup_sh20.h` by ToupTekSoftware, INDI PR #2442 "Add TOUPTEK SH20 telescope driver", merged 2026-07-08, tested by the author on a real SH20 with KStars | commit `64096c8d107f9fa9792860df74556a1b29b2176d` (unchanged on INDI master `2bedd4aeafbcfc2162f2b88a1bf09f776fec4fbd`) |
| S2 | INDI `drivers/telescope/lx200telescope.cpp`, `lx200driver.cpp`, `lx200driver.h`. These are the base-class commands S1 inherits but does not override | INDI master `2bedd4aeafbcfc2162f2b88a1bf09f776fec4fbd` |
| S3 | ZWO mount serial communication protocol v2.1 (`indigo_drivers/mount_asi/docs/ZWO_Mount_communication_protocol_v2.1.pdf`), used only as a cross-reference for the shared AM5 heritage | in repository |
| S5 | ToupTek **TMount** ASCOM server (`TMount.exe`, .NET) inside the official "ToupTek Astro Equipment Driver" installer `ToupTekASCOMSetup.exe` (download list dated 2026-09-16, SHA-256 `a7489d850e17a44e077d8229dd460a122c018334cc5a17700ea143c3ee687642`). Studied by static IL inspection for interoperability. Nothing was executed, and no binary or code is stored in the repository | as downloaded 2026-11-06 |
| S6 | ToupTek *ATLAS SH20 User Manual* (`https://www.touptek-astro.com/dl_manual/ATLAS_SH20_en.pdf`), chapter 6 (TMount connection) and chapter 11 (firmware) | as downloaded 2026-11-06 |
| S4 | Retail descriptions of the ATLAS SH20: strain-wave mount, EQ/Alt-Az switchable, ASCOM/INDI/INDIGO support claimed, pulse guiding through the driver | agenaastro.com, proastronomy.co.uk |

The INDI source is LGPL. Only protocol facts are taken from it; no code is copied into INDIGO.

Confidence levels:
- **S1-tested**: S1 sends this command explicitly, and its author tested that feature on hardware. The PR lists connection, manual motion, mount type and pier side.
- **S1**: S1 sends this command explicitly, but the PR does not list the feature as tested.
- **S2-inherited**: S1 does not override this behavior, so the INDI base class sends the generic LX200 command to the SH20. The command is used, but nothing in the source shows that the SH20 implements it as Meade does.
- **unconfirmed**: an inference, or S1 and S3 disagree.

## Transport and framing

| Item | Value | Confidence |
| --- | --- | --- |
| Physical link | USB serial (the INDI LX200 base also offers TCP; S1 includes `connectiontcp.h` but has no SH20-specific network code) | S1/S2; whether the SH20 has Wi-Fi/TCP is **unconfirmed** |
| Serial settings | INDI LX200 default 9600 8N1. S1 does not change the baud rate | **unconfirmed** for the SH20 |
| Command frame | `:` + command + `#`, ASCII. The maximum command length is 62 characters (from S3) | S1 |
| Reply terminator | `#` (`0x23`) for string replies. Boolean replies are a single `0`/`1` byte **without** `#` | S1 |
| Reply timeout | 3 s (S1 `DRIVER_TIMEOUT`) | S1 |
| Input flush | S1 flushes both directions before every command and flushes again after reading the reply | S1 |
| Error replies | S3 (AM5) answers some commands with `e<n>#` instead of `0`. S1 reads a single byte, so it cannot tell `e` apart from success (see notes N3, N4) | **unconfirmed** for the SH20 |
| Degree sign | `*` (ASCII) in commands, as in S1 format strings. Replies are parsed with INDI `f_scansexa`, which accepts any separator | S1 |

Reply notation: `HH` hours, `MM` minutes, `SS` seconds, `s` sign, `DD`/`DDD` degrees, `n` digit, `∅` no reply.

## 1. Connection and identification

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Connection probe | `:GR#` → `HH:MM:SS#` | S1-tested | Tried twice, 250 ms apart. Any parseable RA reply counts as success |
| Precision detection | `:GR#`; if `HH:MM.T#` then `:U#` ∅ and `:GR#` again | S2-inherited | INDI `checkLX200EquatorialFormat`. S3 always answers in high precision. The INDIGO driver should not toggle with `:U#` (N8) |
| Model/firmware query | — | — | S1 sends no identification command. Whether `:GV#`/`:GVP#` (AM5) work is **unconfirmed** (Q2) |

## 2. Coordinates

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Get RA | `:GR#` → `HH:MM:SS#` | S1-tested | JNow |
| Get DEC | `:GD#` → `sDD*MM:SS#` | S1-tested | |
| Set target RA | `:SrHH:MM:SS#` → `1`/`0` | S2-inherited | Long format. INDI reads one byte |
| Set target DEC | `:SdsDD*MM:SS#` → `1`/`0` | S2-inherited | `-00*MM:SS` for negative zero. S3: an error may be `e2#` (N4) |
| GOTO | `:MS#` → `1` success, `0` failure | S1 | The S1 comment says "for sh20 0:failed, 1:success", which **inverts the Meade convention** (Meade `0` = OK). S3 documents `e<n>#` on error (N3) |
| Sync | `:CM#` → string up to `#` | S2-inherited | S3: `N/A#` on success, `e2#` on error. INDI ignores the content (N5) |
| Stop all | `:Q#` ∅ | S2-inherited | Also sent before a new GOTO while one is running, followed by a 100 ms pause |

## 3. Manual motion and rates

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Move N/S/E/W | `:Mn#` `:Ms#` `:Me#` `:Mw#` ∅ | S1-tested (via S2 `MoveTo`) | Uses the current slew rate |
| Stop one axis/direction | `:Qn#` `:Qs#` `:Qe#` `:Qw#` ∅ | S1-tested (via S2 `HaltMovement`) | |
| Slew rate | `:Rn#` ∅, n = 0..9 | S1-tested | 0.25×, 0.5×, 1×, 2×, 4×, 8×, 20×, 60×, 720×, 1440× sidereal (S1 labels; identical to S3) |

## 4. Tracking

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Tracking on | `:Te#` → `1`/`0` | S1 | |
| Tracking off | `:Td#` → `1`/`0` | S1 | |
| Tracking state | `:GTS#` → `1#` on / `0#` off | S1 | SH20-specific (S3 uses `:GAT#`). Polled only while not slewing, parking or homing |
| Sidereal / solar / lunar rate | `:TQ#` / `:TS#` / `:TL#` ∅ | S2-inherited | INDI also offers "Custom" → `:TM#`, which S3 does not document (Q6) |
| Get tracking rate | `:GT#` → `n#` | S1 | S3: 0 sidereal, 1 solar, 2 lunar, in ASCII. S1 uses the raw byte as an array index (N1) |

## 5. Guiding

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Pulse guide | `:MgnDDDD#` `:MgsDDDD#` `:MgeDDDD#` `:MgwDDDD#` ∅ | S2-inherited, enabled by S1 (`LX200_HAS_PULSE_GUIDING`) | DDDD in ms, zero-padded. S3 allows 0000–3000 |
| Set guide rate | `:Sgr0.nn#` ∅ (as S1 sends it) | S1 | Rate 0.10–0.90 × sidereal, `%.2f`. **SH20-specific**: AM5 uses `:Rg0.nn#`. S1 reads no reply (Q5) |
| Get guide rate | `:Ggr#` → `0.nn#` | S1 | Same rate for both axes |

## 6. Park and home

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Park | `:MP1#` ∅ | S1 | SH20-specific. S1 sets "parking" and reports parked once `:GU#` contains `N` (N6) |
| Unpark | `:MP0#` ∅ | S1 | S1 marks the mount unparked immediately |
| Set current position as park | `:SPc#` → `1`/`0` | S1 | Park position is stored in the mount |
| Go home | `:MH#` ∅ | S1 | Done when `:GU#` contains `H`. AM5 uses `:hC#` |
| Find home (sensor search) | `:MFH#` → `1`/`0` | S1 | S1 reports the action complete immediately on `1`. Whether the search runs asynchronously and how to detect its end is **unconfirmed** (Q7) |

## 7. Status

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Status string | `:GU#` → flags + `#` | S1-tested | Flags as in S3: `n` not tracking, `N` stopped (S1 treats it as "not moving"), `H` at home, `G` equatorial, `Z` alt-az, `L` low power, `S`/`s` RA/DEC stall, `T`/`t` ST4 guiding, followed by axis flags, rates and state. S1 uses `N`, `H`, `G` and `Z` only. There is no park flag (S3 v1.8 removed it), so park state cannot be read back (Q4) |
| Side of pier | `:Gm#` → `0#` east / `1#` west | S1-tested | **Differs from S3** (AM5 answers `E`/`W`/`N`). S1 sends it only in equatorial mode (N7) |

## 8. Mount geometry

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Equatorial mode | `:AP#` ∅ | S1-tested | The mount must be **rebooted** for the change to take effect (S1 warning, S3) |
| Alt-az mode | `:AA#` ∅ | S1-tested | Same reboot requirement |
| Read mode | `:GU#` flags `G` / `Z` | S1-tested | |

## 9. Site, date and time

The order S2 uses at connect and on update is: UTC offset, local time, local date. Location is latitude and
longitude only; elevation is not sent.

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Set longitude | `:SgsDDD*MM:SS#` → `1`/`0` | S1 | **East positive**, −180..+180 (S1 converts 0–360 E to ±180). This is not the Meade 0–360 W convention |
| Set latitude | `:StsDD*MM:SS#` → `1`/`0` | S1 | North positive |
| Get longitude | `:Gg#` → `sDDD*MM#` (or `…:SS#`) | S2-inherited | INDI treats the value as east positive (`range360`) |
| Get latitude | `:Gt#` → `sDD*MM#` | S2-inherited | |
| Set UTC offset | `:SGsHH:MM#` → `1`/`0` | S1 | S1 sends the **standard** sign (UTC+1 → `:SG+01:00#`). S3 documents the **Meade** sign (UTC+8 → `:SG-08#`). They conflict (N2, Q3) |
| Get UTC offset | `:GG#` → `sHH:MM#` | S2-inherited | INDI negates it (Meade convention), which is inconsistent with S1's set command (N2) |
| Set local time | `:SLHH:MM:SS#` → `1`/`0` | S2-inherited | 24-hour clock (S1 forces `LX200_24`, so `:Gc#` is not queried) |
| Set local date | `:SCMM/DD/YY#` → `1`/`0` | S1 | One-byte reply. S1 overrides the Meade version, which reads extra strings |
| Get local time / date | `:GL#` → `HH:MM:SS#`, `:GC#` → `MM/DD/YY#` | S2-inherited | Used at connect to read the mount clock |

## 10. Device options

| Function | Command → reply | Confidence | Notes |
| --- | --- | --- | --- |
| Set buzzer | `:SBzn#` ∅, n = 0 off, 1 low, 2 high | S1 | **SH20-specific**: AM5 uses `:SBun#` |
| Get buzzer | `:GBz#` → `n#` | S1 | S1 uses the raw byte as an array index (N1) |
| Set meridian behavior | `:STaFTsLL#` → `1`/`0` | S1 | F = 1 flip / 0 no flip; T = 1 keep tracking / 0 stop after the meridian; sLL = limit −15..+15° (e.g. `:STa11+05#`). Equatorial mode only. The S3 semantics apply: a negative limit stops before the meridian |
| Get meridian behavior | `:GTa#` → `FTsLL#` | S1 | 5 characters |

## 11. Commands S1 does not use

S1 deliberately limits the LX200 capabilities to pulse guiding. It sends no site-name, site-select, focuser,
alignment-mode query (`ACK`), tracking-frequency or Meade park (`:hP#`) commands. AM5 commands that S1 does not
use (`:GV#`, `:GAT#`, `:hC#`, `:Rg`, `:SBu`, `:GH#`/`:SH`, `:GS#`, `:GZ#`, `:GA#`, `:NSC#`, `:Rv`) may or may not
exist on the SH20. The INDIGO driver must not depend on them unless they are confirmed (Q2).

## Findings from the ToupTek ASCOM server and manual (S5, S6)

These facts come from ToupTek's own Windows software. They take precedence over the inferences above where the two differ.

| Topic | Finding | Evidence |
| --- | --- | --- |
| Identification | Connect sends a raw mode probe followed by `:GEM#`. A reply containing `F3` or `Updating` means the mount is in its bootloader. Then `:GVP#` → model string, and `:GV#` → firmware, of which the first 5 characters are used. Both must be non-empty, otherwise the connection fails with "COM port opened but mount did not reply to firmware query". The trailing `#` is stripped | S5 `Impl::Connect`, `Model_Mount`, `FwVer_Mount`, `GetEquipmentMode`, `GetStr` |
| `:GVP#` reply | The model string forms the label `<model>_VERSION`. S6 names the mount firmware field `SH20_VERSION`, so `:GVP#` answers `SH20#` (inferred; the literal reply is mount firmware, not visible in S5) | S5 `FwVer_Mount`; S6 §11 |
| Hand controller | `:GMV#` (HC firmware) and `:GMVP#` (HC model, `MHC`) are queried over Wi-Fi only | S5 `FwVer_MHC`, `Connect` |
| Serial link | 115200 8N1, 500 ms read/write timeouts, USB CDC ("USB Serial Device (COMx)") | S5 `ISerial::Open`; S6 §6.3.1 |
| Network link | TCP to the hand-controller AP `192.168.4.1`, **port 8080**, `NoDelay`, 3 s timeouts. Bridge mode uses a user-configured IP. BLE also exists (`SH20_xxxxxx`) | S5 `IWifi::Open`; S6 §6.3.2 |
| UTC offset | `:SGsHH:MM#` (and `:SGsHH#`) with the **standard** sign: UTC+1 → `:SG+01:00#`. This answers Q3 and confirms S1 | S5 `SetTimeZone`, `NegSymbol` |
| Park state | `:GPS#` → `0#` not parked, `1#` parking, `2#` parked, `e<n>#` error. The same encoding as ZWO `:Gps#`. This answers Q4 | S5 `GetParkStatus`, `IsPark` |
| Boolean replies | `1` or `1#` means success. Errors are `e1#`…`e13#` (S5 lists the same 13 codes as ZWO) | S5 `ParseRet`, `ParseErrCode` string table |
| `:GT#` | ASCII `0#`/`1#`/`2#`, mapped to 0/1/2 (as ASCOM drive rates, likely 0 sidereal, 1 lunar, 2 solar) | S5 `ParseTrackRate` |
| `:GBz#` | ASCII digit, parsed with `Convert.ToInt32`. Together with the `:GT#` row this answers Q8 and confirms N1 as an INDI defect | S5 `GetBeepMode` |
| Further commands present in S5 | `:GPS#`, `:SH#` (set home), `:Sh`/`:Gh` (altitude limits), `:SRl`/`:GRl#` (rate limit), `:So`/`:Go#`, `:SLD#`/`:SLE#`, `:GLC#`, `:GvM#`, `:HvM#`/`:NvM#`/`:LvM#`, `:NSC#`, `:Rv`, `:TC`, `:SMMC`/`:SMeq`/`:SMGE`, `:GMINF#`/`:SMINF`, `:FDR#`, `:GBTN#`/`:SBTN`/`:RBTFN#`/`:EBINF` (wireless info), `:GmV1#`, `:GP08#`, `:GS#`, `:GZ#`, `:GA#`, `:Gr#`, `:Gd#`, `:Update#`/`:Update01#`/`:Update02#` (firmware update; never to be sent by INDIGO) | S5 user-string heap. Semantics not yet decoded |

### Command semantics in TMount (S5)

This is how TMount sends each command and handles its reply. "Get" means TMount waits for a reply, up to 3 s for the first byte on serial and up to 1 s for a `#` on TCP. "Send" means TMount does not wait for a reply.

| Command | TMount usage | Reply handling |
| --- | --- | --- |
| `:MS#` | Get | **`0#` = GOTO accepted**, anything else is parsed as `e<n>#`. This contradicts S1, which treats `1` as success; the INDI PR did not list GOTO as tested. S5 is authoritative |
| `:CM#` | Get | Reply returned as is (S3: `N/A#` success, `e<n>#` error) |
| `:SrHH:MM:SS#`, `:SdsDD*MM:SS#` | Get | Reply returned as is (S3: `1`, or `e2#` for Dec) |
| `:StsDD*MM:SS.000#`, `:SgsDDD*MM:SS.000#` | Get | `1`/`1#` = OK. Latitude and longitude are signed, longitude **east positive** −180..180, with fractional seconds `.000` |
| `:Gt#`, `:Gg#` | Get | String. Longitude east positive |
| `:SGsHH:MM#` / `:SGsHH#` | Get | `1`/`1#` = OK. Standard sign |
| `:GG#` | Get | `sHH:MM`. A `+` is prepended when the sign is missing. Standard sign (matches the setter) |
| `:SCMM/DD/YY#`, `:SLHH:MM:SS#` | Get | `1`/`1#` = OK |
| `:Te#`, `:Td#` | Get | `1`/`1#` = OK |
| `:GTS#` | Get | `0#` off, `1#` on, else `e<n>#` |
| `:TQ#` `:TL#` `:TS#` | Send | Drive rate index 0 sidereal, 1 lunar, 2 solar |
| `:GT#` | Get | `0#` sidereal, `1#` lunar, `2#` solar (anything else 0) |
| `:Rn#` | Send | n is clamped to 0..9 |
| `:Me#` … `:Qe#` …, `:Q#` | Send | — |
| `:Mg[ewsn]DDDD#` | Send | Duration clamped to 0..3000 ms |
| `:Sgr0.nn#` | Send | Rate 0.10..0.90 with 2 decimals |
| `:Ggr#` | Get | `0.n#` (exactly 4 characters with `#`), default 0.5 on a bad reply |
| `:MH#` | Send | Go home (TMount labels both "Go Home" and "Find Home" with it) |
| `:MFH#` | Send | Home **search** (TMount "Search Home") |
| `:SPc#` | Get | Reply ignored |
| `:MP1#` | Send | Park. Completion: a thread polls `:GU#` until `N` |
| `:MP0#` | Send | Unpark |
| `:GPS#` | Get | `0#` not parked, `1#` parking, `2#` parked, `e<n>#` |
| `:GU#` | Get | `N` absent = slewing. `H` = at home. TMount also checks `n` |
| `:Gm#` | Get | Integer 0 = pier east, 1 = pier west, other = unknown |
| `:AP#`, `:AA#` | Get | Reply ignored. Mode readback is `:GU#` `G`/`Z` |
| `:SBzn#` | Get | Reply ignored. `:GBz#` → ASCII digit |
| `:STaFTsLL#` | Get | Limit sent as sign plus tens and units digits. `:GTa#` → `FTsLL#` |
| `:GEM#` | Get | Bootloader if the reply contains `F3` or `Updating` |
| `:GVP#`, `:GV#` | Get | Model; firmware (first 5 characters) |

## Notes on the INDI reference implementation

These are protocol ambiguities or likely defects in S1/S2. The INDIGO driver must not copy them.

- **N1** `:GT#` and `:GBz#`: S1 indexes a switch array with `response[0]`, the raw reply byte. With an ASCII reply
  (`0#`, as in S3) that index is 48 or more, past the end of the array. Either the SH20 replies with a binary
  byte 0..3, or S1 has an out-of-bounds write. The reply encoding is **unconfirmed**. The INDIGO driver will
  parse an ASCII digit and also accept a binary 0..3 byte until this is settled.
- **N2** UTC offset sign: S1 sets the offset with the standard sign, but the inherited S2 reader negates `:GG#` (Meade
  convention), and S3 (AM5) documents the Meade sign for `:SG`. At least one of these is wrong for the SH20.
  This needs confirmation on hardware: set a known offset, then compare `:GG#` with the mount clock against UTC.
- **N3** `:MS#`: S1 reads one byte and treats only `0` as failure. If the SH20 answers `e<n>#` as AM5 does, the
  error passes as success and the rest of the reply (`n#`) is flushed. The INDIGO driver should treat `1` as
  success, `0` as failure and `e<n>#` as failure with code n.
- **N4** `:Sr`/`:Sd`/`:Sg`/`:St`/`:SG`/`:SL`/`:SC`: S2 `setStandardProcedure` reads one byte and treats only `0` as
  failure, so an `e<n>#` error would also pass. The INDIGO driver will handle these the same way as N3.
- **N5** `:CM#`: the reply content is not checked by S2. The INDIGO driver should treat `e<n>#` as failure.
- **N6** Park completion: S1 sets parked as soon as `:GU#` contains `N`. If `N` is already present in the first poll
  after `:MP1#`, before motion starts, park completes immediately. There is no park flag in `:GU#`, so park state
  after reconnect cannot be read back.
- **N7** `:Gm#`: S1 maps `1` to west and `0` to east, against AM5's `E`/`W`/`N` letters. S1's author lists pier side
  display as tested, so the digits are trusted for the SH20. Whether `1` means "pointing west" or "pier west" in
  the INDI sense is taken from S1 as is.
- **N8** `:U#` toggles the precision. A toggle is unsafe without knowing the current state, and AM5 has no low precision, so the
  INDIGO driver will not send `:U#` and will reject a reply in low precision.
- **N9** `:Sgr`, `:SBz`, `:AP#`/`:AA#`, `:MP0#`/`:MP1#` and `:MH#`: S1 reads no reply. If the SH20 does answer them, the unread
  byte is flushed before the next command. The INDIGO driver will discard pending input before every
  transaction (`indigo_uni_discard()`), so it is safe either way. Whether they answer is still **unconfirmed** (Q5).

## Open questions

Each question needs ToupTek documentation, a hardware log or a hardware session.

- **Q1** *Answered (S5/S6):* 115200 8N1; TCP 192.168.4.1:8080 through the hand controller.
- **Q2** *Answered (S5):* `:GVP#` (model, inferred `SH20`) and `:GV#` (firmware). Still to confirm: the literal `:GVP#` reply on hardware.
- **Q3** *Answered (S5):* standard sign for `:SG`. Still to confirm: the `:GG#` reply sign.
- **Q4** *Answered (S5):* `:GPS#` 0/1/2.
- **Q5** *Answered (S5):* no reply to `:Sgr`, `:MP0#`, `:MP1#`, `:MH#`, `:MFH#`. A reply (content ignored) to `:SBz`, `:AP#`, `:AA#`, `:SPc#`, `:STa`.
- **Q6** Custom tracking rate (`:TM#`, AM5 `:Rv`?) and King rate support.
- **Q7** *Partly answered (S5):* `:MFH#` is sent without waiting for a reply. Completion is not visible in S5 (presumably `H` in `:GU#`).
- **Q8** *Answered (S5):* ASCII digits; `e1#`…`e13#`.
- **Q9** *Answered (S5):* TMount treats a missing `N` in `:GU#` as slewing.
