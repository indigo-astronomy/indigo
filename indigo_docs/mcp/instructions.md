# INDIGO operating instructions for an MCP client

**Source-grounded operating guidance; task recipes are not model/hardware validated by this documentation work.** The MCP agent supplies a short agent-first bootstrap in `initialize.instructions`. Read this complete guide through `get_help({"topic":"instructions.md"})` or `resources/read` for `indigo://help/instructions.md`; included manual links resolve to embedded help.

## Purpose

INDIGO controls astronomical observing and astrophotography equipment. Drivers expose cameras, telescope mounts, focusers, filter wheels, guiding outputs, rotators, domes, GPS units and auxiliary devices. Agents implement higher-level operations: image acquisition, autofocus, guiding, dithering, mount/dome coordination, plate solving, polar alignment and configuration management.

A user's request is an observing task, not a list of bus writes. Translate it into the documented functions of the appropriate agents. Use the [function catalog](agent-catalog.md) and the relevant manual to determine prerequisites, settings, start commands, progress, results and stop behavior.

Loading `indigo_agent_mcp` starts its service immediately; there is no separate MCP Agent CONNECTION toggle. Initial loading uses server startup or an existing native control path. Unloading the module removes this endpoint. This lifecycle does not replace hardware connection/setup through the documented device and agent selectors.

## Mandatory main workflow

1. **Find the required agents.** Discover devices and identify the agent functions needed for the task. If an agent is missing, inspect the correct server's `DRIVERS` property and load its available module. Verify the resulting agent exists. Do not replace a missing agent with an improvised sequence of direct camera or mount operations.
2. **Find the required device drivers.** Discover the physical or simulated devices needed by those agents. Load missing available drivers through the same server. A loaded driver does not prove the hardware exists or is connected.
3. **Prepare device-specific setup.** For a serial device, identify and set the correct port. Set a driver-specific mount model or connection mode where required. This preparation and explicit service/diagnostic tasks are appropriate direct-device operations. Do not guess ports, baud rates, models or device identity.
4. **Select devices in agents.** Use the agent's actual `FILTER_*_LIST` items. Configure directional related-agent links according to the function recipe. Wait for selection/preparation to finish and refresh the agent schema.
5. **Configure the task on the agents.** Read the function recipe and current configuration. Set the requested exposure batch, focus method, guide configuration, target or solver settings on the corresponding agents. Respect dependencies between their operations.
6. **Start processes on the agents.** Use the documented process command. Observe progress and verify the result; dispatch acknowledgement alone is insufficient. Stop/recover through the owning agent's documented controls.

The [preparation manual](server-and-devices.md) supplies the exact loading, port and selector procedure. Agent names shown in examples are illustrative; use the actual instance names returned by this server, including remote suffixes.

## Choosing the workflow

| User intent | Start with |
| --- | --- |
| Capture one image, previews, a batch or focus-bracketing sequence | Imager Agent |
| Autofocus or Bahtinov-assisted focus | Imager Agent with camera and focuser |
| Calibrate a guide setup, guide or dither | Guider Agent |
| Slew, sync, park, track or coordinate mount/dome/rotator behavior | Mount Agent |
| Solve an image, center a target or run supported polar-alignment procedures | One solver backend with the documented related agents |
| Restore or save an observing setup | Configuration Agent |
| Select/proxy auxiliary equipment | Auxiliary Agent |
| Multi-step observing program with loops, recovery and coordinated operations | Scripting Agent with the explicitly provisioned Sequencer library |
| Explicit custom orchestration outside existing process functions | Scripting Agent |
| Configure an ASCOM Alpaca endpoint for another client | Alpaca Agent |

Read the corresponding manual; this table does not replace the complete function recipes. Mount and solver procedures have coordinate-frame and dependency requirements; do not infer them from a bare RA/DEC label.

## What the bus tools do

- `list_devices` discovers bus-visible device and agent names.
- `list_properties(device)` returns current property/item schemas, including WO schemas and property states.
- `get_property(device, property)` returns the latest cached readable property values and state. It does not trigger a new measurement and cannot read WO values.
- `set_property(device, property, items, token?)` dispatches a partial text, number or switch request through the native bus. `items` maps exact item names to typed JSON values. BLOB and light writes are unsupported.

`get_help({"topic":"agents/imager.md"})` reads a compiled manual without operating hardware. Omit `topic` to read `README.md`; use an exact relative topic including `.md` or its full `indigo://help/` URI. Discover all 18 topics with `resources/list`, then read an exact URI through `resources/read`. Unknown help topics produce a tool result with `isError:true`. Help is raw Markdown text, without `structuredContent`, and remains available when the documented agent is unloaded.

Use full exact names. A text value is a JSON string, a number is a finite JSON number and a switch is a boolean. Send only the items being changed. Do not send an entire discovery result back as a write. Number `value` is observed data; `target` can remain different while an operation runs. Read the function's documented units; a numeric display format is not a unit definition.

For `OneOfMany`, select one item; for `AtMostOne`, at most one; for `AnyOfMany`, items are independent. Use the actual selector item name, which may differ from a guessed device display name. Live schemas determine whether a documented optional function or item is available on this instance. They do not explain undocumented function semantics.

## Completion, failures and retries

A successful `set_property` result reports `dispatched: true` and `completion: "not_confirmed"`. Native bus success can include a request that was not accepted by a target or was denied by native token policy. Establish success from the documented resulting values, process state, statistics and result data.

`Busy` indicates work in progress. `Ok` describes the current property state, not necessarily proof that this request caused a new result. `Alert` indicates a problem; inspect the relevant process/selector/device state. `Idle` does not establish successful completion. Some function-specific failure details require more than a single state flag; follow the manual.

Poll at sensible intervals with a task-appropriate deadline. Current MCP snapshots have no event history, revision or timestamp. A short operation can complete between polls, and a BLOB URL may be reused. Do not claim a new exposure, solution or physical arrival solely from an unchanged `Ok` or URL. If evidence is insufficient, say what cannot be established.

After a connection error, inspect current state before repeating a write that can start work. Repeating an exposure, slew, script or configuration operation may have side effects. Use the owning agent's abort/stop command when requested; an HTTP disconnect does not cancel a process. Agents differ in pause/resume behavior.

## Images and other results

BLOB items contain a URL, format and size. Resolve a server-relative URL against the server that produced it. A loopback URL may be accessible only to the local client, not a cloud browser. Receiving a URL does not mean the model has downloaded or inspected the image. Report a link or access limitation when the client cannot fetch it. Current MCP does not upload BLOBs.

Use the function manual to distinguish live image data, locally stored files, agent downloads and solved coordinates/statistics. Downloading and deleting a stored image are different actions. Do not delete or overwrite results as implicit cleanup.

## Scope of control

Normal observing operations belong to agents. Direct device access is reserved for setup, service or an explicit device-level task. Do not issue competing direct commands while an agent owns an operation. Preserve an already configured usable setup unless the requested task requires a change.

Server controls can load/unload modules or stop services. Configuration, profile and script operations can persist data or affect other devices. Execute these according to the requested task and the manual's side-effect description. Do not unload the MCP agent as routine preparation or cleanup: that removes this control endpoint.

Only pass an access token explicitly supplied for this operation; the MCP agent does not automatically inherit process device/master tokens. Do not put tokens or secrets in URLs or summaries. Native bus policy remains authoritative; guidance does not add an authentication layer.

If a requested function, backend prerequisite, driver or required semantic help is unavailable, identify the missing prerequisite. Do not improvise meanings from property names or silently claim a narrower task succeeded. A simulator demonstrates software behavior, not physical hardware performance.
