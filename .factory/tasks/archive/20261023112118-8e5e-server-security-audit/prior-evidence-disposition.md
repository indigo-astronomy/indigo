# Prior evidence disposition

Reviewed historical lead: task 20261023101753-bc8d-complete-repository-audit, proposals/unfinished-defensive-boundaries.json. None of that proposal is applied.

The proposed zero-master-token bus authorization change is rejected as a security correction: indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md explicitly states that access control/locking are not security features and require a configured master token; device tokens have no effect otherwise. The existing source behavior therefore follows the documented contract. The retained test_bus_access.c expectation for the zero-master case cannot establish a defect. No new finding is allocated for it.

Other proposal portions (XML/JSON buffer boundaries and HTTP BLOB input handling) are leads requiring fresh reviewer validation. The JSON harness's former positive cases were not a passing regression suite; no historic run is presented as fresh acceptance evidence. Existing compiled libraries may contain intermediate diagnostic objects and are not source-current runtime evidence without controlled recompilation.

Historical exclusion of server-security work applied to the completed functional audit only. The owner's explicit 2026-10-23 request now authorizes a focused defensive server security review of their own open-source project. Historical reports and their exclusions remain immutable.
