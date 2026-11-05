# MCP Agent

**DRAFT operating manual — implementation inspected; these written recipes are not newly runtime-validated; this manual is included in compiled MCP help.**

Module `indigo_agent_mcp`; device `MCP Agent`; lifecycle behavior below applies to driver version 3.0.0.3. It adapts the existing INDIGO bus to MCP clients. It does not replace Imager, Guider, Mount or solver workflows: use it to inspect those agents and submit their documented commands. Direct hardware properties are appropriate for setup/service tasks described in [server/device preparation](../server-and-devices.md), not an automatic fallback for an absent science agent.

## Function inventory

| Function ID | Interface | Meaning |
| --- | --- | --- |
| `mcp.lifecycle` | Server `DRIVERS.indigo_agent_mcp` when actually listed | Loading starts the MCP service immediately; unloading stops it. Verify `MCP Agent` exists and the endpoint responds. There is no agent CONNECTION toggle. |
| `mcp.list-devices` | `list_devices({})` | Devices with visible properties, including WO schemas. Names preserve service suffixes and Unicode. |
| `mcp.list-properties` | `list_properties({"device":"<exact name>"})` | Property/item schema, type, permission, state, labels; number limits/step/format and switch rule. Lists omit values/targets. |
| `mcp.get-property` | `get_property({"device":"<exact name>","property":"<exact property>"})` | Latest observed visible RO/RW values/state; excludes WO. This is cache observation, not a hardware poll. |
| `mcp.set-property` | `set_property({"device":"…","property":"…","items":{"ITEM":scalar},"token":"optional hex"})` | Partial typed writes to visible RW/WO text/number/switch properties. Unsupported light/BLOB/RO/hidden/missing targets fail explicitly. |
| `mcp.help` | `get_help({"topic":"agents/imager.md"})`, `resources/list`, `resources/read` | Read 18 compiled Markdown manuals/references without agent loading or runtime filesystem/network access. |
| `mcp.status-recovery` | Server driver lifecycle plus tool result/property state | Recover unavailable cache/service, observe real process progress, and avoid unsafe retries. |

## Start and connect a client

Load the module through server `DRIVERS` using an existing native control path, or start the server with `indigo_server indigo_agent_mcp`. Loading starts the MCP service immediately; no separate agent CONNECTION property or connect step is exposed. Unloading the module stops the service and removes this control endpoint. On the default server port the URL is `http://127.0.0.1:7624/mcp`; substitute the actual server address/port. The same existing listener handles other INDIGO resources; the localhost example does not restrict its bind address.

Configure an MCP HTTP server entry in the client. Codex CLI supports that URL; Claude Desktop's local Code mode can use project `.mcp.json` with an HTTP entry. Desktop Chat local-process integration is a separate transport setup; this agent supplies no stdio bridge. A cloud remote connector cannot use this computer's loopback address. The existing [agent README](../../../indigo_drivers/agent_mcp/README.md) records the client versions and bounded simulator checks, not universal version compatibility.

## Discover before acting

Call `list_devices`, select an actual agent name and call `list_properties`. Use exact property/item IDs, types, units and prerequisites from that agent's manual. Read `get_property` for readable values and current states. The four bus tools include JSON in both text content and structuredContent; tool execution errors can be returned with `isError:true`, while malformed/unknown RPC calls use protocol errors. Successful transport is not success of the requested INDIGO operation.

Hidden properties are absent. WO property/item schemas are listed to make writes discoverable, but WO values are neither cached nor returned; `get_property` fails for them. Readable password/token text is deliberately not removed by a name heuristic. Numeric values and targets are distinct; nonfinite observed numbers serialize as JSON null. Dynamic text/results have no arbitrary application output/cache cap; allocation or serialization/cache failures return explicit errors rather than a truncated-success result.

BLOB read items expose `url`, `format`, `size`, never embedded/base64 data. Local `/blob/…` URLs are relative to the same server; external URLs remain external data. A non-Ok BLOB state does not provide a usable URL. This agent does not fetch payloads as a side effect of a read, and `set_property` cannot upload a BLOB by supplying a URL. Fetching a URL is a separate client action subject to that resource's native behavior.

## Submit and verify a partial change

