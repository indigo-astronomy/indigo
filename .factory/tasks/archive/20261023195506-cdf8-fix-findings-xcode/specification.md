# Specification

Every active/archive finding JSON has exactly one navigator reference in its matching group. Archival validates a proposed closed record, preserves its ID, updates both register and project, and rolls back on failure. Creation updates project references. Non-Xcode repositories continue to work. Unsupported/shared project structures fail before mutation. Unrelated PBX objects and build membership remain unchanged.

Reconcile all tracked Factory navigator files using inventory only; preserve unrelated application objects. Propagate shared behavior/tests/docs to the explicitly authorized AI Factory task.
