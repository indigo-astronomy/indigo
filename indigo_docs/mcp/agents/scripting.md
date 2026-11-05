# Scripting Agent

**DRAFT — source-inspected offline manual; recipes/examples are not runtime-validated by this documentation task; this manual is included in compiled MCP help.**

Module `indigo_agent_scripting`; default device `Scripting Agent`. A persistent Duktape JavaScript environment observes INDIGO properties, submits changes and can publish script-owned properties. Use it to coordinate documented agent workflows, not to bypass Imager/Guider/Mount procedures with guessed device writes. Load through server `DRIVERS` as described in [server/device preparation](../server-and-devices.md). Its normal CONNECTION, CONFIG and PROFILE controls are hidden; it has no separate user-facing “connect script engine” step.

## Agent function inventory

| Function ID | Exact property/item | Semantics |
| --- | --- | --- |
| `scripting.run-inline` | `AGENT_SCRIPTING_RUN_SCRIPT.SCRIPT` | Execute submitted JavaScript source immediately; not a filename. |
| `scripting.add` | `AGENT_SCRIPTING_ADD_SCRIPT.NAME`, `.SCRIPT` | Add a stored script with a nonempty unique display name; source is raw text. |
| `scripting.inspect-edit` | Dynamic `AGENT_SCRIPTING_SCRIPT_n.NAME`, `.SCRIPT` | Read/edit/rename a stored script; IDs are published slot IDs, not the script's display name. |
| `scripting.execute` | `AGENT_SCRIPTING_EXECUTE_SCRIPT.<published-script-property-ID>` | Execute one stored script; the switch item's label is its human name. |
| `scripting.delete` | `AGENT_SCRIPTING_DELETE_SCRIPT.NAME` | Delete by exact stored human name; not by slot ID. |
| `scripting.on-load` | `AGENT_SCRIPTING_ON_LOAD_SCRIPT.<script-ID>` | Any-of-many scripts selected for execution after configuration restoration; also has `AGENT_SCRIPTING_ADD_SCRIPT` (“New script”) as the selection default for newly added scripts. |
| `scripting.on-unload` | `AGENT_SCRIPTING_ON_UNLOAD_SCRIPT.<script-ID>` | Selected scripts run during detach; same new-script default item. |
| `scripting.events` | `indigo_devices`, `indigo_event_handlers`, callback API below | Maintain observed state and react without blocking the engine. |
| `scripting.timers` | `indigo_set_timer`, `indigo_cancel_timer` and time helpers below | One-shot scheduling and cooperative cancellation; no independent calendar/sequence job manager. |
| `scripting.publish` | Native define/redefine/update/delete functions below | Create and maintain script-owned bus properties. |
| `scripting.blobs` | BLOB observation, enable/populate/save functions below | Explicit server-side BLOB fetch/save, distinct from MCP Agent's URL-only BLOB reads. |
| `scripting.stop-recovery` | Script-managed timers/handlers and owning agent abort controls | No generic abort-script property or forced JavaScript interrupt is implemented. |

## Store, edit, run and remove a script

For a harmless first script submit both add items:

```json
{"NAME":"Report readiness","SCRIPT":"indigo_send_message('Readiness script evaluated');"}
```

Use these items on `Scripting Agent.AGENT_SCRIPTING_ADD_SCRIPT`. Wait for `Ok`; discover the resulting `AGENT_SCRIPTING_SCRIPT_n` property and the corresponding execute switch item. On success the add-input fields are cleared. Duplicate/empty names or exceeding128 stored scripts produces `Alert`; adding an existing name is not an ordinary replace operation.

To edit, write `SCRIPT` on that published script property. Rename through its `NAME`; empty/duplicate names are rejected. Renaming changes labels and redefines/sorts execute/on-load/on-unload lists, so refresh definitions; do not identify scripts by list position. Ordinary edits, additions, deletions and hook selections save the agent's configuration automatically. These are filesystem/persistence side effects, even without an explicit CONFIG request. Inline `RUN_SCRIPT` source is a one-off input, not a stored script.

