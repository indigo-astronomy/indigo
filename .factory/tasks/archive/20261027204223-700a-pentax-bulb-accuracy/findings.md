# Finding: physical K-m bulb accuracy

FND-20261027204223-700a-pentax-bulb-accuracy-001: Requested1.2s/3s exposures report1.09s/2.88s in JPEG EXIF (−110/−120ms). Both fail the diagnostic±50ms criterion. Registered RAW1.2s reports1.100s through LibRaw. The production software command deadline regression passes, but this does not prove physical curtain duration. Cause/calibration is unresolved; no fixed offset has been added.

Open, medium functional finding, owned by Pentax maintainers. Next action: independently measure shutter/command timing and define safe model-specific correction if required. Not blocking delivery of diagnostic results and accurate documentation, which is this task's scope; not accepted as correct timing or a fixed issue. README discloses the limitation.
