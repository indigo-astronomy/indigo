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

The indexes `tyc2-<fov>deg.isix` are built from the Tycho-2 catalogue for fields of view of 1°, 2°, 5°, 10° and 20°:

| Index | Field of view | Size |
| ----- | ----- | ----- |
| TYC2_1 | 1° | 534 MB |
| TYC2_2 | 2° | 354 MB |
| TYC2_5 | 5° | 174 MB |
| TYC2_10 | 10° | 88 MB |
| TYC2_20 | 20° | 32 MB |

The files are kept outside the repository in `$INDIGO_CACHE_BASE/solver`, or in `~/.indigo/solver` when
`INDIGO_CACHE_BASE` is not set. They are managed with the `X_AGENT_SOLVER_INDEX` property: selecting an item downloads
the file from the `solver-indexes-v1` release of the INDIGO repository on GitHub and verifies it, clearing it deletes the
file. Installed indexes are offered in `AGENT_PLATESOLVER_USE_INDEX` and
at least one of them must be selected to solve. Choose an index whose field of view is close to 1.5 x the size of the
frame; when the pixel scale is known, the index fitting the field size is tried first, then the other selected ones.

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

- `AGENT_PLATESOLVER_HINTS.SCALE` (pixel scale in degrees per pixel) orders the indexes to try; without it the pixel scale reported by the related Imager or Guider agent is used, and without either the selected indexes are tried in their order.
- `AGENT_PLATESOLVER_HINTS.DOWNSAMPLE` of 2 or more bins 16 bit mono frames by that factor before the star detection.
- `AGENT_PLATESOLVER_HINTS.EPOCH` of 0 reports the solved centre in JNow, otherwise in J2000.

## Limitations

- Only INDIGO RAW uploads are solved, FITS and JPEG uploads are refused.
- The downsampling hint is applied to 16 bit mono frames only.
- The field of view is limited by the available indexes, from about 0.5° to 15°.

## Testing

- 2026-10-27 13:45 3.0.0.4 mac arm64 simulator 6/6 OK
