# Task brief

- Task ID: 20261107080809-a02b-fix-ccd-svb-gain-after-open
- Owner: Peter Polakovic
- Created: 2026-11-07

## Problem and desired outcome
A user running INDIGO 3.0-9 on a Raspberry Pi 4 (Debian 12 arm64, SVB SDK v1.13.4, `indigo_ccd_svb` 3.0.0.34)
reported that `CCD_GAIN` cannot be set on an SVBONY SV705C (IMX585, range 0-650) nor on an SV905C2 (range 0-100):
every request (A1, `indigo_prop_tool set "SVBONY SV705C.CCD_GAIN.GAIN=252"`) leaves the property in ALERT at 0,
after a fresh connect and after a service restart, over USB 3 and USB 2, while `CCD_OFFSET` is accepted. With the
server at `-vv` nothing is logged for the gain or the offset request. On 3.0-7 the gain works on the same cameras.
The same user found that after the upgrade from 3.0-7 the Imager Agent refused the SV705C until the saved
configuration (`~/.indigo/SVBONY_SV705C.config`) was removed.

The assessment of the report (chat, 2026-11-07) found the cause in the driver: the hand-written 3.0-7 driver wrote
`SVB_EXPOSURE` = 1 s right after opening the camera with the comment "fix for SDK gain error issue", and that
workaround was dropped by the migration of the driver to the code generator in 3.0-8. The SVB SDK refuses a gain
write after `SVBOpenCamera()` until the exposure (or the window) has been written once; the INDI driver carries the
same workaround with the same comment and an independent .NET binding of SDK v1.13.4 documents the refusal as
verified on hardware. The regenerated `svb_set_number()` additionally swallows the SDK error codes, which is why
nothing reaches the log. The refused saved `CCD_GAIN` turns the configuration restore into ALERT, and since 3.0-8 an
agent's device selection fails on a configuration restore in ALERT ("Device configuration restore failed"), which is
the second symptom.

Desired outcome: a gain request is accepted right after connecting the camera, before any exposure; a refused SDK
write is logged with its error code; the fake SDK models the refusal so the hardware-free suite reproduces the
defect and pins the fix; the hardware suite checks a gain write before the first exposure.

## Users and use cases
Every user of an SVBONY camera through `indigo_ccd_svb` who sets the gain before the first frame of a session,
and every user whose saved configuration holds a gain (the Imager Agent refuses the camera otherwise).

## Scope and non-goals
In scope: the driver's connect sequence (`.driver` source and regenerated outputs), the logging of
`svb_set_number()`, the driver version, the fake SDK and the integration suite in
`indigo_test/integration/test_ccd_svb_sdk.c`, the hardware suite's first gain write, `MIGRATION_STATUS.md` test
count, the recorded run. Non-goals: the empty `indigo.log` during a session reported by the same user (not
reproducible from this repository; depends on how the server is started on the Pi), the 3.0-7 short-exposure hang
over USB 3 (restructured acquisition in 3.0-8/3.0-9), the agent's selection policy, the generator, hardware
validation (no SVBONY camera is available in this session).

## Constraints and references
Root AGENTS.md driver, generated-driver and formatting rules; `indigo_drivers/AGENTS.md`; `indigo_test/AGENTS.md`
(fake SDKs model the real SDK, quirks selectable per device); `indigo_test/DRIVER_TESTING_RULES.md`; driver source
`indigo_drivers/ccd_svb/indigo_ccd_svb.driver`; the 3.0-7 driver at commit cd408e64b
(`handle_ccd_connect_property()`, "fix for SDK gain error issue"); INDI `indi-svbony/svbony_base.cpp` (same
workaround); https://www.nuget.org/packages/SVBony.SDK ("a gain written before the first window is set after an
open is refused with SVB_ERROR_GENERAL_ERROR", SDK v1.13.4); the user's report (quoted in the chat of 2026-11-07).

## Initial questions
None blocking. Whether an exposure write of the value already held by the SDK also unlocks the gain is unknown;
the task writes 1 s as the proven 3.0-7/INDI workaround does.