Each `items` value must match the property's type: JSON string for text, finite JSON number for number, boolean for switch. Empty/unknown/duplicate item sets, extra arguments and wrong types fail before dispatch. Names can be escaped Unicode but still identify exact items. Numeric ranges, steps, switch rules and operational conflicts remain framework/device responsibilities; do not infer that an MCP syntactic success authorizes an otherwise invalid operation.

For example, after discovering an available auxiliary device in the Auxiliary Agent selector, submit:

```json
{"device":"Auxiliary Agent","property":"FILTER_AUX_1_LIST","items":{"<exact discovered auxiliary name>":true}}
```

Replace the placeholder with the actual offered item. Observe the selector and agent-side device definitions before issuing further controls. For an acquisition, use the [Imager guide](imager.md) rather than directly changing camera exposure properties.

A successful set result is `{"dispatched":true,"completion":"not_confirmed"}`. It means the native bus dispatch call returned, not that the device accepted its token, started, completed, or even remained available. Poll the owning agent's documented state/result properties; for WO operations use observable state/schema or another documented result, not a nonexistent readable WO value. Stop/abort through the owning workflow's explicit abort command. Retrying a non-idempotent action can repeat it.

Optional `token` is a1–16-digit hexadecimal string. Omission means zero; device/master tokens are not inherited automatically. Native bus rules decide acceptance, including the existing case where an unset master token equals zero. There is no additional MCP authentication or target denylist: loading the agent exposes the chosen read/write scope through the existing listener, including server and agent controls, to accepted reachable clients. Origin-bearing requests are rejected, but that is not user authentication. Native opt-in bus trace logging may include submitted values/tokens.

## Protocol limits and recovery

Implemented protocol revision is `2025-11-25`: initialize, initialized notification, ping, tools/list, tools/call, resources/list and resources/read. Initialization includes short agent-first operating instructions. Exact raw path `/mcp`; POST JSON responses; connections close after each response. GET returns405; no SSE stream, issued session ID or prompts are implemented. Header/media/framing/version errors and malformed JSON have explicit failures. Inbound body limit64KiB,256JSON tokens, depth16; eight admitted requests; body progress deadline5seconds and total read deadline15seconds. These are input/read limits, not an overall deadline for a legacy blocking device callback.

Admission capacity, transient shutdown or request-allocation failure can return 503; this is not a normal disabled-until-connected mode. Fix the cause and retry reads deliberately. A sticky unavailable-cache error requests agent reload; first stop active work and use native server controls to unload/reload. Loading starts the service again without a CONNECTION request. Same-thread synchronous self-unload during a bus dispatch returns native BUSY before destroying agent state; external unload drains HTTP callbacks. Do not assume a write was undone because its HTTP response was lost. Read state before choosing a recovery action.

## Embedded help and coverage

Initialization provides a short agent-first bootstrap. `resources/list` discovers 18 Markdown resources named `indigo://help/<topic>`; `resources/read` accepts their exact URI. `get_help` is a read-only alternative: omit its optional string `topic` for `README.md`, or pass an exact relative topic including `.md` or its full URI. For example, `get_help({"topic":"agents/imager.md"})` reads the Imager manual; `get_help({"topic":"indigo://help/agents/imager.md"})` is equivalent. Unknown topics return a tool error with `isError:true`.

Help is raw Markdown text in content, without `structuredContent`. It is compiled into the binary: no runtime documentation directory or Internet request is needed, and manuals are available even when their agents are unloaded. Included relative manual links become embedded URIs during generation, retaining anchors. Links outside the bundle are optional source/provenance references, not help resources. The canonical Sequencer reference is topic `reference/sequencer.md`; its executable JavaScript is not installed/evaluated, and its known transit-offset units conflict remains explicit in the Sequencer guide.

All four bus tools, help retrieval, load/unload lifecycle and protocol limits are covered here. No new live client/hardware operation was run for this draft. Sources: [agent C](../../../indigo_drivers/agent_mcp/indigo_agent_mcp.c), [header](../../../indigo_drivers/agent_mcp/indigo_agent_mcp.h), [property reference](../../PROPERTIES.md), [native bus](../../../indigo_libs/indigo_bus.c).

Help links may include a `#section` fragment. `get_help` and `resources/read` resolve the whitelisted base topic/URI and return the complete manual; section navigation is client-side. Fragments do not enable arbitrary file or URL access.

Resource requests accept the standard optional `_meta` object. No progress notifications, subscriptions or resource-list change notifications are advertised; all 18 entries fit in one response with no continuation cursor.
