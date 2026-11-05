# Specification

## Behavior
- AC1: `indigocat_get_dso_data()` also returns Sharpless (`Sh2-N`), Barnard (`BN`), van den Bergh (`vdBN`), Abell planetary nebula (`AbellN`), Collinder (`CrN`) and Melotte (`MelN`) objects with J2000 RA in hours and Dec in degrees.
- AC2: An object already in the catalogue is not added again. Identity is a shared SIMBAD object whose position agrees, or a close position, a similar size and the same broad class.
- AC3: Existing entries keep all their values, and all entry ids are unique.
- AC4: `DARK_NEBULA` is appended to the enum and `indigocat_dso_type_description[]` has a matching last element, so existing type values keep their meaning.
- AC5: The server sky map input does not change. New entries have an empty name, which its filter excludes.
- AC6: Sizes are single published diameters with r1 = r2 and angle 0. An unknown size is 0.
- AC7: The size change of the library is measured and reported.

## Failure behavior and assumptions
The table is static data with no new runtime failure path. Published positions may be coarse. Collinder and Melotte coverage is limited to what SIMBAD resolves as numbered objects.
