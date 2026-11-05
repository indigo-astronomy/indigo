# Server and device preparation

**Draft semantic manual.** Applies to the MCP bus tools and embedded help interface. The normal execution target after preparation is an agent.

## Preparation functions

| ID | Function | Recipe |
| --- | --- | --- |
| setup.discover | Identify servers, loaded agents and devices | [Discovery](#discovery) |
| setup.load-agent | Load a missing available agent | [Load through DRIVERS](#load-through-drivers) |
| setup.load-driver | Load a missing available hardware/simulator driver | [Load through DRIVERS](#load-through-drivers) |
| setup.serial | Select the correct serial endpoint | [Serial ports](#serial-ports) |
| setup.model | Configure driver-specific connection/model prerequisites | [Model and connection setup](#model-and-connection-setup) |
| setup.select | Select a controlled device and await initialization | [Device selection](#device-selection) |
| setup.relate | Establish the function's required agent relations | [Agent relations](#agent-relations) |
| setup.configure | Use profiles/configuration deliberately | [Configuration side effects](#configuration-side-effects) |
| setup.unload | Release an idle module when actually required | [Unloading](#unloading) |

## Discovery

Call `list_devices`, then read `INFO` on candidate devices/agents and inspect their schemas. Use `INFO.DEVICE_DRIVER` to distinguish an instance from its module. One loaded module may create multiple device objects and may support several interfaces. Display names and serial paths alone are not proof of hardware identity.

Find the server instance associated with the relevant devices. Names can have remote service suffixes. Use the exact returned name; do not assume every host is the unsuffixed `Server` or load a driver on an unrelated host.

Preconditions for preparation are a loaded MCP Agent with a reachable endpoint and access allowed by the native server/bus. Loading `indigo_agent_mcp` starts its service immediately; there is no separate MCP Agent CONNECTION step. Initial module loading must use server startup or an existing native control path, since an unloaded MCP endpoint cannot load itself. This guide does not install a driver binary or attach nonexistent hardware. If the needed module is absent from the server's available-driver list, report that installation/build support is missing.

## Load through DRIVERS

1. Read the relevant server's `DRIVERS` property using `get_property`. It is an `AnyOfMany` switch vector whose item names are driver module identifiers, such as `indigo_agent_imager` and `indigo_ccd_simulator`.
2. If a required item is already true, reuse it; inspect the actual devices it created. If false, send a partial switch request containing only required modules to enable.
3. The server queues driver operations, publishes `Busy` and eventually publishes its final selection. Wait for completion, then verify each requested item is true and discover the expected agent/device objects.
4. Do not infer load success from MCP dispatch or the vector's final `Ok` alone. Failed initialization can leave an item false. A loaded driver may still expose no suitable physical device.
5. Load missing hardware drivers after determining the appropriate agent/device roles. Continue with preparation and agent selectors rather than directly starting a low-level acquisition or slew.

Example arguments to `set_property`, only after those item names were found in the live server schema:

```json
{
  "device": "Server",
  "property": "DRIVERS",
  "items": {
    "indigo_agent_imager": true,
    "indigo_ccd_simulator": true
  }
}
```

This does not turn other driver items off. It also does not connect the camera or start an exposure. Some solver backends are mutually exclusive; follow the chosen backend's manual before loading it.

**Results:** requested driver switches and newly visible agent/device names. **Failure:** false selection/missing objects, inaccessible module, initialization error or native-policy denial. **Recovery:** inspect current selection and prerequisites; do not repeatedly toggle a module or unload working dependencies. There is no generic cancellation command for an already queued driver-load request.

## Serial ports

Configure serial devices before selecting them in an agent when the driver requires a port first.

1. Inspect `DEVICE_PORT`, `DEVICE_PORTS` and any visible connection-mode/baud/model properties on the actual device. Some drivers enumerate multiple slots; identify the intended device instance.
2. If `DEVICE_PORTS` is exposed, read its available endpoint items. Select the endpoint that matches the requested hardware, or set `DEVICE_PORT.PORT` to the correctly identified endpoint. Selecting a `DEVICE_PORTS` item copies its name into `DEVICE_PORT`.
3. A path such as `/dev/cu.usbserial-EXAMPLE` is illustrative, never an instruction to use a fabricated port. Obtain ambiguous identity/port information from the user or the relevant driver documentation.
4. Set baud/framing or connection mode only when required and known for this hardware. Do not infer the protocol from an OS port suffix.
5. Read back configuration, then select the device in its agent. Connection/preparation outcome is verified there.

`DEVICE_PORT` may also represent a driver-supported network endpoint; use that driver's supported syntax. Do not replace an existing working endpoint merely because another port is listed. Multiple logical devices can share one physical connection; follow the driver's documented setup ownership.

**Results:** correct port/model values and successful agent selection. **Failure/recovery:** an unavailable/busy/wrong port can fail preparation; release the failed selection and correct the setup before retrying. Port changes can affect subsequent connections and saved configuration.

## Model and connection setup

Some drivers support several controller models or transports. Their setup properties are not universal. For example, the LX200 family exposes `X_MOUNT_TYPE`; its actual available items select protocol-specific behavior. Choose the matching item from the live schema using the hardware model and the driver's semantic documentation. Do not assume a generic `MOUNT_TYPE` property exists on every mount.

A missing setup recipe is a documentation gap. Property labels are insufficient evidence to choose a controller protocol. Do not attempt arbitrary combinations until a device responds. A required setup that is not writable in the current state may require an orderly disconnect/reselection according to the driver manual.

## Device selection

Use the agent's selector for the role: for example, Imager `FILTER_CCD_LIST`/`FILTER_FOCUSER_LIST`, Guider `FILTER_CCD_LIST`/`FILTER_GUIDER_LIST`, or Mount `FILTER_MOUNT_LIST`. Read the selector itself to obtain exact item names and current selection.

Example, only if the returned selector contains this item:

```json
{
  "device": "Imager Agent",
  "property": "FILTER_CCD_LIST",
  "items": {"CCD Imager Simulator": true}
}
```

The shared filter prepares a selected controlled device, including connection, `CONFIG.LOAD` when available, and enumeration. Its selector can remain `Busy` during preparation; do not start a process merely because the desired switch became true. Wait for the selector's final `Ok`, verify the intended selection, and refresh the agent's schema. Device properties can be forwarded onto the agent only after preparation is complete.

On failure the selection can clear and become `Alert`; correct the dependency and select again. Selecting the actual no-device item cancels preparation. Use the returned item identifier rather than assuming the literal display label. Preparation has its own timeout; do not add competing direct CONNECTION requests while it is running.

Different agents can expose similar forwarded properties. Operate through the agent that owns the requested process and do not concurrently issue conflicting commands on the underlying device.

## Agent relations

Related agents coordinate independent higher-level functions: an Imager may request dithering from a Guider, or a solver may use related Imager/Guider/Mount agents. `FILTER_RELATED_AGENT_LIST` is an `AnyOfMany` selector, not the same as a controlled-device selector.

Follow the recipe for which agent points to which other agent, which instance is used when several are selected, and when reciprocal links are appropriate. Do not select every available agent or construct cycles speculatively. Controlled-device preparation and related-agent selection have different completion behavior; the controlled-device connection/CONFIG wait must not be assumed for every relation.

## Configuration side effects

Do not send `CONFIG.LOAD`, `CONFIG.SAVE`, `CONFIG.REMOVE`, profile selection, or Configuration Agent restore operations as an automatic prerequisite for every task. They can replace a working setup and affect other processes. Controlled-device preparation already handles its documented restoration phase.

Native `CONFIG.LOAD` is asynchronous and waits for restored requests to complete. Successful dispatch does not establish restoration success. Follow the relevant device/configuration-agent recipe and inspect the final state.

Agents hide `PROFILE_NAME`; do not attempt to rename agent profiles. Regular devices with profile support still expose it. Some changes persist automatically: their base handler for `PROFILE_NAME` saves profile names. A client making no explicit `CONFIG.SAVE` call has not thereby guaranteed zero filesystem writes. [Configuration Agent](agents/configuration.md) describes named setup management separately from a single device's profile controls.

## Unloading

Setting a `DRIVERS` item false requests its shutdown/unload. A busy/in-use module can refuse and remain true. Stop its agent processes and release its users according to their manuals before unloading when the task actually requires it. Unloading can remove multiple devices and invalidate dependent selections/results.

Do not unload unrelated modules as routine cleanup, switch solver backends during a solve, or unload `indigo_agent_mcp` while relying on this MCP connection. The latter removes the control endpoint. Observe the final driver selection and device disappearance rather than assuming a dispatched false succeeded.

## Source references for this draft

- [Server registry and queued loading](../../indigo_server/indigo_server.c)
- [Property manipulation and DRIVERS examples](../PROPERTY_MANIPULATION.md)
- [Common/selector property names](../PROPERTIES.md)
- [Shared agent filter](../../indigo_libs/indigo_filter.c)
- [Device configuration and profile handlers](../../indigo_libs/indigo_driver.c)
- [LX200 model-selection reference](../PROPERTIES.md#mount_lx200)

These are optional repository review links outside the help bundle. The embedded help supplies the operating semantics without requiring local source files or GitHub access.
