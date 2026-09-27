# WandererCover V4-EC refactoring record

This file was created on 2026-09-27 for the switch target work on branch `refactoring_targets`. The driver was
migrated to the generator and INDIGO 3.0 before this file existed; that migration is not recorded here. The
driver has a hardware-free suite, `indigo_test/integration/test_aux_wcv4ec_simulator.c`, against the host-side
simulator `aux_wcv4ec_simulator/aux_wcv4ec_simulator.c`. No hardware test was run for the changes below.

## Cover request during an autodetection and bounded detection reply (TGT-015, TGT-D04, 3.0.0.10, 2026-09-26)

Findings TGT-015, TGT-B05 (aux_wcv4ec part) and TGT-D04 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, reproduced before the fix.

- **Defects (reproduced):** AUX_DETECT_OPEN_CLOSE sets `operation_running` without making AUX_COVER BUSY, so a cover request can be queued while the detection runs. The 1 s status timer, already due when the detection ends, ran ahead of that handler, wrote CLOSE/OPEN into AUX_COVER, published OK over the pending BUSY and cleared `operation_running`; the handler then sent the side the timer wrote. The DETECT refusal published AUX_SET_OPEN_CLOSE instead of AUX_DETECT_OPEN_CLOSE, and the OpenSet / CloseSet read loop had no bound.
- **Fix:** the timer still ends a detection but leaves value and state of an AUX_COVER that is BUSY before its handler started a move alone; the cover handler sends the request read with `indigo_get_switch_target()` and applies it with `indigo_apply_switch_targets()`. The DETECT refusal publishes AUX_DETECT_OPEN_CLOSE with ALERT and the trigger released; the reply is read with `indigo_uni_read_section2()` (5 s first byte, 1 s next bytes) for at most ten lines.
- **Regression tests:** `cover_request_survives_detection_end`, `detection_refused_while_the_cover_moves`, `lost_detection_reply_does_not_block_the_queue` (simulator `--detect-reply-delay`, `--lose-detect-reply`); all three failed against 3.0.0.9 and pass with 3.0.0.10.
- **Verification (Linux x64):** `python3 tools/run_driver_test.py aux_wcv4ec` 18/18 OK.
- **Observed, not changed:** the detect handler discards the input after sending the detect command, and `wcv4ec_read_status` reads lines without an inter-byte timeout (TGT-D10). The call order was kept until a test could show the effect, see below.

## Detection reply discarded and status read without an inter-byte timeout (TGT-D10, 3.0.0.11, 2026-09-27)

Finding TGT-D10 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, both parts reproduced before the fix.

- **Defect 1 (reproduced):** the detect handler called `indigo_uni_discard()` after `wcv4ec_command()` had written 100001 / 100000. The box answers with OpenSet / CloseSet as soon as it has taught the angle, so a reply already in the input when the handler reached the discard (a fast box, or a host preempted after the write) was thrown away with the stale status frames. The handler then read ten status frames and ended the detection with ALERT after 10 s although the box had taught the angle. The simulator already answers at once; the test holds the driver thread for 0.3 s in the debug log of the command write, right after the write and before the discard, so the reply is waiting. Against 3.0.0.10 the detection ended with ALERT after 10.0 s.
- **Defect 2 (reproduced):** `wcv4ec_read_status` read lines with `indigo_uni_read_line()`, which has a 5 s first-byte timeout and no timeout for the next bytes. On Linux and macOS each `read()` is bounded only by the 5 s `VTIME` of the port, so a line that trickles a byte every few seconds keeps the status read, and with it the device queue, busy until its newline. On Windows the port is opened without `COMMTIMEOUTS`, so a line that stops mid-way can block the read with no bound at all; that part follows from the source and was not reproduced (no Windows run). With the simulator's `--stall-frame-at 5000`, which sends 25 bytes of one status frame, then 6 bytes 2 s apart and then the rest, a heater request sent 2 s into the stall was served after 11.0 s against 3.0.0.10: the status read ran for about 12 s, past its 5 s timeout. The partial line would also have been parsed if it had been cut late enough, because the reader did not check that the line ended.
- **Fix:** the detect handler discards stale input before it sends the command instead of after it, so everything the box sends after the command is read by the bounded OpenSet / CloseSet loop. Status lines are read by the new `wcv4ec_read_line()`, which uses `indigo_uni_read_section2()` with 5 s for the first byte and 1 s for the next bytes and accepts only a line ended by `\n` (an unterminated fragment returns -1, so `wcv4ec_read_status` fails and the next frame is read on the next timer tick). The box sends a frame of about 50 bytes in one go at 19200 Bd, so a second between two bytes is not a frame in progress. The discard at the start of `wcv4ec_read_status` and the order of the reads are unchanged.
- **Regression tests:** `fast_detection_reply_is_not_discarded` (the detection must end OK with the angle taught; 3.0.0.10: ALERT after 10.0 s; 3.0.0.11: OK after 0.3 s, 3/3) and `stalled_status_frame_does_not_hold_the_queue` (a heater request sent during the stalled frame must be served within 7 s and the frames after the stall must be read again; 3.0.0.10: not served within 6 s, served after 11.0 s with a longer wait; 3.0.0.11: served after 1.0 to 4.0 s, 3/3).
- **Verification (Linux x64):** `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_wcv4ec` 20/20 OK (`MIGRATION_STATUS.md` 18 / 0 -> 20 / 0). Regeneration with the unchanged generator reproduces the checked-in output; the only generated changes are the edited code and handler blocks and the version.
- **Observed, not changed:** the first-byte timeout of each status read is still 5 s and `wcv4ec_read_status` may read up to three lines, so one timer tick can still hold the queue for several seconds when the box goes silent; the box sends a frame every second, so this only happens when it has stopped. The detect handler still reads the reply with `indigo_uni_read_section2()` and prefix matching, so an unterminated "OpenSet" fragment would count as a confirmation.

```sh
cd indigo_test && INDIGO_TEST_CASE_FILTER=fast_detection_reply_is_not_discarded ./build/integration/test_aux_wcv4ec_simulator
cd indigo_test && INDIGO_TEST_CASE_FILTER=stalled_status_frame_does_not_hold_the_queue ./build/integration/test_aux_wcv4ec_simulator
```

Final test summary: 20 simulated tests run, 20 passed; 0 hardware tests run, 0 passed.
