# Plan

1. Download the published catalogues and SIMBAD identifiers.
2. Generate rows with `generate_dso_extra.py`, kept with this task. It is not part of the product build.
3. Review merges and compare positions between sources.
4. Append the rows before the terminator, and append the type and description.
5. Measure sizes, validate the table, and test with Ain Platesolver.

Affected components: `indigo_libs/indigocat/indigocat_dso.c`, `indigo_libs/indigo/indigocat/indigocat_dso.h`. Reverse by reverting the commit; no data migration.
