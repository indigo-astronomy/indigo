# Lunatico focuser refactoring and validation record

Status: first automated coverage added on 2026-09-18. The driver is still on the 2️⃣ API and has not
been migrated to the generator, so this record covers the new simulator suite and the defects it
exposed, not a completed refactoring.

## Current-state audit

- `indigo_focuser_lunatico.c` is a thin entry point: it sets `DEFAULT_DEVICE = TYPE_FOCUSER` and
  includes `shared/lunatico_shared.c`, which `indigo_rotator_lunatico.c` also includes. The Main
  port therefore comes up as a focuser here and as a rotator in the other driver; everything else is
  common code.
- Until 2026-09-18 the driver had no automated tests at all. `integration/test_focuser_lunatico_simulator.c`
  now drives it over a real PTY against the standalone seletek/step simulator that already existed
  for `rotator_lunatico` (`rotator_lunatico_simulator`), which speaks the same protocol.
- Manufacturer protocol evidence was not re-obtained for this pass; the simulator's command set is
  taken as-is from the existing `rotator_lunatico` work.

## Scenario-to-test mapping

All scenarios are in `indigo_test/integration/test_focuser_lunatico_simulator.c`; all 11 pass.

| Scenario | Test case |
| --- | --- |
| Default device type and focuser class property completeness | `main_port_defaults_to_focuser` |
| Absolute goto, progress and completion on the Main port | `main_absolute_move` |
| The same on a secondary port sharing the Main connection | `exp_absolute_move` |
| Relative steps in both directions via `FOCUSER_DIRECTION` | `relative_steps_follow_direction` |
| Abort leaves the focuser short of the target | `abort_motion` |
| A refused goto is reported as `INDIGO_ALERT_STATE` | `goto_failure` |
| A corrupt position readback is reported | `read_failure` |
| Connection refused on a wrong model reply | `wrong_model` |
| Connection refused on no reply | `silent` |
| Connection refused on an oversized reply | `oversized` |
| Disconnect followed by reconnect | `reconnect` |

Only the Main port device opens the serial handle: `PRIVATE_DATA->count_open` gates the open and
each port device opens through its own `DEVICE_PORT`, which defaults to `auto://`. A secondary port
can therefore only share a connection that the Main device established first, and every device has
to be disconnected before `INDIGO_DRIVER_SHUTDOWN` is accepted. The test's `start()`/`stop()`
helpers encode that ordering.

## Defects found and fixed

- Stack buffer overflow in `lunatico_command()` (`shared/lunatico_shared.c`), reported by ASan as
  `stack-buffer-overflow` at the `response[index] = '\0'` store. The read loop filled up to `max`
  bytes and then wrote the terminating NUL at `response[max]`, one past the end of the caller's
  `char response[LUNATICO_CMD_LEN]`. Its UDP branch made this worse by reading a full
  `LUNATICO_CMD_LEN` regardless of `max`. The three call sites now pass `sizeof(response) - 1` and
  the UDP read is bounded by `max`. Reproduced by the simulator's `oversized` profile, which sends
  100 bytes with no `#` terminator; that scenario is clean under ASan after the fix.
- A refused goto left the property BUSY forever. In the `FOCUSER_POSITION` goto branch no failure
  state was set at all, and in the rotator goto and focuser relative-steps branches the
  `INDIGO_ALERT_STATE` assignment was immediately overwritten by the 0.5 s poll scheduled right
  after it, so all three assignments were dead code. All three now publish ALERT and skip the poll
  on failure, matching the adjacent sync branches. This also fixes the previously failing
  `goto_failure` scenario of `test_rotator_lunatico_simulator.c`.

Both drivers that include the shared source had their `DRIVER_VERSION` incremented
(`focuser_lunatico` to `0x02000009`, `rotator_lunatico` to `0x02000008`).

## Deferred work

- Use-after-free on port reconfiguration. Changing `LUNATICO_PORT_*_CONFIG` runs
  `delete_port_device()` / `create_port_device()`, and a later `indigo_init_number_property()` reuses
  memory that `indigo_focuser_detach()` already freed. ASan reports `heap-use-after-free` at
  `indigo_bus.c:1139`. Reproduce with the `exp_rotator`, `third_rotator`, `exp_focuser` and
  `third_powerbox` scenarios of `test_rotator_lunatico_simulator.c`. Fixing this is a port-device
  lifecycle rework and belongs with the generator migration, so no scenario in the focuser suite
  reconfigures a port.
- `test_rotator_lunatico_simulator.c` has a build rule but is not listed in `INTEGRATION_TESTS`, so
  `make -C indigo_test test` never runs it. It currently passes 7 of 11 scenarios; the four failures
  are the port-reconfiguration cases above. It should be wired in once that defect is fixed.
- `Attempt to set timer with non-NULL reference` is still logged on some focuser paths; the timer
  reference is not cleared before being reused. Not investigated in this pass.
- Temperature compensation, backlash, software limits, `AUX_POWERBOX` port mode, the UDP transport
  and multi-instance behaviour are not covered.
- Hardware testing was not performed. No hardware validation is claimed.

## Final test summary

- Simulated tests: 11 executed, 11 passed.
- Hardware tests: 0 executed, 0 passed.
