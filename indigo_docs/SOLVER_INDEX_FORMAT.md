# INDIGO Solver Index Format
Revision: 29.10.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

Co-authored by: **Claude** (Anthropic, Claude Opus 5.5)

## Overview
The index files of the INDIGO Solver agent (`indigo_agent_solver`) and of the `indigo_solver` command line tool hold the
stars of a catalogue and the quads (groups of four stars) built from them. The solver matches the quads it finds in an
image against the quads of an index. The files are written by `indigo_solver_indexer` from `indigo_tools` and are read by
`indigo_solver_load_index()` in `indigo_drivers/agent_solver/indigo_solver.c`.

The files are memory mapped and used as they are: an index opens at once and only the parts a solve touches are read
from the disk. The current format is version 2; files of the former version 1 (packed records read into memory) are
refused.

## Files
An index is named after its quad scale, zero padded to two integer digits so the files sort by it, and its catalogue:

| File | Catalogue | Quad scale |
| ----- | ----- | ----- |
| `index-00.5deg-gaia.isix` | Gaia DR3 (G < 14) | 0.5° |
| `index-01deg-tyc2.isix` | Tycho-2 (VT < 11.5) | 1° |
| `index-02deg-tyc2.isix` | Tycho-2 | 2° |
| `index-05deg-tyc2.isix` | Tycho-2 | 5° |
| `index-10deg-tyc2.isix` | Tycho-2 | 10° |
| `index-20deg-tyc2.isix` | Tycho-2 | 20° |

Scales below 1° come from Gaia DR3, the others from Tycho-2. The scale is the size of the quads in the index, not a field
of view: an index of scale n° holds quads whose stars are about 0.1 n° to 0.8 n° apart. A field is solved best by the
index of about 0.7 times its size.

## Common conventions
All numbers are little endian. Integers are unsigned unless stated otherwise, floating point numbers are IEEE 754 single
precision (`float`).

### Stars
A star is a pair of floats:

| Offset | Type | Field | Meaning |
| ----- | ----- | ----- | ----- |
| 0 | float | ra | right ascension in radians, ICRS |
| 4 | float | dec | declination in radians, ICRS |

The proper motion is applied up to the date the index was built. A star is referred to by its position in the star
section, starting with 0.

### Quads
A quad has four stars P1, P2, P3 and P4, a code and a baseline:

* P1 and P2 are the two stars of the quad farthest apart. The stars are projected on the tangent plane and placed in a
  frame with its origin in P1, the x axis pointing to P2 and the unit length equal to the distance of P1 and P2.
* (x3, y3) and (x4, y4) are P3 and P4 in that frame. P3 is the one with the smaller x (the smaller y when x is equal),
  the frame is mirrored when needed so that y3 ≥ 0 (y4 ≥ 0 when y3 is 0), and of the two directions of the baseline the
  one with the smaller code is used. The code does not change when the stars are moved, rotated or scaled.
* When one of these choices is closer than 0.005 (`INDIGO_SOLVER_CODE_AMBIGUITY`), measurement noise can turn it, so the
  index holds the other variants of the quad as well, up to 8; they are separate quads with the same stars.
* The baseline is the distance of P1 and P2 on the tangent plane in units of 16″ (the projection of the indexer). The
  solver uses only ratios of baselines.

The four star indices of a quad are stored in the order P1, P2, P3, P4; the solver relies on that order.

### Quad hash
The code is quantised to 1/1000 of the baseline (`INDIGO_SOLVER_CODE_QUANTUM`) and packed into a 64 bit hash:

| Bits | Component | Range |
| ----- | ----- | ----- |
| 48 – 63 | hx3 = round(1000 · x3) | 0 to 1000 |
| 32 – 47 | hy3 = round(1000 · y3) | 0 to 1000 |
| 16 – 31 | hx4 = round(1000 · x4) | 0 to 1000 |
| 0 – 15 | hy4 = round(1000 · y4), two's complement | -1000 to 1000 |

The components are clamped to their ranges. The quads of an index are sorted by the hash as an unsigned 64 bit number,
so all quads with the same (hx3, hy3) are next to each other.

## Header
The header is 64 bytes long:

| Offset | Type | Field | Meaning |
| ----- | ----- | ----- | ----- |
| 0 | char[4] | magic | `ISIX` |
| 4 | uint32 | version | 2 |
| 8 | float | fov | quad scale in degrees, as in the file name |
| 12 | uint32 | star_count | number of stars |
| 16 | uint32 | quad_count | number of quads |
| 20 | uint32 | bucket_side | 1001, the number of values of hx3 and of hy3 |
| 24 | uint64 | stars_offset | offset of the stars |
| 32 | uint64 | buckets_offset | offset of the bucket table |
| 40 | uint64 | hashes_offset | offset of the quad hashes |
| 48 | uint64 | baselines_offset | offset of the quad baselines |
| 56 | uint64 | quad_stars_offset | offset of the quad star indices |

## Sections
The offsets are counted from the start of the file. The sections hold one array each; the entry i of the hashes, the
baselines and the star indices belongs to quad i:

| Section | Element | Count | Alignment |
| ----- | ----- | ----- | ----- |
| stars | star (2 × float) | star_count | 4 |
| bucket table | uint32 | bucket_side² + 1 = 1 002 002 | 4 |
| hashes | uint64 | quad_count | 8 |
| baselines | float | quad_count | 4 |
| quad star indices | uint32[4], P1 to P4 | quad_count | 4 |

The quads are sorted by hash. The bucket table gives the range of the quads with the same (hx3, hy3): with
k = hx3 · 1001 + hy3, the quads of bucket k are the quads `buckets[k]` up to, but not including, `buckets[k + 1]`. So
`buckets[0]` is 0, the table never decreases and its last entry is quad_count. The solver looks the quads of an image up
in the table directly; the table spares it a pass over all quads of the index for every solve.

`indigo_solver_indexer` writes the sections in the order of the table above, each at the next multiple of 8 bytes from
the start of the file, with zero bytes in between. A reader must use the offsets of the header and must not rely on that
order. The size of a file written that way is

64 + 8 · star_count (+ padding) + 4 008 008 (+ padding) + 8 · quad_count + 4 · quad_count (+ padding) + 16 · quad_count

bytes; for the Gaia 0.5° index with 3 839 927 stars and 31 293 651 quads it is 910 949 720 bytes.

### Validity
When it maps an index, the solver refuses it unless

* the header is complete, the magic is `ISIX`, the version is 2 and bucket_side is 1001,
* every section lies within the file and is aligned for its elements,
* the bucket table starts with 0, never decreases and ends with quad_count.

The star indices of a quad are checked when the quad is used: a quad referring to a star beyond star_count is skipped.
The data is used in place, so indexes are mapped on little endian hosts only.

## Memory
An index is not copied: its pages belong to the page cache of the system, are read when a solve first touches them and
can be dropped by the system under memory pressure. Matching reads mostly the hashes and the bucket table; the baselines
and the star indices are read only for the quads whose hash matches. With the Gaia 0.5° index, a solve of a 0.5° field
touches about 300 MB of the 911 MB file.
