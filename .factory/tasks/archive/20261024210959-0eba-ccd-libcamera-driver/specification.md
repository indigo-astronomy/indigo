# Specification

Status: approved by owner request (2026-10-24)

## Problem and outcome
Provide `indigo_ccd_libcamera`, a generated INDIGO CCD driver that exposes every camera enumerated by libcamera as an INDIGO CCD device and delivers RAW sensor frames.

## In scope
Discovery at driver INIT, one device per camera named after the sensor model, RAW single exposures, streaming, abort, analogue gain, sensor modes as CCD_MODE, software subframe, Bayer pattern keyword, sensor info (size, pixel size), build without libcamera present (driver reports no cameras), fake-bridge tests and a hardware suite.

## Out of scope
ISP-processed output, hot-plug, cooling, `ccd_rpi` changes.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Each libcamera camera is attached as one CCD device named after its sensor model (upper case); duplicates get a ` #n` suffix. | fake-bridge test; hardware run |
| AC-002 | Connect opens and acquires the camera, publishes CCD_INFO (sensor size, pixel size, 16-bit) and CCD_MODE items per RAW sensor mode; disconnect releases it. Open failure reports ALERT and leaves nothing acquired. | fake-bridge test; hardware run |
| AC-003 | CCD_EXPOSURE delivers one RAW 16-bit frame of the selected mode/subframe with the requested exposure (verified from frame metadata) and gain; BAYERPAT is set for colour sensors. | fake-bridge test; hardware run |
| AC-004 | CCD_ABORT_EXPOSURE stops a running exposure or stream and settles properties; a new exposure works afterwards. | fake-bridge test; hardware run |
| AC-005 | CCD_STREAMING delivers the requested number of frames and finishes OK. | fake-bridge test; hardware run |
| AC-006 | Bridge failures (start failure, frame timeout, invalid frame) end the operation in ALERT without leaving the device BUSY. | fake-bridge test |
| AC-007 | Generator `sdk { usb = false; discover {} }` emits a USB-free discovery path; all existing driver definitions regenerate byte-identically. | generator comparison; driver build |
| AC-008 | The driver builds on Linux with and without libcamera development files. | build on RPi; stub build check |

## Failure behavior and edge cases
Exposure longer than the sensor maximum is clamped by CCD_EXPOSURE limits from the camera; frame timeout = exposure + readout margin; camera acquired by another process → connection ALERT.

## Permissions, data, and security requirements
No network, credentials or private data. Frame sizes from the SDK are validated against the allocated buffer.

## Compatibility and nonfunctional requirements
Linux only (libcamera). Built from `indigo_linux_drivers` by `Makefile.drvs`. Hardware-free tests also run on macOS through the fake bridge.

## Assumptions and open questions
libcamera raw-only stream configuration is accepted by the Raspberry Pi pipeline handler (verified on hardware).

## Owner decisions and requirement changes
2026-10-24: name `ccd_libcamera`, location `indigo_linux_drivers`, generator-based, independent of `ccd_rpi`, device names by sensor, non-interactive hardware test mode, generator extension approved (Peter Polakovic, chat).
