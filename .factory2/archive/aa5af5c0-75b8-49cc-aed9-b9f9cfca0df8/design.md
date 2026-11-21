# Design: a coordinates request refused before BUSY does not keep its ALERT

Ticket aa5af5c0-75b8-49cc-aed9-b9f9cfca0df8. Base: master at 8c1a3dd94 (contains a09e0f878 of ticket 42b85be0).
Status: proposal. Nothing in the repository was changed. The library and generator diffs below need the owner's approval.

## 1. Recommendation (summary)

Alternative B, "an explicit refusal entry point in the mount base class":

1. `indigo_mount_driver.c/.h`: the state machine of `indigo_update_coordinates()` moves unchanged into a private `update_coordinates(device, refusal, message)`. `indigo_update_coordinates()` calls it with `refusal = false`. A new public `indigo_reject_coordinates_change(device, format, ...)` sets ALERT and calls it with `refusal = true`, so the ALERT is kept until the next BUSY whatever was published before. A new macro `INDIGO_REJECT_COORDINATES_CHANGE_IF(c, f, ...)` is the coordinates form of `INDIGO_REJECT_CHANGE_IF`. The flag is still read and written only inside that one function, apart from the existing clear on disconnect.
2. `indigo_generator.c`: for `MOUNT_EQUATORIAL_COORDINATES` only, the parked-mount admission guard and every `reject_change` block use `INDIGO_REJECT_COORDINATES_CHANGE_IF`, and the parked guard in the handler calls `indigo_reject_coordinates_change()`. `MOUNT_MOTION_*` and `MOUNT_TRACKING` stay as they are.
3. Drivers: all ten generated mount drivers are regenerated and get a version bump. In `mount_lx200` (classic guiding) and `mount_synscan` (a slew it did not claim), the refusal inside the handler is published through the new function instead of `indigo_update_property()`.
4. Clients see no BUSY for a refused request, as today. The ALERT now stays until the next request or motion, so the Mount Agent and other clients no longer see a refusal turn into OK.
5. Tests: the shared helper `check_refusal_kept()` in `mount_coordinates_test_common.h`, refusal cases in the ten driver suites, unit cases in `test_mount_alignment_load.c` and generator cases in `test_generator_architecture.c`.

## 2. Open questions for the owner

1. **Entry point.** You said `indigo_update_coordinates()` is the one place for "ALERT must stay ALERT". Do you accept a second public entry point, `indigo_reject_coordinates_change()` plus `INDIGO_REJECT_COORDINATES_CHANGE_IF`, that shares the single internal state machine (Alternative B)? The other option is Alternative A: the guard publishes BUSY and then ALERT through `indigo_update_coordinates()`, which needs no library change. If B, is the name acceptable?
2. **Duration of the kept ALERT.** A refused request on a parked mount then keeps `MOUNT_EQUATORIAL_COORDINATES`, and with it `MOUNT_HORIZONTAL_COORDINATES`, in ALERT through the unpark until the next GOTO, sync or visible motion. Is that intended?
3. **Scope, failures after BUSY.** The audit found ALERTs published with `indigo_update_property()` after the request's BUSY: failures and aborts, not refusals (section 4.3). Should they be fixed in this ticket, or registered as a separate ticket? I recommend a separate ticket.
4. **Scope, base-class sync path.** `indigo_mount_change_property()` refuses a sync through `indigo_mount_sync()` without BUSY (`indigo_mount_driver.c:811-812`), and that ALERT is not kept either. None of the ten generated drivers reaches this path. Include the two-line fix shown in 5.1?
5. **Off-limits drivers.** `mount_asi` (`indigo_mount_asi.c:1508-1511`) and `mount_mxhd` (`indigo_mount_mxhd.c:315-318`) refuse a parked request with `indigo_update_coordinates()` in ALERT without BUSY, so they keep the defect. Leave them as they are, or authorise a one-line change in each?
6. **Commits.** I propose one ticket and one change commit, as for 42b85be0 (a09e0f878): the generator change and the regenerated output of all ten drivers cannot be separated, and each driver's README test record goes in that same commit. Do you want a different split?
7. **Test-run mode.** The final recorded runs use `tools/run_driver_test.py` for the ten mount drivers, plus `agent_mount` if its suite changes. Should they be non-interactive simulator runs?

