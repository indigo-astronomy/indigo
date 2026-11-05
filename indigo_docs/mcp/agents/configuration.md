# Configuration Agent

**DRAFT — source-inspected operating manual; these recipes have not been runtime-validated for this documentation task; this manual is included in compiled MCP help.**

Module `indigo_agent_config`; default device `Configuration Agent`. This agent saves/restores a local INDIGO service arrangement: loaded drivers, selected device profiles, agent device selections and related-agent links. It does not turn a configuration file into an acquisition sequence or replace each driver's own configuration persistence.

First follow [server/device preparation](../server-and-devices.md). Load the exact module item through the local server's `DRIVERS` property and wait for the agent to appear. Discover its current definitions rather than treating display labels as item names. Configuration changes can disconnect selected devices or load/unload drivers; stop science processes through their owning agents first.

## Function inventory

| Function ID | Exact interface | Meaning / result |
| --- | --- | --- |
| `configuration.inspect` | `AGENT_CONFIG_DRIVERS`, `AGENT_CONFIG_PROFILES`, dynamic `AGENT_CONFIG <agent-name>` | Read observed drivers, selected profile identifiers, filter selections and related-agent links. These are RO snapshots, not editable setup forms. |
| `configuration.setup-autosave` | `AGENT_CONFIG_SETUP.AUTOSAVE_DEVICE_CONFIGS` | Boolean; default false. On a service save, request `CONFIG.SAVE` on devices selected by tracked agent filters. |
| `configuration.setup-unload` | `AGENT_CONFIG_SETUP.UNLOAD_UNUSED_DRIVERS` | Boolean; default false. During restore, turn off unused driver switches instead of retaining additional currently loaded drivers. |
| `configuration.device-profiles` | Device `PROFILE.PROFILE_n`, `PROFILE_NAME.NAME_n`, `CONFIG.SAVE` | Setup-only device operations choose/name/save the profile referenced by the service snapshot; inspect the actual driver's definitions. |
| `configuration.save` | `AGENT_CONFIG_SAVE.NAME` | Text name; save the current arrangement to a named service configuration. |
| `configuration.list` | Items of `AGENT_CONFIG_LOAD` | Dynamically scanned names available for load; names are the actual switch item IDs. |
| `configuration.load` | `AGENT_CONFIG_LOAD.<exact-name> = true` | Start asynchronous restore; observe this property and `AGENT_CONFIG_LAST_CONFIG`. |
| `configuration.remove` | `AGENT_CONFIG_REMOVE.NAME` | Delete the named saved service configuration; this is the actual property name, not `AGENT_CONFIG_DELETE`. |
| `configuration.last` | `AGENT_CONFIG_LAST_CONFIG.NAME` and state | Last saved/loaded name and whether its observed arrangement still matches; `Idle` after relevant live changes does not erase the saved file. |

## Prepare profiles and choose save policy

A service snapshot records selected profile identifiers, not a separate copy of every device setting. In setup, select the intended device `PROFILE` item (for example `PROFILE_0` if actually present), configure that device according to its driver manual, and save its settings with `CONFIG.SAVE` if needed. Normal observing remains through Imager/Guider/Mount agents. Profile naming itself may persist device metadata; it is not a harmless temporary label operation.

With autosave false, a service save does not request device-setting saves. With autosave true, selected devices' current profiles can be overwritten. Two differently named service configurations that reference the same device profile share that profile's settings. For independent arrangements choose distinct device profiles first. Autosave is not a promise that every device-side save succeeded: inspect device `CONFIG` states as well as the Configuration Agent result.

Changing either setup switch also persists the Configuration Agent setup. `UNLOAD_UNUSED_DRIVERS=false` retains extra loaded drivers during restore; true deliberately makes the driver selection match the saved arrangement more closely. Do not enable it while unrelated work depends on those drivers.

## Save and inspect a named arrangement

1. Finish agent selections and related-agent links, and wait for their selectors/processes to settle. Read `AGENT_CONFIG_DRIVERS`, `AGENT_CONFIG_PROFILES` and `AGENT_CONFIG <exact-agent-name>` snapshots to confirm what will be recorded.
2. Set `AGENT_CONFIG_SAVE.NAME = "Observatory Night"`. Names must be nonempty and cannot be `.`, `..` or contain slash, backslash or colon. Whitespace is normalized to underscores, so this example becomes `Observatory_Night`.
3. Observe `AGENT_CONFIG_SAVE.state`: `Ok` plus a saved-name message means the agent's file save succeeded; `Alert` means failure. The load-list definition is refreshed and the normalized name becomes an item of `AGENT_CONFIG_LOAD`. `AGENT_CONFIG_LAST_CONFIG.NAME` is updated on success.
4. Reusing a name replaces its service snapshot. Files are in the INDIGO configuration directory, with a server-port-specific suffix for nondefault persistent ports; use the published list rather than constructing filenames.

The save records tracked driver/profile/filter state. It is not a general dump of arbitrary writable properties, images, scripts' runtime variables or ongoing process progress. The dynamic RO `AGENT_CONFIG <agent-name>` entries represent selectors; a related-agent entry can contain a semicolon-separated selection list.

## Load and apply

Select a currently published `AGENT_CONFIG_LOAD` item. Do not send an arbitrary text filename. The agent publishes `Busy` on load and last-config state, first deselects tracked filter devices and related agents, then reads the stored configuration. Restore applies driver selection, device profiles and agent selectors/relations, with readiness checks and finite waits. Driver loading is queued by the server, so successful dispatch or a nonempty last-name field is not completion.

Wait for `AGENT_CONFIG_LOAD` and `AGENT_CONFIG_LAST_CONFIG` to become `Ok`; also inspect server `DRIVERS`, device profile readbacks and agent selector states. A load can connect devices through agent selections and can unload unused drivers when configured. Missing hardware, failed profile selection or selectors that remain busy can yield `Alert` after partial application. It is not a transactional rollback to the previous arrangement.

No dedicated load-abort property is implemented. While loading, new load requests are not started and save/remove requests report an alert. Let the bounded restore complete; inspect messages, fix driver availability/ports/device readiness, and retry the intended configuration. Do not unload the agent as a substitute for stopping an active science process.

## Delete and recover

Set `AGENT_CONFIG_REMOVE.NAME = "Observatory_Night"`, then inspect its state and refreshed load list. Deletion removes that named service file; it does not undo the current live configuration or remove each driver's saved profiles. A nonexistent/invalid/unremovable name produces `Alert`. Deleting the currently named saved configuration clears last-name tracking on success. Back up/choose a different name before an intentional replacement; no undelete command exists here.

## Coverage and sources

Agent-specific control/readback inventory above follows the actual save/load/remove handlers and observers. Common driver lifecycle/profile controls are described in [server/device preparation](../server-and-devices.md). This draft does not promise arbitrary remote-server configuration restoration or per-driver hardware settings beyond the profiles actually published.

Sources: [implementation](../../../indigo_drivers/agent_config/indigo_agent_config.c), [module header](../../../indigo_drivers/agent_config/indigo_agent_config.h), [existing configuration-agent notes](../../../indigo_drivers/agent_config/README.md), [property definitions](../../PROPERTIES.md). No files or live configurations were changed to validate this draft.
