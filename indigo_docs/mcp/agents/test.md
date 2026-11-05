# Test Agent — developer scaffold

**DRAFT — source inspection only; no developer test action was executed for this manual and inclusion in compiled MCP help does not validate the test actions.**

Source module `indigo_agent_test`; device `Test Agent`. This is a developer demonstration of handler scheduling and property updates. It is not an observing agent, simulator of an astronomical instrument, or replacement for the automated test suite.

The server source includes and registers this entrypoint in its `STATIC_DRIVERS` build. The directory has C/header/README sources, but availability in a particular installation must be established from server `DRIVERS` rather than assuming a separately packaged dynamic module exists. If `indigo_agent_test` is absent, do not fabricate a load command or load an unrelated driver to obtain it. See [server/device preparation](../server-and-devices.md).

## Function inventory and exact actions

The only custom property is `TEST`, an at-most-one RW switch with item IDs `1`, `2`, `3`, `4`. These literal names do not follow the modern `X_` naming convention, but are the names in this existing scaffold; clients must not rename them.

| Function ID | Start | Actual handler behavior / observation |
| --- | --- | --- |
| `test.test-1` | `TEST.1 = true` | Log start; sleep approximately1second in its handler; clear item1; publish `TEST=Ok`; log finish. |
| `test.test-2` | `TEST.2 = true` | Same one-second demonstration for item2. |
| `test.test-3` | `TEST.3 = true` | Same one-second demonstration for item3. |
| `test.test-4` | `TEST.4 = true` | Queue handler1 twice, handler3 with a10second delay, and handler2 with explicit priority100; clear item4 and publish `Ok`. Other queued callbacks can run after this publication. |

Prerequisites: a development/test server where the actual item is available, no science work depending on its handler queues, and access to server logs if scheduling order matters. Start only one switch at a time. The property handler publishes `Busy` before scheduling a selected demonstration and ignores new test changes while `TEST` is busy. For tests1–3 the later `Ok`/cleared item corresponds to that handler returning through its explicit update.

Test4 illustrates why a parent callback's `Ok` is not proof that its child queue/delayed work is finished. Inspect the actual log sequence and wait for scheduled work to drain. The delay is in seconds; priority100 is a scheduler priority argument, not a device value, percentage or time delay. Do not infer a guaranteed wall-clock ordering merely from the property label.

## Stop, results and limits

No public abort/cancel property is implemented. Do not promise that `TEST.1=false`, device disconnect or module unload cancels sleeping/delayed callbacks safely. Let demonstrations finish in a disposable development session; investigate a stuck run from logs and queue behavior rather than running it against production hardware. There is no image, coordinate, science result or automatic PASS/FAIL evaluation produced by these four switches.

Inherited standard agent/device properties are lifecycle metadata and are not additional astronomical test functions. This source is not advertised as a production-safe job runner. The guide covers all four custom handlers; packaging, queue timing under load and unload safety have not been runtime-validated for this draft.

Sources: [implementation](../../../indigo_drivers/agent_test/indigo_agent_test.c), [header](../../../indigo_drivers/agent_test/indigo_agent_test.h), [static server registration](../../../indigo_server/indigo_server.c), [property conventions](../../PROPERTIES.md).
