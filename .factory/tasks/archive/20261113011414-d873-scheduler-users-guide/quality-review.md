# Task quality review

Verdict: pass
Reviewer and independence: independent reviewer subagent (fresh context, read-only) fact-checked the guide against the code; corrections applied by the coordinator, not independently reinspected.
Reviewed revision and working-tree state: branch `scheduler` at c52655486 with this task's changes.

## Scope and method
Every statement of `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` compared with the agent, header, sky and plan code, `indigo_prop_tool`, the Scripting Agent and `Sequencer.js`.

## Quality findings
Wrong, all corrected: `@twilight none` described as 0°; priorities described as strict level ordering, while the sky bonuses (up to 200 points by default) can outweigh the level; history example out of order. Misleading or missing caveats, all corrected: shared retry budget, retry delay after preemption, ABORT discarding one-off urgent tasks, SHUTDOWN hook exceptions, STATE Alert causes and counters, failure reasons, stop-timeout results, operations counted by the estimate and the estimate timeout, header ended by a plain comment and the 255-character line limit, one-sided nightly windows, start-time constraint check, cases in which an urgent task does not interrupt, requeued urgent order, delays not kept over a restart, hook rename, automatic stop with only hooks. Nits applied: value ranges, limits counting hooks and urgent tasks, REASON source, extra messages and reason, `scheduler_task.token`. Not applied: the plan's 256-run cap (not relevant for users).

## indigo_prop_tool change (owner request after the first review)
Reviewer: coordinator, not independent. Diff of `indigo_tools/indigo_prop_tool.c` read in full: `set_script` reads a file for items named `SCRIPT` or `TASK` (any property, as `set_script` is the explicit file mode; the `ADD_SCRIPT` NAME check is unchanged); the new `send_message` callback prints only for set requests and only messages whose property names the target device (over the network the `device` argument is the protocol adapter, so the property's device is used); file traffic is logged at trace level. Behaviour change for scripts parsing `set` output: messages of the target device add `<device>: <message>` lines. Guide sections rewritten to task files and the tool; JavaScript submit, status, watch and edit scripts removed; no remaining reference to them.

## Fix reinspection
Corrections re-read in context; code blocks unchanged after the example run; link check repeated.

## Quality verdict
Pass. One code defect found by the fact check is recorded as FND-20261113011414-d873-scheduler-users-guide-001.