## 3. Root cause

`indigo_update_coordinates()` (`indigo_libs/indigo_mount_driver.c:1386-1412`) sets `keep_coordinates_alert` only when it publishes ALERT and the previous state *it* published was BUSY (line 1394). A refusal escapes this in one of two ways:

- **(A) Admission refusal before BUSY.** `INDIGO_REJECT_CHANGE_IF` leads to `indigo_reject_change()` (`indigo_libs/indigo_bus.c:1342-1367`), which publishes ALERT through `indigo_update_property()`. The flag is never set, and a poll that sets OK publishes OK.
- **(B) Refusal after the request's BUSY, published with `indigo_update_property()`.** `coordinates_state` stays BUSY, the ALERT is not seen, and the next call with OK publishes OK (this is described in `indigo_mount_driver.h:619-621`). Per the owner's rule this bypass is a defect in itself.

Whether the ALERT survives depends on the driver's idle poll. The polls of synscan (`indigo_mount_synscan.c:1027`), rainbow (`indigo_mount_rainbow.c:366-370`, `:CL0#`), starbook (`indigo_mount_starbook.c:564-567`), lx200 (`indigo_mount_lx200.c:4019-4026`), nexstar (`indigo_mount_nexstar.c:491-494`) and temma (`indigo_mount_temma.c:452-455`) set OK whenever they are idle, so the ALERT is lost within one poll. The polls of simulator, ioptron, nexstaraux and pmc8 republish the current state, so there the ALERT survives only by accident, until some path sets OK.

## 4. Failing scenarios (evidence)

### 4.1 Admission refusals (class A)

