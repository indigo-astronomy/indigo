# Plan

1. Capture existing driver/simulator/transport baseline and known numeric failure. Update driver REFACTOR before code.
2. Change only direction constants and driver version, regenerate. Make numeric-direction scenario mandatory; use numeric simulator profiles for existing lifecycle scenarios. Update transport wire expectations and simulator evidence comments.
3. Run targeted simulator/transport regressions through recorded runner. Build repaired driver in an isolated RPi work area and test public properties, hardware acceptance and observed NYX motor response. Restore hardware state.
4. Update driver/test documentation, finding disposition, run scoped reviews and process checks, archive and commit. No merge/push.
