# Specification

- AC-001: indigo_drivers/REVIEW_SWITCH_TARGETS.md is removed from the tree and from indigo.xcodeproj/project.pbxproj; the project file stays valid.
- AC-002: Every item of the file that is not terminal is preserved in .factory/findings/active.json with its TGT ID as alias. Of 117 rows, only TGT-D29 is open; TGT-D25 is terminal (part fixed, part won't fix).
- AC-003: No other file changes. The REFACTOR.md notes that cite the file keep their text; the file stays readable in Git history.
