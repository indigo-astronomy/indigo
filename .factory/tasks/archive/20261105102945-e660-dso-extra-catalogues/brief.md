# Task brief

- Task ID: 20261105102945-e660-dso-extra-catalogues
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-05

## Problem and desired outcome
indigocat holds almost only NGC and IC objects (12082 of 12085 entries), so many common imaging targets have no entry. Add the Sharpless, Barnard, van den Bergh, Abell planetary nebula, Collinder and Melotte catalogues and report the size change of the library.

## Users and use cases
Consumers of `indigocat_get_dso_data()`, in particular the object overlay of Ain Platesolver.

## Scope and non-goals
In scope: new catalogue entries, a `DARK_NEBULA` type and its description. Non-goals: position angles or corrected sizes of existing entries, aliases in existing entries, LBN, LDN, PGC or UGC.

## Constraints and references
Preserve the public ABI and the sky map of the server. Values come from published catalogues only. Sources: CDS VizieR VII/20, VII/220A, VII/21 (Harvard mirror) and SIMBAD TAP (Harvard mirror).

## Initial questions
None open. The owner chose priority 1 and 2 catalogues and asked for the library size difference.