To execute, set the actual script-property-ID switch true on `AGENT_SCRIPTING_EXECUTE_SCRIPT`. Observe `Busy` then `Ok` or `Alert`, plus messages. JavaScript evaluation is synchronous in the agent's protected interpreter; queued bus operations, timer callbacks or hardware processes started by that script may finish later. Thus “script evaluated successfully” is not “observing sequence completed.” An evaluation error can follow earlier side effects; there is no automatic rollback.

Delete by `AGENT_SCRIPTING_DELETE_SCRIPT.NAME = "Report readiness"`; success removes its script property and hook/execute entries and clears the input. Missing names report `Alert`. Deleting stored source does not revoke event handlers or timers already installed into the running global environment. Perform script-specific cleanup first.

## Hook lifecycle and stopping

On-load selection is applied after the configuration restore finishes successfully. On-load scripts can install handlers/timers. At detach, existing agent timers are canceled, selected on-unload scripts are evaluated, then the interpreter is destroyed. Avoid using on-unload hooks to start delayed work expected to outlive the interpreter. Boot helper code executes when the interpreter is created.

There is no `AGENT_ABORT_PROCESS` or generic “stop script” switch here. Write event-driven scripts with an explicit cleanup function: cancel timer handles still pending, remove their entries from `indigo_event_handlers`, and request the documented abort/stop operation from any science agent they started. Do not implement an infinite polling loop or assume unloading will safely preempt blocking JavaScript. A timer callback is also interpreter work; keep it bounded. After an exception inspect messages and actual agent state before rerunning a non-idempotent script.

Example of a bounded, non-hardware timer and explicit cleanup:

```javascript
var demoTimer = indigo_set_timer(function () {
  demoTimer = null;
  indigo_send_message('Timer callback ran');
}, 1);
function stopDemo() {
  if (demoTimer !== null && demoTimer >= 0) {
    indigo_cancel_timer(demoTimer);
    demoTimer = null;
  }
  delete indigo_event_handlers.demo;
}
```

Delay is seconds. Timers are one-shot: repeat by explicitly scheduling again. There are32 native timer slots. A successful schedule returns its numeric handle; scheduling failure can return-1 or raise an interpreter error when no slot is available. Cancel only an outstanding valid handle; invalid/inactive cancellation raises an error. No built-in persistent recurring schedule or ready-made acquisition-sequence DSL is registered by this agent; an Imager capture batch belongs to the [Imager guide](imager.md), and custom script sequencing must wait on actual agent state.

## JavaScript observed-property model

Boot code exposes `indigo_devices[exactDeviceName][exactPropertyName]`. A property object has `device`, `name`, `label`, `items`, `item_defs`, `state`, `perm`, `message` and `change(items)`. `items` is an item-ID/value dictionary; `item_defs` supplies labels/types and numeric format/min/max/step. Updates merge published item values; deletions remove cached objects. Do not keep treating a deleted property's object as a live device.

Register a handler object under a unique `indigo_event_handlers` key. Optional `devices: ["exact name"]` filters define/update/delete/message events. Supported callbacks are `on_define(property)`, `on_update(property)`, `on_delete(property)`, `on_message(text)`, `on_enumerate_properties(request)` and `on_change_property(request)`. Enumerate/change requests are dispatched without the devices filter; inspect their `device`/`name` explicitly. `on_message` receives combined device/message text. Event handlers should inspect the relevant property/state and return promptly.

`property.change(items)` and `indigo_change_property(device,property,items)` select the native text/number/switch change function from the first item's JavaScript type. Use homogeneous nonempty dictionaries. These submit requests rather than await device completion. The `indigo_log_with_property` and `indigo_error_with_property` helpers log full item values; do not use them indiscriminately for sensitive data.