| # | Driver | Refusal | Generated code | Lost today? |
|---|---|---|---|---|
| A1 | all 10 generated | parked-mount admission guard, emitted by `indigo_generator.c:2097-2099` | e.g. `indigo_mount_rainbow.c:1243`, `indigo_mount_lx200.c:5398`, `indigo_mount_synscan.c:2745`, `indigo_mount_simulator.c:641` (same line in every driver's change branch) | yes for synscan, rainbow, starbook, lx200, nexstar, temma; held only by accident for the other four |
| A2 | mount_rainbow | `reject_change` "Mount is searching for home" (`.driver:812-815`) | `indigo_mount_rainbow.c:1244` | yes |
| A3 | mount_rainbow | `reject_change` "Guiding pulse in progress" (`.driver:816-819`) | `indigo_mount_rainbow.c:1245` | yes |
| A4 | base class | sync refused by `indigo_mount_sync()` in `indigo_mount_change_property()` | `indigo_mount_driver.c:811-812` | yes, but no generated driver reaches it (Q4) |
| A5 | mount_asi, mount_mxhd (off-limits) | parked refusal through `indigo_update_coordinates()` without BUSY | `indigo_mount_asi.c:1508-1511`, `indigo_mount_mxhd.c:315-318` | yes (Q5) |

### 4.2 Refusals inside the handler after BUSY, published around `indigo_update_coordinates()` (class B, in scope)

| # | Driver | Refusal | Code |
|---|---|---|---|
| B1 | all 10 generated | handler parked guard (a request admitted just before a park was accepted), emitted by `indigo_generator.c:1888-1896` with `INDIGO_UPDATE_PROPERTY_STATE` | e.g. `indigo_mount_lx200.c:4799-4803` |
| B2 | mount_lx200 | "Classic guiding is active" | `.driver:4658-4662`, `indigo_mount_lx200.c:4806-4810`; also refuses a sync |
| B3 | mount_synscan | a request that claimed neither sync nor slew (`global_mode != SLEWING`) is refused silently | `.driver:2073-2074`, `indigo_mount_synscan.c:2252-2253`. Reachable only when a motion starts between admission and handler: the CONTROLLER alignment mode is hidden (`.driver:1965-1971`), so a sync always claims |

### 4.3 Failures after BUSY that bypass `indigo_update_coordinates()` (found by the audit, Q3)

These are failures, not refusals:

- synscan: `synscan_set_coordinate_state()` (`indigo_mount_synscan.c:1089-1095`, used at 1602, 1904, 1928, 1952, 2028, 2475, 2598), the slew failure at 2268 and the abort at 2477.
- nexstaraux: `mount_slew_failed()` at 899-902, the stopping/arrival paths at 931-933 and 966-968, and the park BUSY at 1233-1234.
- pmc8: the abort at 1080-1082.

### 4.4 Effect on the Mount Agent

`indigo_agent_mount.c:436-456` waits up to 3 s for BUSY. A refusal never produces BUSY, so a driver poll within those 3 s publishes OK. Lines 499-506 then accept OK when the mount is within `TARGET_TOLERANCE` (0.25°) of the request. For a slew, the "did not start the slew" heuristic catches most refusals. For a **sync** of a few arcminutes nothing does, so it is reported "Synced". Concrete cases:

- a sync on mount_rainbow while it is searching for home (A2);
- a sync on mount_lx200 during classic (host-timed) guiding (B2), which is a realistic plate-solve-and-sync while guiding;
- any request racing a park on all generated drivers (A1, B1); `unpark_before_coordinates()` at lines 340-370 avoids the plain parked case for the agent itself.

Direct clients (Ain, scripts) are affected in every case.

### 4.5 Reproduction (run in a scratch copy of HEAD, repository untouched)

The library and the drivers were built in a `git archive HEAD` copy, under `.../scratchpad/repro`. Then a check was added after the existing refusal assertion in each case: wait 3 s and check that `MOUNT_EQUATORIAL_COORDINATES` is still ALERT.

| Case | Result |
|---|---|
| `rainbow_park_slews_to_park_position` (A1) | FAIL, state OK after one poll |
| `rainbow_refuses_motion_while_searching_for_home` (A2) | FAIL, state OK |
| `lx200_park_rejects_motion_and_unpark_recovers` (A1) | FAIL, state OK |

These checks become the regression tests below; the existing assertions only test the immediate ALERT.

## 5. Alternatives

| | A: guard publishes BUSY then ALERT via `indigo_update_coordinates()` | **B: refusal entry point in the base class (recommended)** | C: keep every ALERT until BUSY | D: move the parked guard into the library macros |
|---|---|---|---|---|
| Library change | none | new function and macro; the state machine stays in one function | condition at line 1394 dropped | B plus the parked check inside `INDIGO_COPY_*_PROCESS_COORDINATES_CHANGE` |
| Generator change | coordinates guard emits copy-targets, BUSY, then ALERT | coordinates guard and `reject_change` emit the new macro; handler guard calls the new function | none | admission guard no longer emitted for coordinates |
| What clients see | BUSY (targets = refused request), then ALERT. The agent marks the request "accepted" (line 1630), and for a request from another client it aborts the imager and guider processes as "slewing" (lines 1634-1645). Clients that treat BUSY as accepted are misled | ALERT once, values = real position, targets unchanged, message attached; ALERT kept until the next BUSY. `MOUNT_HORIZONTAL_COORDINATES` follows in ALERT (new) | as B, but a failed position readback also stays ALERT after recovery, which regresses 42b85be0 | as B |
| Covers asi/mxhd | no | no (Q5) | yes | no |
| Verdict | rejected: a refused request must not look started | **recommended** | rejected: breaks the documented recovery after a read failure | rejected for now: same effect as B with more churn, and splits the parked guard between library and generator (motion and tracking stay in the generator) |

### 5.1 Proposed library diff (Alternative B)

`indigo_libs/indigo/indigo_mount_driver.h`, after `indigo_update_coordinates()`:

```c
/** Refuse a MOUNT_EQUATORIAL_COORDINATES request: set the state to ALERT and publish it through the state machine of
    indigo_update_coordinates() with the reason, keeping the ALERT until the next BUSY like the ALERT that ends a BUSY
    request. Every item is marked for update, so the client sees the values (the reported position) and targets again.
    Use it for every refusal of a coordinates request, before the copy (through INDIGO_REJECT_COORDINATES_CHANGE_IF) and
    in the change handler after the request's BUSY.
 */
INDIGO_EXTERN void indigo_reject_coordinates_change(indigo_device *device, const char *format, ...);

/** INDIGO_REJECT_CHANGE_IF for MOUNT_EQUATORIAL_COORDINATES: a refused request is kept in ALERT through
    indigo_reject_coordinates_change(); a BUSY property belongs to a running operation and only gets the message.
 */
#define INDIGO_REJECT_COORDINATES_CHANGE_IF(c, f, ...) if (c) { if (MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state == INDIGO_BUSY_STATE) { indigo_reject_change(device, MOUNT_EQUATORIAL_COORDINATES_PROPERTY, f, ##__VA_ARGS__); } else { indigo_reject_coordinates_change(device, f, ##__VA_ARGS__); } return INDIGO_OK; }
```

The doc comment of `indigo_update_coordinates()` gains one sentence: "A refusal published through indigo_reject_coordinates_change() is kept the same way."

`indigo_libs/indigo_mount_driver.c` (add `#include <stdarg.h>`):

```diff
-void indigo_update_coordinates(indigo_device *device, const char *message) {
+// The only place of the keep-ALERT state machine; a refusal is kept whatever was published before it.
+static void update_coordinates(indigo_device *device, bool refusal, const char *message) {
 	indigo_property_state state = MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state;
 	if (state == INDIGO_BUSY_STATE) {
 		MOUNT_CONTEXT->keep_coordinates_alert = false;
-	} else if (state == INDIGO_ALERT_STATE && MOUNT_CONTEXT->coordinates_state == INDIGO_BUSY_STATE) {
+	} else if (state == INDIGO_ALERT_STATE && (refusal || MOUNT_CONTEXT->coordinates_state == INDIGO_BUSY_STATE)) {
 		MOUNT_CONTEXT->keep_coordinates_alert = true;
 	} else if (state == INDIGO_OK_STATE && MOUNT_CONTEXT->keep_coordinates_alert) {
 	... (rest unchanged)
 }
+
+void indigo_update_coordinates(indigo_device *device, const char *message) {
+	update_coordinates(device, false, message);
+}
+
+void indigo_reject_coordinates_change(indigo_device *device, const char *format, ...) {
+	char message[INDIGO_VALUE_SIZE] = { 0 };
+	if (format != NULL) {
+		va_list args;
+		va_start(args, format);
+		vsnprintf(message, INDIGO_VALUE_SIZE, format, args);
+		va_end(args);
+	}
+	for (int i = 0; i < MOUNT_EQUATORIAL_COORDINATES_PROPERTY->count; i++) {
+		MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items[i].do_update = true;
+	}
+	MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
+	update_coordinates(device, true, *message ? message : NULL);
+}
```

Optional, only if Q4 is answered yes (`indigo_mount_change_property()`, line 811):

```diff
-			indigo_mount_sync(device, sync_ra, sync_dec);
-			indigo_update_coordinates(device, NULL);
+			bool synced = indigo_mount_sync(device, sync_ra, sync_dec);
+			update_coordinates(device, !synced, NULL);
```

`update_coordinates()` is declared before `indigo_mount_change_property()`, or the function is moved there. No property, item or ABI of an existing function changes, so `PROPERTIES.md` is not affected.

### 5.2 Proposed generator diff

`indigo_tools/indigo_generator.c`, handler guard (`write_c_property_change_handler`, 1889-1891):

```diff
 		write_line("\tif (!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value) {");
-		write_line("\t\tindigo_send_message(device, %s, \"Mount is parked!\");", property->handle);
-		write_line("\t\tINDIGO_UPDATE_PROPERTY_STATE(%s, INDIGO_ALERT_STATE, NULL);", property->handle);
+		if (!strcmp(property->id, "MOUNT_EQUATORIAL_COORDINATES")) {
+			// the refusal of the admitted request is kept, the next poll must not publish it as success
+			write_line("\t\tindigo_reject_coordinates_change(device, \"Mount is parked!\");");
+		} else {
+			write_line("\t\tindigo_send_message(device, %s, \"Mount is parked!\");", property->handle);
+			write_line("\t\tINDIGO_UPDATE_PROPERTY_STATE(%s, INDIGO_ALERT_STATE, NULL);", property->handle);
+		}
```

Change branch (2097-2102):

```diff
+				bool coordinates = !strcmp(property->id, "MOUNT_EQUATORIAL_COORDINATES");
 				if (!strcmp(property->id, "MOUNT_EQUATORIAL_COORDINATES") || ...) {
-					write_line("\t\tINDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, %s, \"Mount is parked!\");", property->handle);
+					if (coordinates) {
+						write_line("\t\tINDIGO_REJECT_COORDINATES_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, \"Mount is parked!\");");
+					} else {
+						write_line("\t\tINDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, %s, \"Mount is parked!\");", property->handle);
+					}
 				}
 				for (reject_type *reject = property->rejects; reject; reject = reject->next) {
-					write_line("\t\tINDIGO_REJECT_CHANGE_IF(%s, %s, \"%s\");", reject->condition, property->handle, reject->message);
+					if (coordinates) {
+						write_line("\t\tINDIGO_REJECT_COORDINATES_CHANGE_IF(%s, \"%s\");", reject->condition, reject->message);
+					} else {
+						write_line("\t\tINDIGO_REJECT_CHANGE_IF(%s, %s, \"%s\");", reject->condition, property->handle, reject->message);
+					}
 				}
```

Reverse extraction does not parse these macros (no other tool in `indigo_tools/` references them), so `indigo_generator -c` is unaffected.

### 5.3 Driver changes and impact

| Driver | `.driver` change | Version | Regenerated diff |
|---|---|---|---|
| mount_simulator | version only | 27 → 28 | two guard lines |
| mount_synscan | B3: `INDIGO_UPDATE_PROPERTY_STATE(... ALERT, NULL)` becomes `indigo_reject_coordinates_change(device, "Mount is moving");` (the message wording is to be confirmed) | 25 → 26 | guards and B3 |
| mount_rainbow | version only; A2 and A3 are converted by the generator | 27 → 28 | three admission lines and the handler guard |
| mount_starbook | version only | 14 → 15 | guards |
| mount_ioptron | version only | 69 → 70 | guards |
| mount_lx200 | B2: `MOUNT_EQUATORIAL_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE; indigo_update_property(..., "Classic guiding is active");` becomes `indigo_reject_coordinates_change(device, "Classic guiding is active");` (the `return` stays) | 85 → 86 | guards and B2 |
| mount_nexstar | version only | 52 → 53 | guards |
| mount_nexstaraux | version only | 26 → 27 | guards |
| mount_pmc8 | version only | 19 → 20 | guards |
| mount_temma | version only | 28 → 29 | guards |

- The `MIGRATION_STATUS.md` automated-test counts of each driver are updated; the Comment column is not touched.
- No new files, so the Xcode project does not change.
- `mount_asi`, `mount_mxhd`, `system_ascol` and `system_alpaca` are untouched; their behaviour is unchanged (Q5).

## 6. Test plan

Shared helper, in `indigo_test/integration/mount_coordinates_test_common.h`:

```c
// From `from` on: the refusal is one ALERT publication of the equatorial coordinates with no BUSY before it, the
// horizontal coordinates published with it are ALERT as well, every later equatorial publication is ALERT, and at
// least `polls` of them were published (the poll ran); the values stay the position (ra0, dec0).
static bool check_refusal_kept(const char *label, const coordinates_publication *events, int count, int from, int polls, double ra0, double dec0, double tolerance);
```

The helper records through `start_coordinates_publications()` (which forces updates, so unchanged polls are seen). A companion check uses the existing `check_request_publication()`: the next accepted request publishes BUSY and ends OK, so the ALERT is released.

| Suite (file) | New or extended cases |
|---|---|
| unit, `test_mount_alignment_load.c` | `coordinates_refusal_is_kept_until_busy`: a refusal from OK publishes one ALERT with the message; a later OK is published as ALERT; BUSY clears the flag; disconnect clears it. `reject_macro_on_busy_only_sends_message`: the state, values and flag are untouched. If Q4 is yes, `base_sync_refusal_is_kept`. |
| `test_generator_architecture.c` | Extend `mount_coordinates_busy_goes_through_update_coordinates`: coordinates use `INDIGO_REJECT_COORDINATES_CHANGE_IF` for the parked guard and for a `reject_change` block; the handler guard calls `indigo_reject_coordinates_change`; `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA` and `MOUNT_TRACKING` keep `INDIGO_REJECT_CHANGE_IF` and `INDIGO_UPDATE_PROPERTY_STATE`. |
| each of the 10 `test_mount_<driver>_simulator.c` | `<driver>_parked_goto_refusal_is_kept` and `<driver>_parked_sync_refusal_is_kept` (extend the existing parked case where one exists: rainbow 483, lx200 2072, ioptron 1795, nexstar 2049), followed by unpark and an accepted GOTO that clears it. |
| rainbow | Extend `rainbow_refuses_motion_while_searching_for_home` (A2) and `rainbow_refuses_collisions_with_manual_motion_and_pulses` (A3). Add the handler-guard race B1 with the existing `hold_queue()`/`release_queue()` helpers: hold the queue, admit a GOTO (BUSY), accept a park, release; the ALERT is kept. |
| lx200 | `lx200_classic_guiding_refusal_is_kept` (B2), with GOTO and sync during a classic pulse, based on `lx200_classic_timed_guiding`. |
| synscan | B3 needs a motion to start between admission and handler; cover it with a queue hold if the suite has one, otherwise justify it as covered by the shared function in the ticket record. |
| `test_agent_mount_simulator.c` | Optional: "mount simulator: sync racing a park is reported failed". |

The reproductions in 4.5 must fail before the change and pass after it. The final recorded runs use `python3 tools/run_driver_test.py mount_<driver>` for each of the ten drivers (mode per Q7). The `unit` flow check covers the library.

## 7. Documentation to update

- `indigo_docs/DRIVER_GENERATOR_BASICS.md`:
  - lines 250-251: the change-branch skeleton gets the coordinates macro;
  - line 409 (`reject_change`): add the coordinates variant;
  - line 427 (parked-mount admission and the handler repeat for coordinates);
  - line 430: refusals are kept, and the rule "publish every coordinates ALERT through `indigo_update_coordinates()` or `indigo_reject_coordinates_change()`, never `indigo_update_property()`".
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`: lines 444-448 (guards), 457 (kept ALERT: refusals) and 503 (`reject_change` on coordinates).
- `indigo_docs/TIMERS_AND_QUEUES.md`: lines 338-341, add `INDIGO_REJECT_COORDINATES_CHANGE_IF`.
- `indigo_libs/indigo/indigo_mount_driver.h`: doc comments, as in 5.1.
- `indigo_docs/PROPERTIES.md`: no change, since no property or item is added or removed.

## 8. Split and order

One ticket, one change commit at `flow close`, as in 42b85be0 (Q6).

The generator change, the library function it emits and the regeneration of all ten drivers have to land together, otherwise the checked-in outputs are stale or do not compile. The "one driver per commit" rule is about recorded test runs. It is met by recording each driver's run in its README `## Testing` section and in the regenerated `TEST_SUMMARY.md` inside the same change commit, as was done for a09e0f878.

Order of work:

1. Unit tests and the library (red, then green).
2. The generator and its architecture test.
3. Regenerate the ten drivers, with the B2/B3 edits and version bumps.
4. Driver suite cases (red against the old drivers, recorded in the ticket notes as the reproduction).
5. Documentation.
6. Recorded runs, one per driver.

The class-4.3 bypasses (Q3) and the off-limits drivers (Q5) are kept out unless the owner decides otherwise.
