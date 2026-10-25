# Task brief

- Task ID: 20261024210959-0eba-ccd-libcamera-driver
- Owner: Peter Polakovic
- Created: 2026-10-24

## Problem and desired outcome
INDIGO has only the optional, hand-written C++ `ccd_rpi` driver for cameras handled by libcamera. The owner asked for a new, modern, generator-based CCD driver `ccd_libcamera` in `indigo_linux_drivers/`, independent of `ccd_rpi`, with devices named after the camera sensor, tested on the connected Raspberry Pi (`ssh -A rpi.local`).

## Users and use cases
Astrophotographers using Raspberry Pi (or other libcamera-supported Linux boards) with CSI cameras as imaging or guiding cameras: single RAW exposures, streaming, gain, sensor modes (binning), subframes, multiple cameras.

## Scope and non-goals
In scope: generator input and generated driver, a C bridge over libcamera (C++), build integration, a fake bridge and hardware-free integration tests, a hardware test suite, documentation, status files, a generator extension for non-USB SDK discovery (owner-approved 2026-10-24). Non-goals: changing `ccd_rpi`, ISP-processed (RGB/YUV) output, hot-plug of CSI cameras, sensor cooling.

## Constraints and references
Root/driver/test AGENTS.md rules, DRIVER_GENERATOR_MIGRATION.md, DRIVER_TESTING_RULES.md. The RPi checkout `~/indigo` is shared with another session (CCD RPI kamera); the owner asked to wait for its commit and then work there on an own branch.

## Initial questions
Test mode: non-interactive hardware run (owner answer, 2026-10-24). Generator change: approved (owner answer, 2026-10-24).