The low-level callbacks installed by boot code can be replaced, but replacing them also replaces the default high-level cache/event behavior. Their actual signatures are:

```javascript
indigo_on_define_property(device, property, label, items, item_defs, state, perm, message)
indigo_on_update_property(device, property, items, state, message)
indigo_on_delete_property(device, property, message)
indigo_on_send_message(device, message)
indigo_on_enumerate_properties(device, property)
indigo_on_change_property(device, property, items, state)
```

Define/update callbacks carry observed number values; change callbacks carry number targets. Light items use state strings; BLOB items are objects containing `url`, `size`, `format`, `reference`. The `reference` is an engine/native item reference, not a serializable filename or a durable handle safe after device/property deletion.

## Complete registered binding inventory

These are script functions, not extra MCP tools. Grouped function IDs below cover every name registered by the agent and the boot helpers. Arguments `device`, `property`, item keys and filenames are exact strings; message can be null where optional. Unless noted, side-effect functions enqueue bus work rather than return an operation-completion result.

| Function ID | Functions and arguments | Semantics |
| --- | --- | --- |
| `scripting.api-log` | `indigo_error(message)`, `indigo_log(message)`, `indigo_debug(message)`, `indigo_trace(message)` | Emit server log messages at the named level; no device operation. |
| `scripting.api-message` | `indigo_send_message(message)` | Queue an INDIGO message from Scripting Agent. |
| `scripting.api-enumerate` | `indigo_enumerate_properties(device,property)` | Request definitions; null/undefined means all devices/properties for that field. Results arrive by callbacks/cache. |
| `scripting.api-change` | `indigo_change_text_property(device,property,items)`, `indigo_change_number_property(...)`, `indigo_change_switch_property(...)` | Queue typed item changes to existing devices/agents. Strings, numbers and booleans respectively; operation state is observed later. |
| `scripting.api-define-text` | `indigo_define_text_property(device,property,group,label,items,item_defs,state,perm,message)` and `indigo_redefine_text_property` with the same arguments | Create script-owned text property; item definitions need label. Redefine deletes/redefines bus schema. |
| `scripting.api-define-number` | `indigo_define_number_property(device,property,group,label,items,item_defs,state,perm,message)` and matching `indigo_redefine_number_property` | Item definitions contain label, format, min, max, step; units are the author's declared property semantics. |
| `scripting.api-define-switch` | `indigo_define_switch_property(device,property,group,label,items,item_defs,state,perm,rule,message)` and matching `indigo_redefine_switch_property` | Boolean items and labels; use canonical rule strings `ONE_OF_MANY`, `AT_MOST_ONE`, `ANY_OF_MANY`. |
| `scripting.api-define-light` | `indigo_define_light_property(device,property,group,label,items,item_defs,state,message)` and matching `indigo_redefine_light_property` | State-string items and labels; light properties have no writable-permission argument. |
| `scripting.api-update` | `indigo_update_text_property(device,property,items,state,message)`, `indigo_update_number_property(...)`, `indigo_update_switch_property(...)`, `indigo_update_light_property(...)` | Update matching already script-defined property/items. These publish script-owned state; they do not command an unrelated hardware driver. |
| `scripting.api-delete` | `indigo_delete_property(device,property,message)` | Remove a matching script-defined cached property; not a request to unload a driver. |
| `scripting.api-blob-enable` | `indigo_enable_blob(device,property,state)` | Boolean true enables URL BLOB delivery; false disables delivery for the scripting client. |
| `scripting.api-blob-populate` | `indigo_populate_blob(blobItem)` | Can fetch the item's HTTP URL when size is zero; updates its size/reference-backed content. Explicit network side effect. |
| `scripting.api-blob-save` | `indigo_save_blob(fileName,blobItem)` | Can populate URL content and create/write the named file on the INDIGO server host. Check logs/destination; it is not a client download API or a general durable BLOB-reference guarantee. |
| `scripting.api-sexagesimal` | `indigo_dtos(value,format)`, `indigo_stod(text)` | Convert native numeric/sexagesimal representation; caller must choose correct degrees-versus-hours units. |
| `scripting.api-altitude` | `indigo_solar_altitude(latitude,longitude)`, `indigo_target_altitude(ra,dec,latitude,longitude)` | Current-time geometric altitude in degrees, elevation argument fixed to0; target RA is hours, declination/site coordinates degrees in native geographical convention. Not a future-time ephemeris or safety clearance. |
| `scripting.api-timer` | `indigo_set_timer(callback,delaySeconds)`, `indigo_cancel_timer(handle)` | One-shot scheduling/cancellation as above; callbacks execute without arguments. |
| `scripting.api-utc` | `indigo_utc_to_time(text)`, `indigo_utc_to_delay(text)` | Parse UTC text to epoch seconds or nonnegative seconds from now. Accepted valid forms: `YYYY-MM-DD HH:MM[:SS]`, `HH:MM[:SS]`. Time-only values choose the next occurrence, rolling to tomorrow when not later today. |
| `scripting.api-time` | `indigo_time_to_delay(epochSeconds)`, `indigo_time_to_utc(epochSeconds)`, `indigo_delay_to_utc(delaySeconds)` | Nonnegative delay, formatted UTC string, or now-plus-delay formatted UTC respectively. Output UTC format is `YYYY-MM-DD HH:MM:SS`. |
| `scripting.api-time-aliases` | `indigo_utc_diff(text)`, `indigo_local_time_diff(epochSeconds)` | Actual aliases of utc_to_delay/time_to_delay. Despite its name, local_time_diff does not parse a local-clock string. |
| `scripting.api-boot` | `indigo_change_property`, `indigo_log_with_property`, `indigo_error_with_property`, `indigo_call_handlers`; globals `indigo_devices`, `indigo_event_handlers`, `indigo_timers` | Boot convenience/cache/event dispatch described above; indigo_timers is internal callback storage, not a persistent schedule database. |

