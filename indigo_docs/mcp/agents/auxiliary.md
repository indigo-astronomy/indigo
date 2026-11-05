# Auxiliary Agent

**DRAFT — source-inspected; not runtime-validated in this documentation task; included in compiled MCP help.**

Module `indigo_agent_auxiliary`; default device `Auxiliary Agent`. This is a shared-filter facade for up to four auxiliary-device selections. It provides their published controls under one agent and related Imager links. It does not implement an auxiliary acquisition/start/abort process of its own.

Use [server/device preparation](../server-and-devices.md) to load the agent and required auxiliary drivers through server `DRIVERS`. Configure physical connection details such as serial port, network endpoint and model at the actual device during setup, before selecting it here. Read the device-specific manual for voltages, heater ranges, safety conditions and units; the generic AUX interface does not make all auxiliary devices interchangeable.

## Function inventory

| Function ID | Exact property/items | Semantics |
| --- | --- | --- |
| `auxiliary.select-1` | `FILTER_AUX_1_LIST.<exact-device-name>` / `NONE` | Select or release slot1. |
| `auxiliary.select-2` | `FILTER_AUX_2_LIST.<exact-device-name>` / `NONE` | Select or release slot2. |
| `auxiliary.select-3` | `FILTER_AUX_3_LIST.<exact-device-name>` / `NONE` | Select or release slot3. |
| `auxiliary.select-4` | `FILTER_AUX_4_LIST.<exact-device-name>` / `NONE` | Select or release slot4. |
| `auxiliary.control-selected` | Dynamically published `AUX_1_…` through `AUX_4_…` properties | Read/change the selected device through the filter's translated properties; actual items/types/units come from that device. |
| `auxiliary.relate-imager` | `FILTER_RELATED_AGENT_LIST.<exact-Imager-Agent-name>` | Any-of-many related links; candidate validation accepts Imager Agent names. This is a relation, not another AUX device slot. |
| `auxiliary.relation-symmetry` | `FILTER_FORCE_SYMMETRIC_RELATIONS.ENABLED` / `DISABLED` | Default enabled; control reciprocal related-agent selection through the shared filter. |
| `auxiliary.instances` | `ADDITIONAL_INSTANCES.COUNT` when published on the base instance | Create extra filter-agent instances with separate selectors; discover actual instance names and limits. |

## Select, operate and release a device

1. Enumerate `FILTER_AUX_1_LIST` and select the exact current item for a prepared, available auxiliary device by setting that switch true. Do not invent an item from its GUI label or strip a remote service suffix.
2. Selection becomes `Busy` while the shared filter connects/configures/enumerates the device. It can reject a device already busy/in use. Wait for selector `Ok`, verify the selected item, then enumerate agent properties again. Dynamic device controls may not exist before readiness.
3. Operate the newly published agent-side property, using its exact definition. The filter normally adds `AUX_1_` to a property that does not already have that prefix; INFO is represented with `DEVICE_` in the translated name. Read the actual schema instead of deriving a property name from this rule alone. Item names and value units retain the device's meaning.
4. Observe the translated property's state and value/target readbacks. A successful write is not proof that a heater, cover or power outlet reached its requested physical state. Device alerts/messages remain the evidence for failure.
5. To release the slot, select `NONE` on that slot's list. Changing selections disconnects the previous selected device and clears its proxy properties. This may affect other users of the hardware; release after the owning science workflow has stopped.

Repeat for slots2–4; they are four generic `INDIGO_INTERFACE_AUX` filters, not fixed categories such as heater/weather/flat panel. Avoid assuming that selecting one powerbox creates a new scientific process or that the same device can be independently reserved in multiple slots.

## Relate an Imager Agent

Select an actually offered Imager Agent item in `FILTER_RELATED_AGENT_LIST`, then verify both relation lists if symmetry is enabled. Shared filter code normally requests the reverse relation, except when symmetry is disabled. Related-agent selection lets cooperating agents discover one another; it does not itself start exposures, flats or scripted auxiliary actions. Those actions must be documented by the Imager workflow or an explicitly prepared script.

To remove a relation set that item's switch false. `NONE` belongs to single-device selectors, not this any-of-many relation list. If an expected Imager is missing, load/instantiate it first and re-enumerate; this agent's validation does not offer arbitrary Mount/Guider names as equivalent relations.

## Stop, recovery and side effects

There is no Auxiliary Agent `START_PROCESS`/`ABORT_PROCESS` property in this implementation. Stop an individual selected device's ongoing action through its documented translated abort/stop property, or stop the Imager/script that owns the workflow. Selecting `NONE`, disconnecting or unloading is not a universal safe substitute for a power/cover/motion abort.

For selection `Alert`, inspect the device's connection state and ownership, fix setup/transport problems, then select `NONE` and reselect once it is available. Shared-filter selection can take time; sending repeated selection writes while `Busy` is normally refused. New/dropped device definitions cause proxy properties to appear/disappear, so refresh schemas after reconnect. Additional instances are dynamically created/deleted; do not decrease the count while their selections are in use.

## Coverage and sources

All agent-specific behavior is the four visible selectors, Imager relation validation and shared-filter operation above. Device-specific outlet/heater/environment functions are intentionally not guessed: use the selected device's complete property schema and hardware manual. No per-device auxiliary recipe has been runtime-validated here.

Sources: [agent implementation](../../../indigo_drivers/agent_auxiliary/indigo_agent_auxiliary.c), [header](../../../indigo_drivers/agent_auxiliary/indigo_agent_auxiliary.h), [shared filter](../../../indigo_libs/indigo_filter.c), [standard property names](../../../indigo_libs/indigo/indigo_names.h), [property reference](../../PROPERTIES.md).
