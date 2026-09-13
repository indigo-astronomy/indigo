# Vixen StarBook Protocol — Research Summary

## Overview

The Vixen StarBook is a hand-controller / mount-control unit used with several
Vixen SX-series (and later AXD) telescope mounts (StarBook, StarBook S,
StarBook TEN). It exposes control functionality over a network connection
via a **built-in, minimal HTTP server** running on the unit itself.

There is **no official protocol specification published by Vixen**. Everything
known about the protocol comes from community reverse-engineering efforts,
embedded in the source code of several open-source drivers/clients:

- **matlab-starbook** (E. Farhi) — https://github.com/farhi/matlab-starbook
- **indi-starbook** (not7cd) — https://github.com/not7cd/indi-starbook
- An ASCOM driver for StarBook (topic page: https://github.com/topics/starbooks)
- **indi-starbook-ten** — separate INDI driver specifically targeting the
  StarBook TEN unit, communicating over HTTP as well

## Transport / Connection

- The StarBook unit is connected to a computer/network either:
  - directly, via an Ethernet **crossover cable**, or
  - through a standard router (in which case a **static IP** should be
    assigned to the StarBook)
- On boot, the StarBook enters an **INIT** state. The unit's IP address can be
  read from the on-device menu: `About STAR BOOK`.
- Once the IP is known, the device's internal HTTP server can be reached
  directly with a browser (`http://<starbook-ip>/`) or by any HTTP client.
- Communication is done via simple **HTTP GET requests** — no authentication,
  no POST body, no encryption.

## StarBook as a State Machine

The StarBook firmware behaves like a small **state machine**, and not all
commands are valid in all states:

- **INIT** state: initial boot state (date/location/alignment setup via the
  device's own menu). In this state, the unit will **not** respond to
  `GOTORADEC`-type commands.
- **SCOPE** state: the operational state, entered either manually through the
  on-device menu, or remotely by issuing the `START` command
  (`http://<ip>/START`). Once in SCOPE state, goto/tracking/move commands are
  accepted.

Reported reliability caveat (from the `indi-starbook` driver README): the
built-in HTTP server is described as **"very fragile"** — malformed or
unexpected requests can hang the server, sometimes requiring a manual restart
of the StarBook unit.

## Known Commands / Endpoints

Commands are issued as HTTP GET requests to endpoints on the StarBook's IP
address, generally in the form:

```
http://<starbook-ip>/<COMMAND>[?param1=value1&param2=value2...]
```

The response is a compact text/key-value formatted string that client
software parses.

| Command | Purpose |
|---|---|
| `START` | Switches the StarBook from INIT into SCOPE (operational) mode |
| `GETSTATUS` | Returns current mount status: RA/DEC position, mount state (slewing / tracking / stopped), etc. |
| `GOTORADEC?RA=...&DEC=...` | Slews the mount to the given Right Ascension / Declination |
| `MOVE?...` (direction parameters, e.g. N/S/E/W) | Manual directional move — same effect as pressing the physical arrow buttons |
| `STOP` | Aborts an ongoing move or goto |
| `ALIGN` | Syncs/aligns the mount's current position to a given RA/DEC (used for pointing correction) |
| `HOME` | Sends the mount to its HOME reference position |
| `SETSPEED?speed=0-8` | Sets slew/move speed; 0 = stop, 8 = fastest |
| `GETSPEED` | Reads back the current speed setting |
| `GETXY` | Returns raw motor encoder/coder values |
| `GETSCREEN` | Returns the StarBook's own display content as an image (only applicable to units with a 320×240 screen) |
| `HELP` | Opens/returns the device's help page |

Additional higher-level convenience operations exposed by client libraries
(e.g. `matlab-starbook`), which are built on top of the above primitives
rather than being separate raw commands:

- `update` — refresh both status and screen image
- `zoom` — get/set display zoom level (0–8, or `'in'`/`'out'`)
- `date` — get StarBook date/time
- `web` — open the current target on sky-map.org (client-side convenience, not a device command)
- `grid` — prepare a grid pattern around a target (used for pointing-model style calibration)

## StarBook vs. StarBook TEN

- The original **StarBook** (first offered ~2003) and **StarBook S** share the
  command set above and are the primary target of most open-source drivers
  (`indi-starbook`, `matlab-starbook`).
- **StarBook TEN** is a later, more advanced controller (touchscreen,
  planetarium display, LAN port) and exposes additional commands not present
  on the original StarBook. Existing open-source drivers designed for the
  original StarBook (e.g. `indi-starbook`) explicitly note that StarBook TEN
  is only **partially** compatible — commands exclusive to StarBook TEN are
  not implemented/available in those drivers.
- `indi-starbook-ten` is a separate, dedicated driver targeting the TEN
  variant specifically, also communicating via HTTP.

## Practical Notes for Implementation

- Because the on-device HTTP server is fragile, a robust client should:
  - avoid rapid-fire/back-to-back requests,
  - handle timeouts/connection resets gracefully,
  - be prepared to detect a "hung" device and prompt for a manual restart.
- The mount must first be aligned/configured (date, location, initial
  alignment) via the StarBook's own on-device menu before remote `GOTORADEC`
  commands will produce meaningful results.
- No authentication exists — anyone with network access to the StarBook's IP
  can issue commands. Treat it as a trusted-network-only device.

## Primary Sources Consulted

1. https://github.com/farhi/matlab-starbook — clearest plain-language listing of client-side methods mapped to StarBook operations
2. https://github.com/not7cd/indi-starbook — INDI driver; `command_interface.cpp` / `command_interface.h` contain the actual HTTP request construction and response parsing (recommended next step: read these files directly for exact request/response formats)
3. https://www.mathworks.com/matlabcentral/fileexchange/65944-vixen-starbook-control — MATLAB File Exchange listing, mirrors the matlab-starbook README
4. https://packages.debian.org — package descriptions for `indi-starbook` / `indi-starbook-ten` (confirms HTTP-based communication, dependency info)
5. Cloudy Nights forum discussion — general hardware/feature context distinguishing StarBook vs. StarBook TEN

## Suggested Next Step

For exact wire-format details (precise query parameter names, exact response
string format for `GETSTATUS`, error handling, etc.), the most reliable
source is reading the actual driver source code rather than secondary
descriptions:

- `command_interface.cpp` / `command_interface.h` in the `indi-starbook`
  repository
- `starbook_types.cpp` / `starbook_types.h` in the same repository (likely
  defines the state machine / response parsing types)

These were not fetched in full during this research pass and would be the
next logical step if a byte-exact protocol reference is needed (e.g. for
implementing a new INDIGO driver).
