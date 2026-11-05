# Proposed task-only acceptance scenarios

**Draft test design, not executed results.** These scenarios validate whether a model can use the embedded manuals to operate agents. They do not replace native driver tests or the existing MCP protocol tests.

Use a disposable real INDIGO server with `indigo_agent_mcp` loaded (the endpoint starts immediately), software simulator modules available in `Server.DRIVERS`, private configuration and no physical drivers/devices. Start required science agents and simulator drivers unloaded where possible. The client must discover and load them using MCP rather than receiving a prepared property-call script. Serial preparation needs a separate controlled simulator/PTY fixture; do not use real host serial hardware just to exercise this test.

The model receives the manuals, live MCP tools and an ordinary task prompt. The evaluator records actual bus operations and result data separately so that a fluent model summary is not treated as execution evidence.

## Scenarios

| Scenario | Example user task | Required evidence |
| --- | --- | --- |
| Single image | “Take one two-second FITS image with the simulated camera and give me its result.” | Model chooses/loads Imager and camera driver, selects camera in Imager, configures acquisition through the agent and starts the documented process. Actual image result is produced; a link is distinguished from model inspection of pixels. |
| Image batch | “Capture five one-second light frames with a one-second delay.” | Agent batch settings and process are used; observed statistics/results match the request. No client loop of direct camera exposures substitutes for the agent process. |
| Autofocus | “Find focus using the simulated camera and focuser.” | Correct Imager device roles and focus method/prerequisites are established; focus process and result are observed. Moving a focuser alone must not be reported as successful autofocus. |
| Guiding | “Calibrate this simulated guide setup and start guiding.” | Guider camera/output roles, calibration and guiding process choices follow its manual. Readbacks/statistics distinguish calibration, active guiding and an error. |
| Dithered acquisition | “Capture a short batch and dither between frames using the guide setup.” | Required directed Imager/Guider relations and process feature settings are configured; dithering and settling are observed without creating circular relations. |
| Pointing | “Use the simulated mount to slew to these supplied J2000 coordinates and track.” | Mount Agent is used with correct units, documented target/start controls and observed coordinate/state outcome. Raw mount writes do not replace the agent workflow. |
| Solve and center | “Center the supplied target using the simulated camera and mount.” | A supported available solver backend and its prerequisites/related agents are used; solving is distinguished from synchronization and centering. If solver indexes/binaries or a verifiable result are absent, the model reports the specific prerequisite instead of fabricating success. |
| Setup restoration | “Restore the named simulator arrangement.” | Configuration Agent load semantics and saved device-profile dependencies are followed; completion/failure and persistent effects are checked. Restoring a setup is not mistaken for starting acquisition. |
| Abort | “Stop the running acquisition/guiding/slew.” | The owning agent's documented abort/stop is used; relevant final states and stopped activity are verified. A closed HTTP connection is not treated as cancellation. |
| Missing or ambiguous prerequisite | “Prepare the serial mount” with an ambiguous model/port, or request an unavailable agent function. | Model identifies the exact missing information/module/help and requests resolution. It does not guess ports/models, invent properties or silently bypass the required agent. |

## Pass conditions

- Every load, setup, selection, configuration and start action is traceable to a documented function and user intent.
- The correct agent remains the normal process owner; other processes and existing selections are not disrupted unnecessarily.
- Values and units are correct, optional capabilities are checked live, and unsupported functions are not guessed from labels.
- Reported success is supported by actual result evidence. Short-state transitions, stale caches, reused URLs and dispatch-only acknowledgements must not produce false success claims.
- Stop/retry/recovery behavior matches the chosen agent. Native persistence and output files are accounted for inside the private test setup.
- No source checkout, external documentation website or Internet access is required to obtain the operating instructions. Backend dependencies such as an external solver's installed executable/indexes remain separate prerequisites.

## Boundaries to measure explicitly

Current MCP snapshots do not expose update revisions/timestamps or an event stream. Where this prevents a client from proving result freshness, the evaluation should record the limitation and require an honest report rather than weaken the success criterion. If more observation support is needed, specify it as a separate MCP change before claiming end-to-end task reliability.

This document describes future manual/model acceptance work. No model, simulator, hardware or backend run is claimed for this draft-documentation task.
