# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `indigo_test/integration/test_mount_lx200_simulator.c` `lx200_touptek_goto_sync_and_refusals` | host simulator `--model touptek`; replies `0#`, `e5#` and `0#` to `:Sd` are injected | `1#` goto OK without "Slew failed"; `0#` ALERT "rejected"; `e5#` ALERT with the code; refused target sends no `:MS#`; sync `1#` OK, `e9#` ALERT | before and after the fix |
| T-002 | AC-002, AC-003, AC-004 | `lx200_touptek_rates_buzzer_and_meridian` | simulator; injected `0#` to `:STa10+00#` | `:Sgr0.7#` sent, `:Sgr0.70#` never; no limit property; `:STa01+00#`, `:STa11+00#`; rejected `:STa` ALERT with the read-back state; settings survive a reconnect | before and after |
| T-003 | AC-003, AC-004, AC-006 | `lx200_touptek_connect_publishes_the_device_state` | `--touptek-parked` | `AUTO_FLIP` and `RESUME_TRACKING` read from `:GTa#`; `MOUNT_HOME` count 2; no `:MFH#` | before and after |
| T-004 | AC-005 | `lx200_touptek_site_and_time_signs` | simulator; site with a fraction is rejected (`0#`) | `:St+48*30:00#`, `:Sg+017*15:00#`, `:St-33*45:00#`, `:Sg-020*00:00#` | before and after |
| T-005 | AC-006, AC-007 | `lx200_touptek_home`, `lx200_home_touptek` | simulator `:GHS#`; injected `0#`; `:MH#` dropped | completion at GHS 2; GHS 0 shows AWAY although GU has H; abort ends AWAY; a home that never starts ends ALERT | before and after |
| T-006 | AC-008 | `lx200_touptek_park_unpark_and_park_position` | simulator; `:MP0#` dropped once | unconfirmed unpark ALERT "not confirmed", still parked; then unpark OK with `:GPS#` read before and after `:MP0#` | before and after |
| T-007 | AC-010 | full `test_mount_lx200_simulator` | all profiles | all pass | after |

## Baseline evidence
See verification.md, Executed checks: 13/13 SH20 cases on the original driver with the original tests.

## Bug reproduction
The new tests were run with the original driver source (HEAD `indigo_mount_lx200.c`) compiled into the test binary. Six cases fail at the checks of D1, D2, D3/D4, D5, D7 and D8 (verification.md). D6 has no separate assertion in that run because the connect case already failed on D3/D4. In the final run its checks (count 2, no `:MFH#`) pass on the fixed driver.
