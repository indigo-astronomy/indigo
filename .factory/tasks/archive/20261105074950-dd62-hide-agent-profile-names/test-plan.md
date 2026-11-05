# Agent profile-name regression plan

Test writer scope: existing `indigo_test/unit/test_profile_selection.c`, only the expected version in existing `indigo_test/integration/test_agent_mcp.c`, and this task's test-plan/verification reports. No source, Makefile, new permanent test file, hardware or broad suite changes.

Owner contract: PROFILE_NAME is hidden for every device whose interface mask includes INDIGO_INTERFACE_AGENT; pure and combined masks both qualify. Hiding must prevent definition/enumeration and guessed-name renaming through the base handler. Non-agent physical-device profile names remain visible/editable and persist; existing profile selection/config availability remains unchanged. The separate stashed MCP autostart/help work is out of scope.

| Coverage | Fixture and assertions |
| --- | --- |
| S1: pure agent interface | Base attachment with AGENT alone; hidden before enumeration, not defined, no client definition for both exact-filter and full enumeration |
| S1: mixed agent interface | AGENT combined with CCD/AUX; same visibility and write boundary |
| S2: guessed-name write | Submit PROFILE_NAME.NAME_0 to base change handler despite hidden property; default text/profile label unchanged, no update event or private .common file created |
| S2: non-agent control | CCD/AUX mask; visible and defined, profile-name definition observed, rename updates text/selector label and persists exact synthetic value into private .common file; PROFILE_1 remains selectable |
| S2/S4: existing physical persistence | Preserve existing selection_tracks_saved_profile test verbatim |
| S3/S4: agent version compatibility | Existing 30-case MCP suite, default-disabled baseline contract retained; update only expected driver version 3.0.0.0 to 3.0.0.1 |
| S1: class coverage | Coordinator's independent source audit verifies all eleven agent modules call the shared attach path with AGENT and have no override exposing PROFILE_NAME |

Each new unit scenario performs all cleanup before asserting its result, so expected pre-fix failures cannot leak bus/client/device state into subsequent controls. Use existing private-HOME main setup; read only synthetic saved fixture content. Run baseline before production edits, new regressions before the fix, then focused native and applicable sanitizer checks against the final source. No physical mount/camera driver or simulator is needed for the class-level base contract.
