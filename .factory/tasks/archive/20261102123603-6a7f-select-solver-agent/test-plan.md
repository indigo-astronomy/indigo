# Test plan

| Criterion | Test (`test_agent_scripting_sequencer`) |
| --- | --- |
| AC-001 | "select solver agent step" asserts `start.length === 3` |
| AC-002 | "platesolver orchestration", "remote solver orchestration", "astrometry orchestration", final part of "select solver agent step" |
| AC-003 | "select solver agent step": peer named "Custom Solver", not auto-detectable, receives `precise_goto` settings; "all public constructors" |
| AC-004 | "meridian flip uses selected solver", "meridian flip uses solver selected by step". They enter through the sequencer's pause handler and resume at the end of the slew; transit wait and mount are not exercised |
| AC-005 | review |
| AC-006 | full suite |

Mutation checks: removing the flipper device hand-off and making the step not store the name must fail the matching tests.
