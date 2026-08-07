# Incremental Review Status

This file indexes folder-level automatic code review notes. Use it to find the review state for a repository area and to avoid re-reviewing commits that were already covered.

## Review Index

| Path | Review File | Last Reviewed Commit | Notes |
| --- | --- | --- | --- |
| `indigo_libs/` | `indigo_libs/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Core library, bus, protocol, timers, base drivers. All recorded findings resolved (see folder file). |
| `indigo_drivers/` | `indigo_drivers/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Portable drivers and agents. Open findings in folder file. |
| `indigo_linux_drivers/` | `indigo_linux_drivers/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Linux-specific drivers. |
| `indigo_mac_drivers/` | `indigo_mac_drivers/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | macOS-specific drivers. |
| `indigo_optional_drivers/` | `indigo_optional_drivers/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Optional drivers with extra dependencies. Open findings in folder file. |
| `indigo_server/` | `indigo_server/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Server executables, runtime behavior, and web resources. All recorded findings resolved (see folder file). |
| `indigo_tools/` | `indigo_tools/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Command-line tools. Open findings recorded. |
| `indigo_tests/` | `indigo_tests/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Legacy/manual compliance scripts. Open findings recorded. |
| `indigo_docs/` | `indigo_docs/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Developer and user documentation. Open findings recorded. |
| `indigo_examples/` | `indigo_examples/REVIEW.md` | `017ba602857378e4aed489c065c76eacae15924c` | Examples and sample clients/drivers. Open findings recorded. |

## Incremental Review Workflow

Only folders listed in the Review Index are eligible for automatic incremental review. If a requested folder is not listed here, add an explicit index row and folder-level `REVIEW.md` before reviewing it.

1. Select the indexed folder being reviewed and read its `REVIEW.md`.
2. Diff only that folder from the recorded commit to the current target commit:

   ```sh
   git diff <last-reviewed-commit>..<target-commit> -- <folder>
   ```

3. Review for correctness, regressions, memory ownership, concurrency, property state transitions, portability, build impact, and missing tests.
4. Record new findings in the folder-level file with stable IDs.
5. Update the folder-level `Last reviewed commit` only after that folder has been reviewed through the target commit.
6. Update this index row to match the folder-level `Last reviewed commit`.

Do not review or mark complete unindexed folders as part of this process. Do not advance a folder's reviewed commit if the review was partial, skipped generated files that changed, or depended on unresolved assumptions.
