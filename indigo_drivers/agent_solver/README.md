# Solver agent

INDIGO plate solver agent with a native quad based solver. It matches star quads found in the frame against
Tycho-2 quad indexes and needs no external solver executables.

## Supported devices

N/A

## Supported platforms

This driver is platform independent. The index download needs the `curl` executable.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_solver indigo_agent_imager indigo_agent_mount indigo_ccd_... indigo_mount_...

## Status: Under development

## Notes on agent setup

### Index management

The indexes `tyc2-<scale>deg.isix` are built from the Tycho-2 catalogue for quad scales of 1°, 2°, 5°, 10° and 20°. The
scale is the size of the star marks (quads) in the index, not the field of view: an index of scale n° contains quads of
0.1 n° to 0.8 n°:

| Index | Quad scale | Size |
| ----- | ----- | ----- |
| TYC2_1 | 1° | 534 MB |
| TYC2_2 | 2° | 354 MB |
| TYC2_5 | 5° | 174 MB |
| TYC2_10 | 10° | 88 MB |
| TYC2_20 | 20° | 32 MB |

The files are kept outside the repository in `$INDIGO_CACHE_BASE/solver`, or in `~/.indigo/solver` when
`INDIGO_CACHE_BASE` is not set. They are managed with the `X_AGENT_SOLVER_INDEX` property: selecting an item downloads
the file from the `index_release_v1` release of the INDIGO repository on GitHub and verifies it, clearing it deletes the
file. Installed indexes are offered in `AGENT_PLATESOLVER_USE_INDEX` and
at least one of them must be selected to solve. Choose the largest index whose scale is at most 1.5 x the size of the
frame; when the pixel scale is known, the index fitting the frame size is tried first, then the other selected ones.

The indexes can also be built locally with `indigo_solver_indexer 1 2 5 10 20` from `indigo_tools`. The tool reads the
Tycho-2 catalogue (CDS I/259, files `tyc2.dat.00` to `tyc2.dat.19`) from an `I_259` directory in the current working
directory and writes the index files next to it; copy them into the cache directory described above.

### Agent configuration

1. in Solver Agent > Index management select the indexes fitting the field size produced by the camera
2. in Solver Agent > Plate Solver select the indexes to use and the hints for subsequent plate solving
3. in Solver Agent > Main select the related Imager or Guider agent and optionally the Mount Agent
4. if the Mount Agent is selected, configure also the Sync mode
5. trigger the exposure

### Hints

- `AGENT_PLATESOLVER_HINTS.SCALE` (pixel scale in degrees per pixel) orders the indexes to try: the index fitting the field first, then the other selected ones nearest to the field size first; without it the pixel scale reported by the related Imager or Guider agent is used, and without either the selected indexes are tried in their order.
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
- The field of view is limited by the available indexes, from about 0.5° to 15°.

## Testing

- 2026-10-27 20:37 3.0.0.8 mac arm64 simulator 149/149 OK
