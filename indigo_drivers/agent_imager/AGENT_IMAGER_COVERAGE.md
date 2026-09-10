# Imager Agent integration coverage

The suite executes the production Imager Agent through the public in-process bus. It uses the real CCD simulator's camera, wheel, focuser and Bahtinov camera. Test peers implement the external shutter and guider/mount/solver property contracts. Camera fault adapters inject bounded BUSY-to-ALERT transitions and an invalid RAW signature; they do not replace the agent's algorithms.

Every named case runs in a fresh child process with a watchdog and a unique temporary configuration/image directory. The parent checks both assertions and cleanup, reports signals as failures, and removes its temporary files even if a child crashes. No physical hardware, server, network connection or vendor SDK is used. Static framework linking ensures that configuration calls originating inside the framework also use the isolated directory on macOS. The related-agent validation fix admits Imager Agent instances; no driver version or generator output is changed.

## Run

From the repository root after building the library and CCD simulator:

```sh
make -C indigo_test test-agent-imager
```

The executable is also included in `test-integration` and `test`. A substring selects individual cases; an unmatched filter returns failure:

```sh
indigo_test/build/integration/test_agent_imager 'breakpoint POST_BATCH'
```

`INDIGO_TEST_TRACE=1` prints switch requests for diagnosis. The complete suite currently exits nonzero for the production regressions listed in [the driver review notes](../REVIEW.md#current-findings). Assertions deliberately require the correct behavior; known failures are not converted into passing expectations or omitted from the default target.

AddressSanitizer (Clang; agent, filter, configuration framework object and harness instrumented, remaining library/simulator archives uninstrumented):

```sh
make -C indigo_test TEST_BUILD=build/asan IMAGER_TEST_FLAGS='-fsanitize=address -fno-omit-frame-pointer' build/asan/integration/test_agent_imager
indigo_test/build/asan/integration/test_agent_imager 'independent instances'
```

Clang source coverage:

```sh
make -C indigo_test TEST_BUILD=build/coverage IMAGER_TEST_FLAGS='-fprofile-instr-generate -fcoverage-mapping' build/coverage/integration/test_agent_imager
LLVM_PROFILE_FILE=/tmp/imager-%p.profraw indigo_test/build/coverage/integration/test_agent_imager
llvm-profdata merge -sparse /tmp/imager-*.profraw -o /tmp/imager.profdata
llvm-cov report indigo_test/build/coverage/integration/test_agent_imager -instr-profile=/tmp/imager.profdata indigo_drivers/agent_imager/indigo_agent_imager.c
```

On macOS use `xcrun llvm-profdata`, `xcrun llvm-cov` and add `-arch=arm64` (or the executed architecture) to `llvm-cov` for the universal executable. Use a fresh profile prefix for a new measurement. Clean test build artifacts afterward with `make -C indigo_test test-clean`.

## Scenario mapping

| Feature / contract | Named cases and assertions |
| --- | --- |
| Attach, enumerate, missing camera/focuser, idle abort/pause | `metadata and missing devices`: all 23 agent-specific properties, missing-device terminal states and switch reset |
| Settings, legacy focus aliases, dynamic multistar properties, all four estimator choices, defaults | `settings selection estimator and reset`; `default preview estimator` |
| Configuration persistence | `configuration reload`: batch, focus and failure policy survive shutdown/init in the isolated directory |
| Single capture, single preview, finite exposure batch and stream | `finite capture preview batch and streaming`: exact delivered image counts and batch frame statistic |
| Repeated preview, finite/indefinite batch/stream abort | `abort all capture modes and reacquire`: four process paths, finite and indefinite counts, no late images and subsequent successful preview |
| Exposure retry and exhaustion | `exposure failure retry exhaustion and recovery`: two failures then success; three failures must fail the batch |
| Stream failure | `streaming failure propagates`: camera BUSY-to-ALERT must fail the agent process |
| Invalid image | `invalid image detection and recovery`: malformed RAW signature rejected by plain capture and star finding; later valid image succeeds |
| Plain capture / external shutter routing | `external shutter routing`: shutter exposure used during preview; process abort must reach shutter and publish completion |
| Pause and busy guards | `pause resume and busy guards`: immediate abort-and-pause, wait-for-frame pause, resume, abort while paused, competing start rejected |
| All six breakpoints | Six independent `breakpoint ... resume and abort` cases: no new request while suspended, manual resume, abort and recovery. Delay and post-batch defects are exposed independently |
| Delay between images | `interframe delay abort`: waits for the public WAITING phase, aborts with exactly one image delivered, then reacquires |
| Dithering | `dither cadence failure and abort`; `dither disabled dark and missing guider`: exact trigger cadence, skip count, last-frame option, failure policy, abort during settling, dark/disabled suppression and missing guider |
| Mount and solver coordination | `mount transit and solver coordination`: transit pause, resume, mount abort permission changes and solver image-solving disable |
| Multiple agents / camera isolation | `independent instances and reselection`: two cameras run independently, abort one, remove additional instance, reselect camera, shutdown |
| Barrier synchronization | `additional instances and barrier`: related-agent admission, breakpoint/trigger propagation, synchronized batch, abort propagation, self-exclusion and one-way relation. Admission was fixed and the full barrier case passes; explicit instance removal is covered separately |
| Star discovery, selection, HFD and format restore | `stars statistics and format restoration`: real generated star image, detected star selection, measured HFD, FITS restored after RAW processing, clear selection |
| Include region, subframe setting and binning | `selection regions binning and subframe`: include rectangle, discovery, subframe setting, symmetric binning and valid selected coordinates |
| Autofocus | `autofocus estimators`: U-curve, HFD/peak and RMS start 60 steps out of focus and must finish within 20 steps of the simulator optimum; `bahtinov preview and focus`: mask error and successful Bahtinov focusing |
| Focus abort / failure / repeat | `autofocus abort and failure`; `autofocus failure policies and repeat`: abort, exhausted capture retries, STOP/RESTORE selection, repeated attempt recovery |
| Focus bracketing | `bracketing returns to start`: exact batch frame count and return to original position on completion and abort |
| Wheel and focus controls | `wheel offsets and manual focus`: names/offsets, forward/backward differential offset motion, wheel slot readback and manual outward motion/stop |
| Local image storage and headers | `local batch and fits headers`: camera writes two FITS files, agent lists/downloads them, real FITS header contains FILTER/FOCUSPOS/FOCTEMP |
| File management and disk usage | `download listing payload delete`: exact binary payload, unavailable download, delete and repeated delete, positive disk capacity |
| Device loss and recovery | `camera disconnect and recover`: process must leave BUSY after selected camera disconnects and permit reuse |
| Lifecycle with active process | `shutdown while paused`: paused process exits during shutdown and fresh agent initializes; all other cases exercise cleanup too |

## Scope of the measurement

Function/line/branch coverage measures execution, not proof that every combination is correct. Some regression assertions fail before later recovery checks in their case. Barrier synchronization is verified after fixing related-agent admission; instance-removal cleanup remains a separate failing regression.

There is no exposed sequence executor in this checkout's Imager Agent: the source contains unused sequence fields/constants but no sequence property dispatch or process. External scripting/sequencing belongs to a separate agent and is outside this suite. Image analysis uses simulator-generated mono/Bahtinov images; exhaustive RAW pixel-format and transport combinations are covered separately by image-library/CCD suites. Remote BLOB URL downloads, real guiding/meridian motion, hardware timing and Linux/Windows execution are not validated here. The test peers exercise the Imager Agent's side of those property contracts.

## Recorded validation

On macOS arm64, against the working tree based on `a57057656` plus the related-agent admission fix, the full suite and focused final reruns executed all 69 production-agent functions. Clang measured 78.39% of lines, 82.44% of regions and 66.51% of branches. The uncovered branches include platform/error alternatives and combinations not exercised by these scenarios; this is not a claim of 100% branch coverage. ASan separately confirmed the instance-removal double-free. The fixed barrier scenario passes in both normal and coverage builds.

Final per-case results are 26 passing and 9 failing out of 35, combining the full normal run (25/35 before the admission fix) with the successful fixed barrier rerun. Nine failures correspond to eight open production defects; the two delay-breakpoint cases share a cause. The standard target intentionally returns failure while these regressions remain unresolved.

## DRV-111 follow-up

The original independent-instances/reselection case now passes normally and with ASan after clearing the freed additional-client slot. The suite has 36 cases with the new `additional instance lifecycle` scenario: repeated creation, partial removal, complete removal, recreation and shutdown with two instances still attached. All three instance scenarios pass with ASan. Normal testing exposed an intermittent watchdog timeout in the new lifecycle scenario (DRV-120); a traced rerun passed. The earlier full-suite totals and coverage measurements above are historical and have not been rerun for this change.

## DRV-112 follow-up

The retry-exhaustion regression failed before the fix and passes afterward. Coverage now includes third-attempt success, termination after three failures for batch counts 1, 2 and -1, request/image counts and recovery after each failed batch. Finite acquisitions and abort/reacquire also pass. These are three focused case results; previous full-suite totals and coverage percentages are unchanged historical measurements.

## DRV-113 follow-up

The streaming-failure regression failed before the fix and passes afterward for counts 1, 3 and -1, checking terminal ALERT, cleared start switch, image counts and successful recovery. Finite capture, abort/reacquire and pause/resume/BUSY guards also pass (four focused cases). Previous full-suite totals and coverage measurements remain historical.

## DRV-114 follow-up

The expanded external-shutter routing case failed before the fix and passes afterward for preview and exposure batch. It checks camera/shutter abort request counts, shutter terminal state and switch reset, reacquisition, and camera-only abort after shutter deselection. General abort/reacquire and two-agent barrier cases also pass (three focused cases). The shutter is a public-bus test peer; no physical shutter validation or new full-suite coverage measurement is claimed.

## DRV-115 follow-up

The camera-disconnect regression failed before the fix and passes afterward for preview, exposure batch and streaming, with bounded completion, cleared start switch, reconnection and successful acquisition in each case. Preview preserves its existing completion state and retry timing; failed batches publish ALERT. Retry exhaustion, streaming failure and abort/reacquire also pass (four focused cases). Prior full-suite totals and coverage measurements remain historical.

## DRV-117 follow-up

PRE_DELAY and POST_DELAY tests now each cover one/two frames and zero/nonzero delay, resume, abort and reacquisition. Controlled TRIGGER instances execute delays and their breakpoints while continuing to suppress dithering. Both delay cases, both capture-breakpoint cases, both dither cases and the two-agent barrier pass (seven focused cases). Prior full-suite totals and coverage measurements remain historical.

## DRV-118 follow-up

POST_BATCH now checks pending abort before reporting batch success. Its expanded regression failed before the fix and passes afterward for one/two frames, normal resume, abort while the breakpoint remains enabled, terminal states and cleared switches, no additional requests/images and successful subsequent batch. All six breakpoint cases, two-agent barrier and general abort/reacquire pass (eight focused cases). Prior full-suite totals and coverage measurements remain historical.
