# Agent profile-name quality review

Verdict: pass
Date: 2026-11-05
Reviewer and independence: `guides_review` subagent; no production/test/documentation authorship. Independent execution of the verifier-built profile unit binary; separate security pass by the same reviewer, not independent dual-review certification.
Reviewed revision/state: HEAD `323a560ff37d036c34bd7984a5e38abed9036c6a`, branch `task/20261105074950-dd62-hide-agent-profile-names`, uncommitted dedicated fix. Prior unfinished MCP help/autostart work remains stashed and is outside this reviewed commit scope.
Reviewed tracked candidate: 17 changed files; aggregate SHA-256 `a8f0081d65e10547e90bf5847f752207f4569b677d095d6bdd697814f501dadb` using sorted repository-relative path, NUL, current bytes, NUL for each tracked changed file. This excludes task reports/registry metadata and is not a commit/tree identity.

## Scope and method

Read specification, plan, source analysis, quality/security roles and relevant core/test/driver rules. Inspected the full tracked diff, common property construction/enumeration/change matching, all eleven native agent attachment paths and their shared generic/filter/solver layers. Rechecked recorded-runner README/summary changes after release. Excluded vendor/off-limits drivers, archives, shell-compliance tests, transport/hardware operations and broad repository audit.

The common assignment occurs after allocation validation and tests the AGENT bit, rather than comparing the entire interface mask. Direct Alpaca/MCP/Scripting, generic Config/Test, filter Imager/Guider/Mount/Auxiliary and shared solver Astrometry/Platesolver paths all reach this constructor with AGENT. Independently inventoried all eleven native modules and verified every version is exactly one greater than HEAD; no module overrides PROFILE_NAME visibility. Mixed agent/device interfaces are covered without a name list or duplicated guards.

Non-agent devices receive the same false hidden default as before. Item definitions, permissions, profile selection, configuration filenames and persistence handlers are unchanged. Hidden names are not defined by either enumeration path, and indigo_property_match_changeable rejects undefined properties before rename/persistence handling. Existing PROFILE/CONFIG rules are untouched. MCP initialize version and its sole integration-test version expectation agree with 3.0.0.1. The authoritative property-reference comment describes the agent exception.

## Finding and reinspection

Canonical `FND-20261105074950-dd62-hide-agent-profile-names-001` is the original low-severity functional defect. The pre-fix regression observed unwanted definition, rename and persistence for pure/mixed AGENT, while physical controls passed. The central one-line repair addresses that cause. Final unit tests check targeted/general enumeration, undefined state, guessed-write rejection, unchanged names/labels and no common-file write for agents; ordinary rename, selection and persistence still pass. The original saved-profile regression is retained. Repair is verified; coordinator owns final registry closure/archive reconciliation. No additional review finding or unresolved blocking defect.

## Evidence and verdict limits

See [verification.md](verification.md) for exact baseline/reproducer/build/native/sanitizer commands and results. Inspected evidence for core and eleven-agent universal builds, unit 4/4 and MCP 30/30, each also passing the scoped sanitizer run. The unit includes the changed driver source directly; sanitizer evidence does not claim every linked library was instrumented.

Independently ran `indigo_test/build/unit/test_profile_selection` from repository root: exit 0, all four cases passed, including existing selection persistence and new non-agent rename/select/persist control. Independently checked git diff --check and configured Factory security boundaries: pass. Core SHA-256 `4182aaac54ec012f125c6153d5e16120086a976c86d2c6c3bfb3c836383d6269`; unit source `6dc97c22288d6c958622fe07346ea7159a5e5eda0c5180d364d30a6ad9745165`; MCP test `5e34abec62ca5c436afc76fabe35f0cf214fdbe035ebafac94862025d0f5fcfd`, matching final verifier evidence.

Pass is for this dedicated visibility fix and retained evidence. No hardware, Linux/Windows execution, complete agent-process regression or restored MCP WIP is certified. No new dependency, ABI, generator or lifetime behavior was introduced. Parent retains commit, process gates, archival and WIP restoration ownership under the explicit separate-commit authorization; this review grants no push/main integration permission and resets no maintenance checkpoint.