Use state strings `Idle`, `Ok`, `Busy`, `Alert` and permissions `RO`, `RW`, `WO`. Script-defined properties are limited by126 native cache slots; stored scripts and timers have separate limits above. The low-level publish API is for implementing new virtual properties, not an alternative method of changing real devices. When adding a custom driver-style property use the repository's `X_` naming convention.

## Coverage, uncertainty and sources

Inventory is derived from actual property handlers, every `duk_put_global_string` registration and boot.js, not vendor Duktape internals. ECMAScript language-version compatibility, filesystem permission/error behavior of BLOB saving, astronomical edge cases and each user's custom scheduling policy are not runtime-certified by this draft. There is no assertion that an arbitrary script is safe or that no native operation can block. Examples are bounded synthetic source, not executed here.

Sources: [agent implementation](../../../indigo_drivers/agent_scripting/indigo_agent_scripting.c), [boot helpers](../../../indigo_drivers/agent_scripting/boot.js), [header](../../../indigo_drivers/agent_scripting/indigo_agent_scripting.h), [existing scripting notes](../../../indigo_drivers/agent_scripting/README.md), [standard names](../../../indigo_libs/indigo/indigo_names.h), [property reference](../../PROPERTIES.md).

## Optional observing Sequencer

The repository includes an observing-sequence library with prepared agent workflows; see the [Sequencer manual](sequencer.md). It is not automatically evaluated by the Scripting Agent. Its 88,741-byte source exceeds the current 64 KiB MCP request-body limit before JSON escaping, and the interpreter has no file-import binding. Do not attempt a single MCP upload of this library or assume server file presence makes `Sequence` available. Pre-provision it through a suitable native client/configuration before using short sequence expressions. The complete canonical Sequencer reference is available as embedded topic `reference/sequencer.md`; this is documentation, not executable library source. Native library bootstrap remains required.
