# Claude Code entry point

@AGENTS.md
@.factory/bootstrap.md

These imports keep the shared rules canonical. Explicitly read scoped AGENTS.md files for every assigned source/test/doc path and load the assigned role; do not rely on tool-specific automatic discovery. Check session instruction loading before work. This file grants no additional tool, network, delegation, or publishing permissions.

For drivers under `indigo_drivers/`, also read `indigo_drivers/AGENTS.md`. For automated tests under `indigo_test/`, also read `indigo_test/AGENTS.md`.
