# Agent profile-name visibility analysis

Read-only production-source analysis for the owner-requested common-layer correction. Analyst writes only this report; coordinator owns implementation/version/reference edits. Current source is the task branch based on323a560, with prior MCP embedding/autostart WIP stashed. No runtime/hardware/build checks were run by this analyst.

## Conclusion

Set `PROFILE_NAME_PROPERTY->hidden = (interface_mask & INDIGO_INTERFACE_AGENT) != 0` in `indigo_device_attach()` after successful property allocation. This is the smallest consistent change: the authoritative interface flag covers every current agent, including mixed agent/device interfaces, without a name list or duplicated per-agent initialization. Existing PROFILE/CONFIG hiding remains owned by existing attachment layers. Ordinary devices without AGENT retain the initialized visible property and its existing profile selection/persistence behavior.

All eleven production native agent modules are covered. Git-tracked inventories under portable, Linux, macOS and optional driver roots contain no additional platform-only agent module. A maintained-production source scan found no other AGENT attachment sites and no redefinition, replacement or visibility override of the common PROFILE_NAME property. Vendor trees, archived records, shell-compliance tests and the five off-limits drivers were excluded. This is an agent/profile-contract inspection, not a general repository audit.

## Attachment evidence

Line numbers below refer to the inspected pre-edit sources; implementation/version edits may shift them.

| Module | Initial attach call | Route to common base |
| --- | --- | --- |
| Configuration | `indigo_drivers/agent_config/indigo_agent_config.c:502` | `indigo_agent_attach()` |
| Test | `indigo_drivers/agent_test/indigo_agent_test.c:98` | `indigo_agent_attach()` |
| Imager | `indigo_drivers/agent_imager/indigo_agent_imager.c:2607` | `indigo_filter_device_attach()` with CCD/WHEEL/FOCUSER |
| Guider | `indigo_drivers/agent_guider/indigo_agent_guider.c:2531` | Filter with CCD/GUIDER |
| Mount | `indigo_drivers/agent_mount/indigo_agent_mount.c:2058` | Filter with MOUNT/DOME/GPS/AUX_JOYSTICK |
| Auxiliary | `indigo_drivers/agent_auxiliary/indigo_agent_auxiliary.c:56` | Filter with AUX |
| Platesolver | `indigo_drivers/agent_platesolver/indigo_agent_platesolver.c:690` | `indigo_platesolver_device_attach(..., 0)` |
| Astrometry | `indigo_drivers/agent_astrometry/indigo_agent_astrometry.c:1137` | Same shared solver base |
| Alpaca | `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c:983` | Direct `indigo_device_attach(..., INDIGO_INTERFACE_AGENT)` |
| MCP | `indigo_drivers/agent_mcp/indigo_agent_mcp.c:1329` | Direct agent-mask attach |
| Scripting | `indigo_drivers/agent_scripting/indigo_agent_scripting.c:1125` | Direct agent-mask attach |

`indigo_libs/indigo_agent.c:43` passes AGENT unchanged to the base; lines44–46 hide CONNECTION/CONFIG/PROFILE. `indigo_libs/indigo_filter.c:49` ORs AGENT with the supplied device interfaces. `indigo_libs/indigo_platesolver.c:913–915` delegates to this filter base, so a zero solver-specific interface argument does not omit AGENT. The public bit is `indigo_libs/indigo/indigo_bus.h:97`.

The common constructor is `indigo_libs/indigo_driver.c:1456`. PROFILE_NAME allocation and initialization are at1510–1520; PROFILE allocation follows. No agent allocates an independent profile-name property. Additional instances reuse the same attach paths rather than a separate profile initialization contract. An interface-bit test, rather than equality to AGENT, is required for Imager, Guider, Mount and Auxiliary.

## Definition, guessed writes and non-agent compatibility

Common enumeration requests PROFILE_NAME at `indigo_libs/indigo_driver.c:1583`. Both `indigo_define_property_to_client()` (`indigo_libs/indigo_bus.c:1068–1075`) and `indigo_define_property()` (`1122–1129`) guard definition with `!property->hidden`; a hidden property therefore does not become defined through either targeted or broadcast enumeration.

The base mutation branch at `indigo_libs/indigo_driver.c:1666–1689` uses `indigo_property_match_changeable()`. That matcher (`indigo_libs/indigo_bus.c:2082–2092`) rejects undefined properties before matching names or permission. Therefore a guessed incoming agent PROFILE_NAME request does not copy text, redefine PROFILE labels or persist names through that branch. The proposed change need not add a duplicate write guard. This reasoning assumes a fresh attach, which is exactly where the visibility default is set; live retroactive property hiding is not part of this change.

For a non-agent attach the predicate is false, matching the current default visibility. Existing items, permission, PROFILE labels and selected profile are unchanged. Persistence remains the existing `.common` contract (`indigo_libs/indigo_driver.c:769–772`,1666–1689); no filename, serialization, ABI or physical-device attach code changes are needed. Hidden agent names may no longer restore from an old common profile-name record because the property is undefined; that is consistent with removing this unusable agent control, not a migration of device profile data. Agent-specific configuration mechanisms remain separate.

## Inventory method and residual risk

Used `git ls-files` plus Python standard-library text searches over936 maintained source/header/DSL/Objective-C/C++ files in the five scoped production roots. Searched AGENT interface occurrences, agent attachment calls, `PROFILE_NAME_PROPERTY`, `profile_name_property`, and the literal property name. All common profile-name references found were confined to the shared driver implementation and public declarations; there were no agent or physical-driver overrides of its visibility or allocation in the scanned scope. Agent module inventory is exactly the eleven rows above. No archives/vendor/private configuration were read.

Duplicated assignments in generic-agent/filter bases plus three standalone agents would require several coupled fixes and still leave future direct AGENT callers exposed. The common predicate expresses the required contract once. It has no allocation, threading, transport, process or resource-lifetime effect. Main regression risks are an incorrect bit/equality test, placing the assignment before allocation validation, or accidentally hiding physical-device names. Required checks should cover pure AGENT, AGENT combined with device flags, ordinary non-agent profile rename/select/persistence, targeted/broadcast enumeration and guessed writes; then retain relevant MCP discovery regression. All eleven affected driver versions should increase under repository policy. Runtime evidence and final review remain coordinator/verifier responsibilities; this report does not claim tests passed or certify unavailable platforms.
