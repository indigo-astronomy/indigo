# Astrometry and Solver mutual exclusion

## Conclusion
Use reciprocal `indigo_driver_initialized()` checks in the two drivers' `INDIGO_DRIVER_INIT` branches if the requirement is that only one solver driver may be initialized in a single INDIGO server. This reuses the documented conflicting-driver mechanism without adding properties or a new lock. Neither driver currently has these checks. This report proposes a change; it does not implement exclusion.

## Evidence at e5e77e58edd20f102577a8f931651f802bdfc612
- `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md:762–798` documents conflicting drivers and requires reciprocal checks.
- `indigo_libs/indigo_client.c:178–189` searches the local available-driver registry by entry-point name and the initialized flag. `indigo_libs/indigo/indigo_client.h:86–89` explicitly requires a synchronized caller.
- `indigo_drivers/ccd_qhy/indigo_ccd_qhy.cpp:1492–1499` is an existing pattern: INIT rejects an initialized conflicting driver with INDIGO_FAILED and preserves a shutdown last_action.
- `indigo_drivers/agent_astrometry/indigo_agent_astrometry.c:1321` and `indigo_drivers/agent_solver/indigo_agent_solver.c:811` enter INIT without checking the other agent. Their solve/config mutexes are per device, not a shared exclusion between drivers.
- `indigo_server/indigo_server.c:1208–1209,1257–1263,1266–1332` creates the Server drivers queue, enqueues driver requests and applies INIT results to initialized flags before processing the next item. A single request selecting both drivers therefore accepts the first successful INIT in property order and rejects the next conflicting INIT. Separate queued requests are likewise serialized.
- `indigo_server/indigo_server.c:1813` loads command-line drivers sequentially. `indigo_libs/indigo_client.c:231–237` publishes the initialized result before the next sequential call.
- `indigo_libs/indigo_client.c:242–265` removes the driver registry name on successful removal; the static server unload path clears initialized at `indigo_server/indigo_server.c:1312`. After successful unload the other agent can initialize. An inactive discovered/registered driver does not conflict.

## Minimal proposed change
In Astrometry INIT, check `indigo_driver_initialized("indigo_agent_solver")`. In Solver INIT, check `indigo_driver_initialized("indigo_agent_astrometry")`. Put each check before changing last_action, executable validation, cache creation, allocation or attach. On conflict, log the conflicting driver's name and return INDIGO_FAILED; leave last_action as SHUTDOWN so a later retry works. Retain the existing action-equality early return for an already initialized agent. Increment both driver versions as required by repository policy. No property-reference update is necessary unless implementation adds/removes a property, which this proposal does not require.

The server already handles INDIGO_FAILED by deselecting the failed driver and sending a generic load-failure message (`indigo_server/indigo_server.c:1246–1254,1280–1300`). A specific conflict log can identify the active agent without changing server behavior. Switching should unload the current agent first, then enable the replacement; a combined request follows property order, so a one-request swap is not guaranteed in both directions.

## Limits and alternatives
- The API is a registry query, not atomic acquisition. `add_driver()` releases its mutex before calling INIT (`indigo_client.c:227–233`); two concurrent callers outside server serialization can both observe the other driver as inactive. An embedded host must serialize driver lifecycle operations through publication of initialized state.
- Direct entry-point calls without registration are invisible to this query. Existing agent tests call INIT directly (`test_agent_astrometry.c:561`, `test_agent_solver.c:334`), so a new exclusion test must register drivers via the real loader API or drive the actual server.
- Each process has its own registry. Separate servers, executable adapters and remote agents are outside this exclusion contract. Existing additional instances of the selected driver remain possible; this proposal excludes different driver implementations, not every solver instance.
- Exclusion lasts for the initialized driver's lifetime, including idle time. Normal completion, solve failure and abort do not release it. Successful unload does. This avoids introducing a lock-release path into each solving/abort branch.
- If both agents must remain loaded and only their operations must exclude each other, this API is unsuitable. A shared coordinator would need to cover uploaded images, automatically snooped frames, solve/sync/center and complete polar-alignment workflows, including failure, abort and teardown. Checking only AGENT_START_PROCESS is insufficient: shared platesolver code enables image solving by default (`indigo_platesolver.c:997`) and accepts frames through other paths (`:1516,:1568`).
- Token-based exclusive device access (`indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md`) controls which clients may change a particular device; it does not prevent two different agent devices from initializing.
- `indigo_try_global_lock()` and `indigo_global_unlock()` are deprecated no-ops (`indigo_driver.c:79–87`; `indigo_driver.h:307–313`), so they cannot provide exclusion.

## Future implementation verification
Test both initialization orders through registry-managed initialization; reject the second without device/client/cache side effects; unload the first then successfully retry the rejected agent; exercise failure of the first initialization; repeated INIT/INFO and unload/reload; distinguish registered-but-inactive drivers; cover static and dynamic server paths and two queued enable requests. Verify a combined enable request leaves only one initialized agent. Test a two-step swap in each direction. Keep existing solving suites as regressions and label concurrent raw library loading or cross-process exclusion unsupported unless the requirement is expanded. Astrometry executable prerequisites must be controlled with existing test fixtures so missing tools cannot masquerade as successful exclusion.

ASTAP is excluded at the owner's request. No obsolete-driver changes are proposed.
