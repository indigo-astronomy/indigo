# Solver agent

INDIGO plate solver agent with a native quad based solver. It matches star quads found in the frame against
Tycho-2 quad indexes and needs no external solver executables.

## Supported devices

N/A

## Supported platforms

This driver is platform independent. The index download needs the `curl` executable.

## License

INDIGO Astronomy open-source license.

## Authors

Peter Polakovic; index management, index optimization and performance optimization by Rumen G. Bogdanovski.

## Use

indigo_server indigo_agent_solver indigo_agent_imager indigo_agent_mount indigo_ccd_... indigo_mount_...

## Status: Under development

## Notes on agent setup

### Index management

The indexes `index-<scale>deg-tyc2.isix` are built from the Tycho-2 catalogue for quad scales of 1°, 2°, 5°, 10° and 20°,
the index `index-00.5deg-gaia.isix` from Gaia DR3 (stars brighter than G = 14) for the quad scale of 0.5°, where Tycho-2
is too shallow. The scale is zero padded in the file and item names, so they sort by it. The scale is the size of the
star marks (quads) in the index, not the field of view: an index of scale n° contains quads of 0.1 n° to 0.8 n°. The
fields column gives the frame sizes for which the index is chosen when all indexes are installed:

| Index | File | Catalogue | Quad scale | Fields | Size |
| ----- | ----- | ----- | ----- | ----- | ----- |
| INDEX_00_5DEG_GAIA | index-00.5deg-gaia.isix | Gaia DR3 | 0.5° | 0.5° to 1° | 911 MB |
| INDEX_01DEG_TYC2 | index-01deg-tyc2.isix | Tycho-2 | 1° | 1° to 2° | 538 MB |
| INDEX_02DEG_TYC2 | index-02deg-tyc2.isix | Tycho-2 | 2° | 2° to 4.5° | 358 MB |
| INDEX_05DEG_TYC2 | index-05deg-tyc2.isix | Tycho-2 | 5° | 4.5° to 10° | 178 MB |
| INDEX_10DEG_TYC2 | index-10deg-tyc2.isix | Tycho-2 | 10° | 10° to 20° | 92 MB |
| INDEX_20DEG_TYC2 | index-20deg-tyc2.isix | Tycho-2 | 20° | over 20° | 36 MB |

The files are kept outside the repository in `$INDIGO_CACHE_BASE/solver`, or in `~/.indigo/solver` when
`INDIGO_CACHE_BASE` is not set. They are managed with the `X_AGENT_SOLVER_INDEX` property: selecting an item downloads
the file from the `index_release_v2` release of the INDIGO repository on GitHub and verifies it, clearing it deletes the
file. Installed indexes are offered in `AGENT_PLATESOLVER_USE_INDEX` and
at least one of them must be selected to solve. Choose the index whose scale is nearest to 0.7 x the size of the
frame (the geometric mean of its width and height); indexes of about 0.4 to 1 x the frame size solve it; when the pixel scale is known, the index fitting the frame size is tried first, then the other selected ones.

The indexes can also be built locally with `indigo_solver_indexer 1 2 5 10 20` from `indigo_tools`. The tool reads the
Tycho-2 catalogue (CDS I/259, files `tyc2.dat.00` to `tyc2.dat.19`) from an `I_259` directory in the current working
directory and writes the index files next to it; copy them into the cache directory described above. The 0.5° index is
built with `indigo_solver_indexer -g <directory> 0.5` from a Gaia DR3 extract: every `*.csv` file of the directory with
the columns `ra, dec, phot_g_mean_mag, pmra, pmdec` of `gaiadr3.gaia_source` for `phot_g_mean_mag < 14`, as returned by
the ESA Gaia archive TAP service (about 16.8 million stars; proper motion is applied from the Gaia epoch 2016.0). Gaia
data: ESA/Gaia/DPAC, CC BY-SA 3.0 IGO.

The index files (version 2) are memory mapped: an index opens at once, only the parts a solve touches are read from the
disk and the memory they occupy is the page cache of the system, so even the 0.5° index solves in about 300 MB of
memory. Index files of the former version 1 are refused and must be downloaded or built again. The format is described
in [Solver Index Format](../../indigo_docs/SOLVER_INDEX_FORMAT.md).

### Agent configuration

1. in Solver Agent > Index management select the indexes fitting the field size produced by the camera
2. in Solver Agent > Plate Solver select the indexes to use and the hints for subsequent plate solving
3. in Solver Agent > Main select the related Imager or Guider agent and optionally the Mount Agent
4. if the Mount Agent is selected, configure also the Sync mode
5. trigger the exposure

### Hints

- `AGENT_PLATESOLVER_HINTS.SCALE` (pixel scale in degrees per pixel) orders the indexes to try: the index fitting the field first, then the other selected ones nearest to the field size first; without it the pixel scale reported by the related Imager or Guider agent is used, and without either the selected indexes are searched stage by stage: the first stage on every index, the largest index first, then the next stage on every index in the opposite order.
- `AGENT_PLATESOLVER_HINTS.RADIUS`, `RA` and `DEC`: with a radius above 0 the catalogue is restricted to the stars within the radius plus the field extent around the position (RA in hours, DEC in degrees, in the hints epoch), which makes solves near a known mount position faster and more reliable in sparse fields; a field outside the radius is not solved. The Mount Agent keeps RA and DEC at the mount position.
- `AGENT_PLATESOLVER_HINTS.PARITY` of 1 or -1 rejects a solution of the other parity; the quad codes of the index serve both parities, so the hint does not shorten the search.
- `AGENT_PLATESOLVER_HINTS.DEPTH` tries the brightest DEPTH stars first and then all detected stars; 0 uses the built-in progression of 40, 100 and all stars.
- `AGENT_PLATESOLVER_HINTS.CPU_LIMIT` (seconds, 0 for none) bounds the solve, counted from the start of the solve including index loading.
- `AGENT_PLATESOLVER_HINTS.DOWNSAMPLE` of 2 or more bins the frame by that factor before the star detection; a Bayer mosaic (RAW with the BAYERPAT appendix or FITS with the BAYERPAT card) is binned at least 2 x 2, an odd factor raised to the next even one.
- `AGENT_PLATESOLVER_HINTS.EPOCH` of 0 reports the solved centre in JNow and reads the position hint in JNow, otherwise in J2000.
- The last used index stays loaded between solves; a solve with the same index does not read the file again.
- The solved message reports the matched stars and the residual of the fit, for example "Solved: 21 of 22 stars matched, rms 0.02"".

### Solution

The WCS angle and parity follow the convention of astrometry.net and of the Astrometry agent: parity 1 is the mirror
image of the sky (east to the right with north up), parity -1 the sky as seen, and the angle is the rotation of the
frame against north of the solution's epoch. The two agents can therefore be exchanged in the Mount agent and polar
alignment workflows.

## Limitations

- INDIGO RAW is the preferred input and is solved as it is; FITS (8, 16 and 32 bit integer, 32 and 64 bit floating point, mono or three planes averaged), JPEG and camera raw files readable by libraw are converted to 16 bit mono first. Colour frames are averaged to mono.
- The field of view is limited by the available indexes, from about 0.3° to 15°.

## Testing

- 2026-10-28 02:11 3.0.0.9 mac arm64 simulator 150/150 OK
