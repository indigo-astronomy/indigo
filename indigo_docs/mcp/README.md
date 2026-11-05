# Embedded MCP operating manuals

These operating manuals are compiled into the MCP agent and available without runtime documentation files, Internet access or a GitHub checkout. The recipes remain source-grounded drafts with explicit semantic gaps; inclusion in help does not claim model, simulator or hardware acceptance of every workflow.

The MCP service starts as soon as `indigo_agent_mcp` is loaded and stops when it is unloaded. No separate MCP Agent CONNECTION step is needed.

## Start here

1. [Model instructions](instructions.md): what INDIGO does, the agent-first execution policy and the complete task lifecycle.
2. [Server and device preparation](server-and-devices.md): load modules through `Server.DRIVERS`, configure serial ports/model-specific settings, and select devices in agents.
3. [Agent function catalog](agent-catalog.md): choose the agent and find the exact function recipe.
4. Read that agent's manual before planning its operations. Use live schemas to check availability and current values, not to guess function semantics.

The manuals cover the in-tree agent modules, including the distinct native Platesolver and external Astrometry backends, protocol bridges and the development Test Agent. Hardware-driver-specific service commands remain outside this manual set; a model must report a missing service recipe instead of inventing one from an `X_` label.

## Documents

| Agent | Semantic manual |
| --- | --- |
| Imager | [Acquisition, previews, focusing and files](agents/imager.md) |
| Guider | [Calibration, guiding, dithering and diagnostics](agents/guider.md) |
| Mount | [Mount/dome/rotator orchestration and site data](agents/mount.md) |
| Platesolver | [Native solver and common solve/center/alignment workflows](agents/platesolver.md) |
| Astrometry | [External Astrometry backend and index setup](agents/astrometry.md) |
| Configuration | [Named configurations and restoration](agents/configuration.md) |
| Auxiliary | [Auxiliary-device selection and forwarding](agents/auxiliary.md) |
| Scripting | [Scripts, custom orchestration and the scripting interface](agents/scripting.md) |
| Optional Sequencer library | [Observing sequences through Scripting](agents/sequencer.md) |
| Alpaca | [Alpaca service configuration and published interfaces](agents/alpaca.md) |
| MCP | [MCP transport, tools and native control boundary](agents/mcp.md) |
| Test | [Development-only handler-queue exercises](agents/test.md) |

Each manual includes a function inventory, exact property/item mappings, prerequisites, execution and observation rules, failure/stop behavior and source references. Function IDs are documentation identifiers, not additional MCP tools or executable property names.

## Read the embedded help

`initialize.instructions` provides a short agent-first bootstrap and directs clients to the manuals. Use either MCP resources or the read-only `get_help` tool:

- `resources/list` discovers 18 compiled Markdown resources with exact URIs `indigo://help/<topic>`.
- `resources/read` with `{"uri":"indigo://help/agents/imager.md"}` returns that manual.
- `get_help({})` returns this index, topic `README.md`.
- `get_help({"topic":"agents/imager.md"})` or `get_help({"topic":"indigo://help/agents/imager.md"})` returns the same Imager manual. Topic names are exact and include `.md`; unknown topics return `isError:true` rather than guessing a match.

`get_help` returns raw Markdown text in content, without `structuredContent`. Help reads do not load an agent, access runtime files, contact the Internet or operate hardware. All manuals remain discoverable when their agents are unloaded. The original four bus tools retain their existing contracts.

The 18 topics are the 17 Markdown pages in this manual directory tree and the complete canonical Sequencer reference at `reference/sequencer.md`. Its URI is `indigo://help/reference/sequencer.md`. That reference is documentation only: the executable Sequencer library is not embedded/evaluated. Its native bootstrap prerequisite and known transit-offset unit discrepancy remain in the [Sequencer guide](agents/sequencer.md).

Included relative Markdown links are rewritten during generation to embedded URIs, preserving anchors. Source/property/provenance links outside the bundle are optional review references; they are not additional runtime help resources or prerequisites for using the manuals.

## Maintain the compiled bundle

When an agent function or manual changes, update the relevant manual and regenerate the checked-in help header from the repository root:

```sh
python3 indigo_drivers/agent_mcp/generate_help.py
python3 indigo_drivers/agent_mcp/generate_help.py --check
```

The generator consumes these Markdown pages and the canonical Sequencer reference. The checked-in generated header supports normal Make, Xcode and Windows builds without requiring Python at runtime. `--check` checks that generated content matches the current inputs; it does not validate the astronomical recipes or prove client behavior.

## Review and acceptance of the manuals

- Verify each function against its implementation, including the shared agent filter and solver layers.
- Check all public command branches, process variants and configuration controls against the function inventory. A list of property labels is not a coverage argument.
- State remaining semantic gaps explicitly; do not label a partially described function as fully documented.
- Test later with task-only prompts on real software simulators: the client should discover/load agents and drivers, prepare/select devices, configure and run agents, then prove the result. Do not supply the expected property-call sequence in the test prompt.
- Include representative imaging, focus, guiding, mount and solving tasks and failure/abort paths. Successful tool transport alone does not validate these manuals.

Concrete task prompts and evidence criteria are in [acceptance scenarios](acceptance-scenarios.md).

The operating recipes have source-reading and documentation checks; no model acceptance of these recipes is claimed here. Protocol/help implementation checks are separate evidence.

## Relationship to existing references

[PROPERTIES.md](../PROPERTIES.md) remains the authoritative property-name reference; the driver and agent source determines behavior. These manuals add the task semantics and agent workflows needed by a model. They should not copy the entire property reference or require it to be installed alongside the server. Facts required for the documented workflows are included in the embedded manuals; explicitly identified gaps still require resolution before those functions can be inferred or certified.

Help links may include a `#section` fragment. `get_help` and `resources/read` resolve the whitelisted base topic/URI and return the complete manual; section navigation is client-side. Fragments do not enable arbitrary file or URL access.
