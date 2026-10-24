# Repair selected core audit findings

Owner: Peter Polakovic. Created: 2026-10-24.

The owner requests repair of these fifteen findings:

- FND-20261024121920-3bd2-repository-audit-004: Numeric formatter reports failure for ordinary successful output
- FND-20261024121920-3bd2-repository-audit-005: Identity image stretch divides by a singular midpoint
- FND-20261024121920-3bd2-repository-audit-006: Standard unified I/O handles initialize the wrong fields
- FND-20261024121920-3bd2-repository-audit-007: JSON decoding loses escaped backslashes
- FND-20261024121920-3bd2-repository-audit-008: Mount alignment file loading trusts unchecked counts and rows
- FND-20261024121920-3bd2-repository-audit-009: Profile selection probes the availability of profile zero
- FND-20261024121920-3bd2-repository-audit-012: RGB48 endian conversion swaps only one channel worth of samples
- FND-20261024121920-3bd2-repository-audit-013: Exponent notation changes numeric zero into a nonzero value
- FND-20261024121920-3bd2-repository-audit-014: RGB stretch selects the wrong reference on tied channel totals
- FND-20261024121920-3bd2-repository-audit-033: Identical U-curve minima are all discarded and produce NaN focus
- FND-20261024121920-3bd2-repository-audit-037: DONUTS accepts narrow valid regions that violate its FFT/background window bounds
- FND-20261024121920-3bd2-repository-audit-038: Three-point polar alignment divides by a zero intermediate for solvable geometry
- FND-20261024121920-3bd2-repository-audit-039: Platesolver reports an exposure started after its acceptance wait expires
- FND-20261023101753-bc8d-complete-repository-audit-031: DNS-SD callbacks use undefined fields after a discovery error
- FND-20261023101753-bc8d-complete-repository-audit-032: Fully masked contrast produces NaN that reaches focus estimates

Scope: corresponding core helpers, imager and platesolver callers, focused tests, evidence, canonical finding closure and project registration. Preserve ABI, properties, off-limits drivers and unrelated changes. No hardware, remote publication or master integration authorized.
