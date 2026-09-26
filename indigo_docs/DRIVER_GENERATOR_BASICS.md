# INDIGO Driver Generator — Writing a New Driver

Revision: 26.09.2026 (draft)

Author: **Peter Polakovic**

e-mail: *peter.polakovic@cloudmakers.eu*

Co-authored by: **Claude** (Anthropic, Claude Opus 5.5)

## Overview

This document describes how to write a **new** INDIGO driver from scratch with the driver generator (`indigo_generator`). It covers one direction only: from the `.driver` definition, written by hand or with the [driver editor](../indigo_tools/driver_editor/README.md), to the generated `.c`, `.h` and `_main.c`. It explains the `.driver` definition language as it is implemented in [indigo_tools/indigo_generator.c](../indigo_tools/indigo_generator.c), the shape of the C code the generator emits, which parts a driver author owns, and the rules that keep a generated driver correct: threading, property states, request rejection, versioning, testing and the device-type specific code the generator adds on its own. Known generator bugs and open design questions are collected in [Known Issues and Open Questions](#known-issues-and-open-questions); the main text refers to them by number.

It builds on the following documents and does not repeat them:

* [Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md) — the driver model, property semantics, [state transitions](DRIVER_DEVELOPMENT_BASICS.md#state-descriptions-and-state-transitions), [value vs. target](DRIVER_DEVELOPMENT_BASICS.md#item-value-vs-item-target), unified I/O and handler queues. Understanding it is a prerequisite for understanding the generated code.
* [Timers and Handler Queues](TIMERS_AND_QUEUES.md) — queue semantics, priorities, cancellation and the `INDIGO_COPY_*_PROCESS_*` macros.
* [Driver Generator Migration Guide](DRIVER_GENERATOR_MIGRATION.md) — converting an existing hand-written driver, `sdk` hot-plug details, C++ output and architecture restrictions.
* [Serial Device Simulators](SERIAL_DEVICE_SIMULATORS.md) and [indigo_test/AGENTS.md](../indigo_test/AGENTS.md) — simulators and automated tests.
* Repository rules in [AGENTS.md](../AGENTS.md) (section *Generated Drivers*) and [indigo_drivers/AGENTS.override.md](../indigo_drivers/AGENTS.override.md). Where this document and those rules differ, the rules win.

The authoritative reference for what the generator accepts is the generator itself. [indigo_tools/template.driver](../indigo_tools/template.driver) is a catalogue of every block and attribute with a one-line description, and [indigo_tools/driver_editor](../indigo_tools/driver_editor/README.md) is a local browser editor that edits `.driver` files and previews the generated source.

---

## When to Use the Generator

Use the generator for every new device driver of one of the supported device classes:

`ccd`, `wheel`, `focuser`, `mount`, `guider`, `rotator`, `dome`, `gps`, `ao`, `aux`, `polaralign`

with one of the supported transports:

| Transport block | Typical use | What the generator provides |
|-----------------|-------------|-----------------------------|
| none (virtual) | simulators, software-only devices | devices are attached at INIT, `on_connect` normally cannot fail |
| `serial` | RS-232, USB-serial, and anything `indigo_uni_open_*` can open from `DEVICE_PORT` (see [Opening serial, TCP and UDP transparently](DRIVER_DEVELOPMENT_BASICS.md#opening-serial-tcp-and-udp-transparently)) | `DEVICE_PORT`/`DEVICE_PORTS` (and optionally `DEVICE_BAUDRATE`), port patterns, `indigo_uni_handle *handle` in private data |
| `hid { ... }` | hidapi devices | libusb hot-plug callback, one logical device set, `indigo_uni_handle *handle` |
| `libusb { ... }` | devices opened directly through libusb | hot-plug with `MAX_DEVICES` logical slots, `libusb_device *usbdev` in private data |
| `sdk { ... }` | vendor SDKs with their own enumeration | hot-plug plus `plug`/`unplug` code blocks, see the [migration guide](DRIVER_GENERATOR_MIGRATION.md) |

Do not use the generator for agents or `system_*` drivers. `libusb` and `hid` drivers must be hot-plug drivers for now ([issue 4](#ki-4)). If the generator cannot express something the driver needs, do not hand-edit the output: describe the limitation and ask for a generator change first, as required by [AGENTS.md](../AGENTS.md).

---

## Directory Layout and Naming

A generated driver lives in its own directory under `indigo_drivers/` (or `indigo_linux_drivers/`, `indigo_mac_drivers/`):

```
indigo_drivers/<type>_<name>/
    indigo_<type>_<name>.driver     source of truth, written by you
    indigo_<type>_<name>.c          generated (.cpp with cpp = true), checked in
    indigo_<type>_<name>.h          generated, checked in
    indigo_<type>_<name>_main.c     generated standalone executable wrapper, checked in
    README.md                       driver documentation and the ## Testing record
    REFACTOR.md                     audit, plan, coverage and evidence (see AGENTS.override.md)
    <type>_<name>_simulator/        host-side protocol simulator, if the driver has one
    *.rules, bin_externals/, ...    optional udev rules, vendor SDK, firmware
```

The names are derived mechanically and must agree:

* `<name>` is the identifier after the `driver` keyword (`driver moonlite { ... }`).
* `<type>` is the type of the **first** device block. The generator writes `indigo_<type>_<name>.{c,h}` and `indigo_<type>_<name>_main.c`, defines `DRIVER_NAME` as `"indigo_<type>_<name>"` and exports the entry point `indigo_<type>_<name>(indigo_driver_action action, indigo_driver_info *info)`.
* `Makefile.drv` names the archive, shared library and executable `indigo_$(notdir $(pwd))`, so the directory must be called `<type>_<name>`.
* The private data type is `<name>_private_data`, accessed through `PRIVATE_DATA`.
* The low-level connection helpers must be called exactly `<name>_open(indigo_device *device)` and `<name>_close(indigo_device *device)`; the generated connection handler calls them. A `libusb` driver must also provide `<name>_match(libusb_device *dev, const char **name)`.

Driver-specific properties must use the `X_` prefix (for example `X_FOCUSER_STEPPING_MODE`). Every property added to a driver must also be documented in [PROPERTIES.md](PROPERTIES.md).

A new driver is not complete until it is registered where its siblings are: the Xcode project (`indigo.xcodeproj`, every new persistent file including the `.driver` file), the per-driver Windows `.vcxproj` files and, for drivers built into the server, the static driver table in `indigo_server/indigo_server.c`. Follow an existing driver of the same class and transport.

---

## The `.driver` File

### Lexical rules

* Blocks use `{ }`, attributes use `name = value;`. `//` comments are ignored outside code blocks. A `// TODO: ...` comment outside code blocks is copied to the top of the generated `.c`.
* Attribute values are one of: a **string** in double quotes, a **bool** (`true`/`false`), an **integer**, an **identifier**, or a **C expression**. A C expression is copied verbatim up to the next `;`, so `name = "FOO";` and `name = FOO_PROPERTY_NAME;` are both expressions.
* A **code block** (`code { ... }`, `on_change { ... }`, ...) is plain C copied into the generated source. Braces must balance and string literals must be closed; the generator only counts braces. The common leading indentation of the block is removed and replaced by the indentation of the generated context.
* Keywords are matched as prefixes ([issue 6](#ki-6)). Give driver-scope constants `UPPER_CASE` names so they cannot be mistaken for a keyword such as `code` or `data`.
* The first line of the file carries the license. A definition containing the text "GNU Lesser General Public" produces LGPL headers, anything else the INDIGO license header. Keep the usual version history comment below it.

Hand-written code in `.driver` blocks follows the repository formatting rules: tabs, K&R braces, braces on every `if`/`for`/`while`, every call on one line, no blank lines inside function bodies, exactly one blank line between functions.

### Driver block

```c
driver astromechanics {
	label = "ASTROMECHANICS LPM";
	author = "Peter Polakovic <peter.polakovic@cloudmakers.eu>";
	copyright = "Copyright (c) 2021-2026 CloudMakers, s. r. o.";
	version = 7;
	serial;
	data { ... }
	code { ... }
	aux { ... }
}
```

| Attribute / block | Type | Meaning |
|-------------------|------|---------|
| `label` | string | `DRIVER_LABEL`, shown to users. |
| `author`, `copyright` | string | Copied into generated headers. |
| `version` | integer | Driver build number; `DRIVER_VERSION` is `0x03000000 + version`. |
| `multi_device_support` | bool | Sets the multi-device flag of `indigo_driver_info` (default `false`). |
| `max_devices` | integer | `MAX_DEVICES` for `libusb`/`sdk` drivers (default 5). Every logical device (camera, its guider, its wheel) takes one slot. |
| `cpp` | bool | Emit `.cpp` instead of `.c`. |
| `supported_architecture` | string | Preprocessor guard, see the [migration guide](DRIVER_GENERATOR_MIGRATION.md#optional-architecture-restriction). |
| `serial`, `hid`, `libusb`, `sdk` | block | Transport; omit all four for a virtual driver. |
| `include { }` | code | Extra `#include` lines, emitted after the standard C headers. |
| `define { }` | code | Extra `#define`s. |
| `data { }` | code | Extra fields of `<name>_private_data`. |
| `code { }` | code | Shared low-level code: protocol helpers, `<name>_open`, `<name>_close`, `<name>_match`. |
| `on_init { }` | code | Runs at the start of `INDIGO_DRIVER_INIT`. |
| `on_shutdown { }` | code | Runs at the end of `INDIGO_DRIVER_SHUTDOWN`. |
| `NAME = expression;` | expression | Any other attribute becomes `#define NAME expression` in a `//+ definitions` section. |
| device blocks | block | At least one. The first device is the master device. |

`serial` accepts `configurable_speed = true;` (unhides `DEVICE_BAUDRATE`), `no_ports = true;` (keeps `DEVICE_PORTS` hidden and skips port enumeration) and any number of `pattern { vendor = ...; product = ...; serial = ...; vid = ...; pid = ...; exact_match = ...; }` blocks for automatic port matching. `hid` and `libusb` accept `hotplug`, `vid` and `pid`. The `sdk` block is described in the [migration guide](DRIVER_GENERATOR_MIGRATION.md).

### Device blocks

```c
focuser {
	name = "MoonLite";
	additional_instances = true;
	on_attach { ... }
	on_connect { ... }
	on_disconnect { ... }
	on_timer { ... }
	inherited FOCUSER_POSITION { ... }
	switch X_FOCUSER_STEPPING_MODE { ... }
}
```

| Attribute / block | Meaning |
|-------------------|---------|
| `id` | Optional C identifier for repeated device classes (`id = guider_ccd;`). Must precede properties and code blocks and be unique. It replaces the type in generated symbol names and code-block markers. |
| `name` | C expression for the device name. For virtual and serial drivers it becomes `<ID>_DEVICE_NAME` (a string literal or `DRIVER_LABEL`). For hot-plug drivers it is an `snprintf` format applied to the matched name, typically `"%s"`. Always set it. |
| `interface` | `INDIGO_INTERFACE_AUX_*` bits passed to `indigo_aux_attach()`; required for `aux`. |
| `additional_instances` | Enables the `ADDITIONAL_INSTANCES` property on the base device. |
| `attach_if`, `name_value` | `sdk` drivers only, see the [migration guide](DRIVER_GENERATOR_MIGRATION.md#optional-sdk-logical-devices-and-names). |
| `code { }` | Device-level helpers (handlers, finalizers) emitted before all generated handlers, so they may call the base-class API and each other. Forward-declare generated handlers you reference (`static void focuser_position_handler(indigo_device *device);`). |
| `on_attach { }` | Runs in `<id>_attach()` after the base class attach and **before** the driver's own properties are created. Use it for inherited properties (counts, ranges, hidden flags, INFO texts). |
| `on_connect { }` | Connected branch of the connection handler, see [Connection handler](#connection-handler). |
| `on_disconnect { }` | Disconnected branch, runs after pending handlers were cancelled. |
| `on_timer { }` | Body of `<id>_timer_callback()`, see [Timer callback](#timer-callback). |
| `on_detach { }` | Runs in `<id>_detach()` before the driver's properties are released. |
| property blocks | See below. |

With more than one device block, all logical devices share one private data structure and one hardware session: the generator adds `int count` to the private data, calls `<name>_open()` for the first connecting device (with the master device) and `<name>_close()` when the last one disconnects. Secondary devices get `master_device` set, so their handlers run on the master's queue.

### Property blocks

A property block is either a **local** property (`text`, `number`, `switch`, `light`), which the generator allocates, defines, deletes and releases, or an `inherited` property, which the base class owns and the driver only customizes.

```c
switch X_FOCUSER_STEPPING_MODE {
	name = "X_FOCUSER_STEPPING_MODE";
	group = FOCUSER_MAIN_GROUP;
	label = "Stepping mode";
	rule = INDIGO_ONE_OF_MANY_RULE;
	on_change { ... }
	item X_FOCUSER_STEPPING_MODE_HALF {
		name = "HALF";
		label = "Half";
		value = false;
	}
	item X_FOCUSER_STEPPING_MODE_FULL {
		name = "FULL";
		label = "Full";
		value = true;
	}
}
```

For a property declared as `X_FOO` the generator defines:

* `X_FOO_PROPERTY` — the property pointer (`PRIVATE_DATA->x_foo_property` for local properties; inherited handles come from the base class headers),
* `X_FOO_<ITEM>_ITEM` — item pointers in declaration order,
* `X_FOO_PROPERTY_NAME` / `<ITEM>_ITEM_NAME` — only when `name` is a string literal; otherwise the given identifier is used as is,
* the change handler `<device id>_<lower case property id>_handler`. When the device has no explicit `id` and the property id starts with the device type, the prefix is not repeated: `mount` + `MOUNT_PARK` gives `mount_park_handler`, `aux` + `CCD_EXPOSURE` gives `aux_ccd_exposure_handler`, `focuser` + `X_FOCUSER_STEPPING_MODE` gives `focuser_x_focuser_stepping_mode_handler`.

| Attribute / block | Local | Inherited | Meaning |
|-------------------|:-----:|:---------:|---------|
| `name` | yes | yes | Property name, string literal or identifier. |
| `label` | yes | yes | C string expression. Set it for every local property and item; there is no usable default. |
| `group` | yes | – | Group, default `MAIN_GROUP`. |
| `perm` | yes | yes | Default `INDIGO_RW_PERM`. The literal `INDIGO_RO_PERM` suppresses the change branch ([issue 11](#ki-11)). |
| `rule` | switch | yes | Default `INDIGO_ONE_OF_MANY_RULE`. |
| `hidden` | `true` only | expression | Local: only the literal `hidden = true;` hides the property at attach ([issue 2](#ki-2)). Inherited: see the note below. |
| `always_defined` | yes | – | Defined even while disconnected; not deleted on disconnect. |
| `persistent` | yes | yes | Saved with `CONFIG.SAVE` (the generator emits an `indigo_save_property()` for it in the `CONFIG` branch). |
| `preserve_values` | yes | yes | Copy only number **targets** from the request (`INDIGO_COPY_TARGETS_*`), keep current values. Use it for positions, temperatures, coordinates. |
| `asynchronous_change` | yes | yes | Default `true`. `false` runs the handler synchronously inside `change_property()` (`INDIGO_COPY_*_PROCESS_SYNC_CHANGE`). |
| `accept_while_busy` | yes | yes | Accept a request while the property is BUSY; see [Guider pulses](#guider-pulses) and [issue 3](#ki-3). |
| `pass_through_change` | yes | yes | After the generated branch, continue to the base class `change_property()` instead of returning. |
| `handler`, `handle`, `pointer` | yes | `handler`, `handle` | Override generated names. Rarely needed. |
| `code { }` | yes | yes | Property-level helpers, emitted with the low-level code. |
| `on_attach { }` | yes | yes | Runs right after this property is created (local) or after its hidden flag is set (inherited). |
| `reject_change { condition = ...; message = "..."; }` | yes | yes | Admission guard, repeatable, see [Rejecting requests](#rejecting-requests). |
| `on_change_request { }` | yes | yes | Code in the bus change branch before the values are copied. |
| `on_change { }` | yes | yes | Body of the change handler. |
| `on_detach { }` | yes | yes | Runs in `<id>_detach()` before the property is released ([issue 8](#ki-8)). |
| `item <ID> { name; label; value; min; max; step; format; }` | yes | – | Items; `min`/`max`/`step`/`format` for numbers only. Defaults: `""`, `0`, `false`, `INDIGO_IDLE_STATE`. `item <ID>;` accepts all defaults. |

Notes:

* **Listing an inherited property unhides it.** For every `inherited` block the generator emits `<PROPERTY>->hidden = <hidden>;` after the device's `on_attach`, with `false` when no `hidden` attribute is given. `inherited FOCUSER_TEMPERATURE { hidden = false; }` exposes an optional base property; to keep a listed inherited property hidden, give it `hidden = true;` or an expression ([issue 1](#ki-1)).
* `light` properties and `INDIGO_RO_PERM` properties never get a change branch. An `inherited` property without `on_change` gets no change branch either; requests go to the base class. A `reject_change` on a property without a change branch is reported as an error.
* `on_change { }` with an empty body is the **copy-only** form: the branch copies values (or targets), sets OK, publishes and returns, without scheduling a handler. Use it for settings the driver only reads later (`switch X_CONFIG { on_change { } ... }` in `aux_dsusb`).

---

## Generated Code and What You May Edit

### Sections of the generated source

The generated `.c` always has the same order. The comments in the table are the `#pragma mark` headings of the output.

| Section | Contents |
|---------|----------|
| header | License, `// This file generated from <file>.driver`, copied `// TODO:` comments, optional `#if <supported_architecture>` |
| Includes | Standard headers, `include { }`, `indigo_driver_xml.h`, `indigo_<type>_driver.h` for every device type (plus `indigo_align.h` for mounts), `indigo_uni_io.h`, `indigo_usb_utils.h` for USB drivers |
| Common definitions | `DRIVER_VERSION`, `DRIVER_NAME`, `DRIVER_LABEL`, `<ID>_DEVICE_NAME`, `MAX_DEVICES`, `PRIVATE_DATA`, driver-scope definitions, `define { }` |
| Property definitions | Handle, item and name macros of local properties |
| Private data definition | `count`, `handle` or `usbdev`, local property pointers, `data { }` |
| Low level code | driver queue (hot-plug), `code { }`, every device `code { }`, every property `code { }` |
| High level code (per device) | `<id>_timer_callback()`, `<id>_connection_handler()`, property change handlers |
| Device API (per device) | `<id>_attach()`, `<id>_enumerate_properties()`, `<id>_change_property()`, `<id>_detach()` |
| Device templates | `INDIGO_DEVICE_INITIALIZER` per device |
| Hot-plug code | libusb callback, plug/unplug handlers on the driver queue |
| Main code | Entry point: INIT (after `on_init`), SHUTDOWN (before `on_shutdown`), INFO |

### Generated output is never edited

Running the generator rewrites the complete `.c`, `.h` and `_main.c` from the `.driver` file. Nothing in the output is preserved between runs. Each code block is wrapped in `//+ <id>` ... `//- <id>` markers (`//+ focuser.FOCUSER_POSITION.on_change`) that identify the origin of the code; the driver editor uses them for its source mapping. They are **not** protected regions: an edit between markers in the `.c` is lost on the next `make`. All changes go into the `.driver` file. (The generator also has a `-c` mode that reads the markers to rebuild a `.driver` from a `.c`; it exists for migrating old drivers and is not a way to develop a generated driver.)

You own the contents of the code blocks. The generator owns everything else: allocation, definition and deletion of properties, the change dispatch and its admission checks, the prologue and epilogue of every handler, the connection lifecycle including open/close reference counting, timer start, cancellation of pending work on disconnect, detach, hot-plug and the entry point.

### Change dispatch (`<id>_change_property`)

For every property with a change branch, in declaration order, the generator emits:

```c
} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
	/* mount only: parked-mount admission check, see below */
	INDIGO_REJECT_CHANGE_IF(<reject_change condition>, FOCUSER_POSITION_PROPERTY, "<message>");
	//+ focuser.FOCUSER_POSITION.on_change_request
	...
	//- focuser.FOCUSER_POSITION.on_change_request
	INDIGO_COPY_TARGETS_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
	return INDIGO_OK;
```

The dispatch macro is chosen as follows:

| Condition | Emitted macro | Busy guard | Queue priority |
|-----------|---------------|------------|----------------|
| `on_change { }` empty | copy, `state = OK`, `indigo_update_property()` | none | no handler |
| `asynchronous_change = false` | `INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE` / `..._TARGETS_...` with `preserve_values` | yes | runs inline |
| `CCD_ABORT_EXPOSURE`, `FOCUSER_ABORT_MOTION`, `ROTATOR_ABORT_MOTION`, `MOUNT_ABORT_MOTION`, `DOME_ABORT_MOTION`, `POLARALIGN_ABORT_MOTION` | `INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE` | yes | `URGENT` |
| `preserve_values = true` | `INDIGO_COPY_TARGETS_PROCESS_CHANGE` | yes | `NORMAL` |
| `MOUNT_MOTION_*` | `INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME` | **no** | `NORMAL` |
| `GUIDER_GUIDE_RA`, `GUIDER_GUIDE_DEC` | `INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE` (`..._ANYTIME` with `accept_while_busy`) | yes (no) | `TIME` |
| anything else | `INDIGO_COPY_VALUES_PROCESS_CHANGE` | yes | `NORMAL` |

The rows are tested top to bottom ([issue 13](#ki-13)). All asynchronous macros copy the request into the property, set it BUSY, publish it and queue the handler (see [Property Change Macros](TIMERS_AND_QUEUES.md#property-change-macros)). The busy guard silently ignores a request for a property that is already BUSY; do not duplicate it in `reject_change` or `on_change_request`.

After all branches, if any property is `persistent`, a `CONFIG` branch saves those properties when `CONFIG.SAVE` is requested and then falls through; the function always ends with `indigo_<type>_change_property(device, client, property)`.

### Change handlers

For a non-empty `on_change` the generator emits:

```c
static void focuser_position_handler(indigo_device *device) {
	FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;              // prologue
	//+ focuser.FOCUSER_POSITION.on_change
	...                                                               // your code
	//- focuser.FOCUSER_POSITION.on_change
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);  // epilogue
}
```

* The **OK prologue** is omitted when the `on_change` text begins with `<PROPERTY>->state = ` or contains `_finalizer` anywhere ([issue 12](#ki-12)).
* The **epilogue** is `indigo_update_coordinates(device, NULL)` for `MOUNT_EQUATORIAL_COORDINATES`, nothing when the `on_change` text contains `_finalizer`, and `indigo_update_property()` otherwise.
* The mount properties listed in [Mount](#mount) get a parked-mount guard in front of the code, and the manual motion handlers get the motion commit after it.

So in the normal case the `on_change` block only performs the operation and sets `INDIGO_ALERT_STATE` on failure; the generator publishes the result. Write explicit updates only for early returns, custom messages, or self-managed asynchronous operations.

### Connection handler

The generator owns the connection lifecycle. For a non-virtual driver the connected branch is:

```c
bool connection_result = true;
connection_result = <name>_open(device);          // with reference counting for multi-device drivers
if (connection_result) {
	//+ <id>.on_connect
	...                                            // may set connection_result = false
	//- <id>.on_connect
}
if (connection_result) {
	/* define local properties, CONNECTION OK, "Connected to ..." */
} else {
	/* "Failed to connect ...", close for multi-device drivers, CONNECTION ALERT + DISCONNECTED */
}
```

and the disconnected branch is:

```c
indigo_cancel_pending_handlers(device);
//+ <id>.on_disconnect
...
//- <id>.on_disconnect
/* every declared property still BUSY returns to OK */
/* delete local properties, <name>_close() (last device), "Disconnected from ..." , CONNECTION OK */
```

followed in both cases by `indigo_<type>_change_property(device, NULL, CONNECTION_PROPERTY)` and, if the device has an `on_timer` block and is now connected, the first queued `<id>_timer_callback`.

Rules for `on_connect` and `on_disconnect`:

* `<name>_open()` is transactional: it returns `true` with all resources acquired, or `false` after releasing everything it acquired. Do not keep an `opened` flag to paper over a close after a failed open.
* Report failure by assigning the provided `connection_result`; do not declare another result variable and never `return` from `on_connect` or `on_disconnect` — that skips the generated property definition/deletion, reference counting, messages and the final CONNECTION update.
* When `on_connect` fails after a successful open in a **single-device** driver, the block must call `<name>_close()` itself before setting `connection_result = false` (as `focuser_moonlite` does, [issue 5](#ki-5)). In a **multi-device** driver the generated failure branch closes the shared session when the reference count drops to zero, so the block must not close it.
* A virtual `on_connect` runs unconditionally. It may opt into failure handling by assigning `connection_result`.
* Pending handlers are already cancelled when `on_disconnect` runs, but property states are not yet reset: `on_disconnect` still sees BUSY properties and can stop the hardware operation they represent (exposure, motion). Stop running hardware operations there and cancel self-rescheduling callbacks that were started outside the queue.
* Hot-plug drivers run the connection handler on the per-driver queue, which serializes open/close across all instances of the driver. The exception are `libusb`/`hid` drivers with several device blocks: only the connection that opens the shared session goes through the driver queue, later ones run on the device queue.

### Timer callback

`on_timer` becomes the body of `<id>_timer_callback(indigo_device *device)`, which returns immediately when the device is not connected. The generator queues the first call at the end of a successful connection; it never reschedules it. A periodic poll must reschedule itself explicitly at the end of the block:

```c
on_timer {
	...
	indigo_execute_handler_in(device, 10, aux_timer_callback);
}
```

Disconnect cancels it together with all other pending handlers.

### Attach, enumerate and detach

`<id>_attach()` calls the base class attach, applies `additional_instances`, unhides the serial port properties of the master device, runs the device `on_attach`, then creates each local property (with its items, formats and `hidden = true`), applies the hidden flag of each inherited property, and runs each property's `on_attach`. `<id>_enumerate_properties()` defines local properties while connected and `always_defined` ones always. `<id>_detach()` disconnects a connected device through the connection handler, runs `on_detach` and the property `on_detach` blocks, and releases local properties before calling the base class detach.

---

## Handlers, Threads and Locking

Two execution contexts matter:

* **Bus context** — `<id>_change_property()`, and therefore every `reject_change` condition and `on_change_request` block, runs on the caller's thread with the bus mutex held. Keep it short: no device I/O, no SDK calls, no waiting, no sleeping. A synchronous handler (`asynchronous_change = false`) runs in this context too, so use it only for handlers that cannot block.
* **Queue context** — asynchronous change handlers, `<id>_timer_callback`, finalizers and anything queued with `indigo_execute_handler*()` run on the handler queue of the **master device**, one task at a time, with the master device mutex held (see [Driver-Facing Wrappers](TIMERS_AND_QUEUES.md#driver-facing-wrappers)). All logical devices of one driver instance are serialized against each other, which is what makes the shared transport handle in `PRIVATE_DATA` safe to use without extra locks. In a handler reached from a slave device, `device` is that slave device; use it directly.

Rules for code in handlers:

* Do not block the queue. A bounded protocol transaction is fine; waiting for motion, calibration, an exposure or homing is not. Use the **handler + finalizer** pattern: the handler starts the operation, publishes BUSY and schedules `<operation>_finalizer`; the finalizer checks progress, reschedules itself while work remains and publishes OK or ALERT at the end. `focuser_moonlite` (`motion_finalizer`) and `mount_simulator` (`manual_motion_finalizer`) are complete examples.
* Name completion callbacks `*_finalizer`. The generator detects the text `_finalizer` in `on_change` and then emits neither the OK prologue nor the final update, so the handler must publish the start (BUSY) or the immediate result itself, and the finalizer publishes the completion.
* Cancel only concrete pending work: an abort cancels the pending start handlers and finalizers it supersedes (`indigo_cancel_pending_handler(device, focuser_steps_handler)` etc.) and settles their properties. Do not add defensive `indigo_cancel_pending_handler()` calls in front of operations the busy guard already protects.
* An urgent abort can overtake a queued start whose property is already BUSY. The abort handler must settle that property even though the start never reached the hardware.
* Priorities order tasks that are due; they never interrupt a running handler. Note that `indigo_execute_handler_in()` uses `TIME` priority while `indigo_execute_handler()` uses `NORMAL`.
* Values reaching a handler are validated by the framework (range, step, finite). Validate device replies and operational constraints instead.

---

## Property States, Messages and Rejecting Requests

Follow the [state transition rules](DRIVER_DEVELOPMENT_BASICS.md#state-descriptions-and-state-transitions). In generated drivers this reduces to:

* The dispatch macro publishes BUSY when the request is accepted.
* An instantaneous handler lets the generator set OK and only sets `INDIGO_ALERT_STATE` on failure:

  ```c
  on_change {
  	if (!IS_CONNECTED || !moonlite_command(device, -1, FOCUSER_MODE_AUTOMATIC_ITEM->sw.value ? ":+#" : ":-#")) {
  		FOCUSER_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
  	}
  }
  ```

  Avoid `PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;`; the prologue already set OK.
* A long operation leaves the property BUSY and the finalizer publishes OK or ALERT. Every path must end in OK or ALERT; a property left BUSY blocks all further requests through the busy guard.
* For number properties with `preserve_values`, keep the request in `target` and report the measured value in `value` (see [Item Value vs Item Target](DRIVER_DEVELOPMENT_BASICS.md#item-value-vs-item-target)).
* Messages go with the update: `indigo_update_property(device, P, "Aborted")`, or `indigo_send_message(device, P, "...")` for a message without an update.

### Rejecting requests

A request the driver cannot serve at the moment must be answered, not dropped. Declare the condition in the `.driver` file:

```c
inherited FOCUSER_POSITION {
	preserve_values = true;
	reject_change {
		condition = FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE || FOCUSER_ABORT_MOTION_PROPERTY->state == INDIGO_BUSY_STATE;
		message = "Motion already in progress";
	}
	on_change { ... }
}
```

The generator emits `INDIGO_REJECT_CHANGE_IF(condition, PROPERTY, "message")` at the top of the change branch, before `on_change_request` and before anything is copied. On a match `indigo_reject_change()` marks every item for update (so the client sees the driver's real values again), sets ALERT, publishes the message and returns. Several `reject_change` blocks are tested in declaration order. Do not use it for the property's own BUSY state; the dispatch macro handles that without overwriting the running operation's state.

Use `on_change_request` only for request-scoped bookkeeping that `reject_change` cannot express, such as zeroing items before a guide pulse is copied, or delegating a request to the base class (`mount_simulator` forwards a plain SYNC to `indigo_mount_change_property()`). It has no generated epilogue; if it returns early, it must publish any rejection itself.

---

## Device-Type Specifics Emitted by the Generator

### Aborts

The six abort properties listed in the dispatch table are queued at `INDIGO_TASK_PRIORITY_URGENT`, so they overtake queued ordinary work. The handler must still stop the hardware, cancel the pending handlers and finalizers of the operations it aborts, settle their properties, switch the abort item off and publish. This applies also when an abort property is declared as a local property of another device class (`CCD_ABORT_EXPOSURE` in `aux_dsusb`).

### Guider pulses

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` are dispatched at `INDIGO_TASK_PRIORITY_TIME`. By default a pulse arriving while the previous one on the same axis is BUSY is ignored by the busy guard. With `accept_while_busy = true;` the request is taken anyway and the driver owns the replacement: clear the stale items in `on_change_request`, cancel the running pulse's finalizer, start the new pulse and schedule `guider_guide_<axis>_finalizer` (see the guider device of `mount_simulator`). Every exposed guider interface needs the pulse-duration accuracy measurement required by [indigo_drivers/AGENTS.override.md](../indigo_drivers/AGENTS.override.md).

### Mount

* **Parked-mount admission.** For `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA` and `MOUNT_TRACKING` the change branch starts with `INDIGO_REJECT_CHANGE_IF(!MOUNT_PARK_PROPERTY->hidden && MOUNT_PARK_PARKED_ITEM->sw.value, ..., "Mount is parked!")`, before any `reject_change` or `on_change_request`, so a refused request never reaches the property values. The handler repeats the check before the `on_change` code, for a request admitted just before a park request was accepted: it sends "Mount is parked!", publishes ALERT and returns. Do not duplicate either check in the `.driver` file.
* **Manual motion is processed anytime.** `MOUNT_MOTION_DEC` and `MOUNT_MOTION_RA` use `INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME`, because the release (all items off) or a change of direction must be accepted while the motion is BUSY. The handler therefore has to cope with any combination of items at any time.
* **Motion ownership.** The generator records the requesting client and commits the motion ownership, see the next section.
* **Coordinates.** The `MOUNT_EQUATORIAL_COORDINATES` handler ends with `indigo_update_coordinates()` instead of `indigo_update_property()`; do not call it yourself at the end of `on_change`. Use `preserve_values = true;` so the request lands in the targets.
* **Abort.** `MOUNT_ABORT_MOTION` is urgent. It has to stop slews, park, home and manual motion, cancel their pending handlers and finalizers, switch all `MOUNT_MOTION_*` items off and publish the affected properties (see `mount_simulator`).

---

## Manual Motion and Client Detach (Mount)

> (API being finalized) The helpers below exist in the working tree, but the exact shape of the generated wrapper is still being settled; verify against the current `indigo_tools/indigo_generator.c` and a regenerated mount driver.

Manual motion (`MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`) runs until the client switches it off. If that client disappears in between — typically because its network connection is lost — nobody would ever send the release and the mount would keep slewing. INDIGO solves this with a detach-abort registry in the bus and two mount base-class helpers.

### Bus registry

Declared in [indigo_bus.h](../indigo_libs/indigo/indigo_bus.h):

```c
typedef struct { indigo_client *client; uint64_t generation; } indigo_client_ref;
indigo_client_ref indigo_current_client_ref(indigo_client *client);
indigo_result indigo_register_detach_abort(indigo_device *device, const indigo_client_ref *client, indigo_property *property, indigo_property *abort);
indigo_result indigo_unregister_detach_abort(indigo_device *device, const char *property_name);
```

* **Owner identity is the attach generation, not the client pointer.** `indigo_attach_client()` gives every attachment a new generation from a bus-wide 64-bit counter that is never reset or reused; the bus keeps it in a private array parallel to its client table, so `indigo_client` itself is unchanged. `indigo_current_client_ref()` returns the pointer and generation of the client's current attachment, read together under the bus mutex, or `{ NULL, 0 }` if the client is `NULL` or not attached. A reference taken while the owner was attached never matches a later attachment — neither a client allocated at the same address after the owner was detached and freed (the TCP server reuses freed XML adapters) nor the same `indigo_client` attached again.

* An entry is identified by the device and the property name; a later registration for the same property replaces the earlier one (another client took the motion over).
* `client` is the **address** of the device's record of the requesting client, an `indigo_client_ref` taken with `indigo_current_client_ref()` when the request was accepted. It is read under the bus mutex, the same mutex under which `change_property()` writes it. If the recorded reference is empty or its attachment has ended (the owner detached, whether or not a client has been attached at the same address since), nothing is registered and `INDIGO_NOT_FOUND` is returned; the device must then stop the operation itself. A full registry returns `INDIGO_TOO_MANY_ELEMENTS`.
* `abort` is copied at registration. When the owning attachment ends (the entry is matched by pointer and generation), the bus sends that copy through `indigo_change_property()` with the device's current access token and removes the entry.
* The bus never infers the end of an operation from property updates. The device must unregister whenever the operation ends or is stopped, including disconnect and detach. Entries left behind by a detached device are dropped with a log message.

### Mount base-class helpers

Declared in [indigo_mount_driver.h](../indigo_libs/indigo/indigo_mount_driver.h); the mount context holds the `indigo_client_ref` records `motion_dec_client` and `motion_ra_client`.

```c
void indigo_mount_record_motion_client(indigo_device *device, indigo_client *client, indigo_property *property);
void indigo_mount_commit_motion_client(indigo_device *device, indigo_property *property);
```

* `indigo_mount_record_motion_client()` stores the reference of the requesting client's current attachment (`indigo_current_client_ref()`) for a `MOUNT_MOTION_DEC`/`MOUNT_MOTION_RA` request in the mount context. It is called in the change branch, after the admission checks and just before the request is copied. A request without a client (or from a client that is not attached) records an empty reference.
* `indigo_mount_commit_motion_client()` is called at the end of the motion handlers, after the driver started or stopped the motion:
  * if any item is on and the property is not ALERT, it registers the release — the same property with all items off — for the recorded client;
  * if the recorded attachment has already ended — also when the owner detached before the commit and another client was attached at the same address in the meantime — or the reference is empty, or the registration fails, it sends that release at once through `indigo_change_property()`; the release passes the change handler again with all items off, which unregisters, so there is no loop;
  * if all items are off or the state is ALERT, it unregisters;
  * called with `MOUNT_ABORT_MOTION_PROPERTY` (from the abort handler) it unregisters both axes;
  * it never publishes the property.
* The base class also unregisters both axes when the device disconnects or detaches.

A request sent with a `NULL` client cannot be owned, so the commit releases it immediately ([issue 9](#ki-9)).

### What the generator emits

* In the `MOUNT_MOTION_DEC` and `MOUNT_MOTION_RA` change branches, after the parked-mount check, `reject_change` and `on_change_request`, and before the copy:

  ```c
  indigo_mount_record_motion_client(device, client, property);
  INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(MOUNT_MOTION_DEC_PROPERTY, mount_motion_dec_handler);
  ```

* In the `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA` and `MOUNT_ABORT_MOTION` handlers, inline after the driver's `on_change` code and the generated epilogue, `indigo_mount_commit_motion_client(device, <PROPERTY>)`; the parked-mount guard of the two motion handlers commits before its `return` as well.
* The handlers, and therefore the commit, exist only when these properties have a non-empty `on_change` ([issue 9](#ki-9)). A mount that implements manual motion always has one.

### Rules for driver authors

* **Do not `return` early from the `on_change` code** of `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA` and `MOUNT_ABORT_MOTION`; it skips the commit ([issue 9](#ki-9)). Structure the block with `if`/`else` so it falls through to the end.
* **Make the property tell the truth before the commit runs.** The commit decides from the items and the state: items on and not ALERT means "running". If the motion could not be started, switch the items off or set `INDIGO_ALERT_STATE`; if it started, keep the requested items on.
* **Motion the driver stops by itself** — a limit, a goto or park that replaces the manual motion, a transport error — must switch the `MOUNT_MOTION_*` items off, set the state and publish the property. That keeps the property consistent for the next commit, and a stale registration only sends a harmless release. The driver may call `indigo_mount_commit_motion_client(device, MOUNT_MOTION_x_PROPERTY)` after switching the items off to unregister at once.
* **The abort handler** must switch the motion items off and publish both motion properties; the generated commit with `MOUNT_ABORT_MOTION_PROPERTY` then unregisters both axes.
* **Hand-written mount drivers** get none of this automatically. They must call `indigo_mount_record_motion_client(device, client, property)` in the `MOUNT_MOTION_DEC`/`MOUNT_MOTION_RA` branches of their `change_property()` just before copying the request, and `indigo_mount_commit_motion_client()` on every exit path of the motion handlers and, with `MOUNT_ABORT_MOTION_PROPERTY`, of the abort handler.
* Other operations that must not outlive their client can use `indigo_register_detach_abort()` / `indigo_unregister_detach_abort()` directly with the same ownership rules.

---

## Build, Regeneration and Versioning

The generator is built by the top-level build into `build/bin/indigo_generator` (target in `indigo_tools/Makefile`). Build it before building drivers, and rebuild it after pulling generator changes.

`Makefile.drv` picks up the `*.driver` file in the driver directory automatically. The generated source depends on the `.driver` file and on `indigo_tools/indigo_generator.c`, and one generator run produces all three outputs, so `make` regenerates whenever the definition or the generator source is newer than the generated source:

```sh
cd indigo_drivers/<type>_<name>
make -f ../../Makefile.drv                            # regenerates if needed, then builds
../../build/bin/indigo_generator indigo_<type>_<name>.driver   # explicit regeneration
```

Run the generator from the driver directory: it reads `../../indigo_libs/indigo_<type>_driver.c` to warn about inherited properties it unhides ([issue 14](#ki-14)). Always pass the path of an existing `.driver` file ([issue 7](#ki-7)).

The [driver editor](../indigo_tools/driver_editor/README.md) is an optional front end for the same step. It edits the `.driver` tree with attribute rows taken from `template.driver`, runs the unchanged generator in a temporary workspace to preview the generated source (highlighting the output of the selected node), and writes the definition and the generated files only on request:

```sh
python3 indigo_tools/driver_editor/driver_editor.py indigo_drivers/<type>_<name>/indigo_<type>_<name>.driver
```

Workflow for every change:

1. Edit the `.driver` file (in an editor or the driver editor).
2. Regenerate and build the driver; fix compiler warnings in your code blocks.
3. Inspect `git diff` of the generated files. Every behavioural difference must be explained by your change or by generator semantics.
4. Run the narrowest simulator or integration test, then the driver's full suite.
5. Commit the `.driver` file together with the regenerated `.c`/`.cpp`, `.h` and `_main.c`.

**Versioning.** Increment `version` in the `.driver` file for every change of driver behaviour (including simulators), regenerate, and check that the new version is higher than the old one. A new driver starts with the version its author assigns; its `DRIVER_VERSION` is `0x03000000 + version`. Tests never assert the exact version, only the API generation (see [Driver Version Assertions](../indigo_test/AGENTS.md#driver-version-assertions)).

---

## Testing

How to write, register and run tests for a driver — backends (protocol simulator, fake SDK/USB, virtual driver), harness helpers, the Makefile wiring, logs, sanitizers, MountSim and hardware runs — is described in [Driver Testing Basics](DRIVER_TESTING_BASICS.md), including a section on testing a new generated driver. Generator-specific points: test through the generated device names and public property names (the generated header exposes only the entry point), cover the paths the generator owns (connect rollback, shared-connection reference counting, BUSY guard, finalizer versus abort/disconnect, parked-mount guard, hot-plug events), and for a mount include manual motion released by a detaching client; `test_detach_abort` covers this for `mount_simulator` and `mount_lx200` only ([issue 10](#ki-10)). Changes to the generator itself are covered by `test_generator_architecture`.

---

## Known Issues and Open Questions

This section lists known generator bugs, limitations and open design questions. Line numbers refer to [indigo_tools/indigo_generator.c](../indigo_tools/indigo_generator.c) at the revision of this document and shift as the file changes; the function names are the stable reference. Each entry says where the behaviour comes from, what it means for a driver author, and the current workaround. Once an entry is fixed, remove it here and update the main text.

<a id="ki-1"></a>
**1. A listed `inherited` property is made visible.** `write_c_attach()` (lines 1880–1885) emits `<PROPERTY>->hidden = <hidden>;` for every `inherited` block, with `false` when the block has no `hidden` attribute, and it emits this after the device `on_attach` (line 1831). `parse_base_code()` (lines 2768–2791) only prints `Warning: <PROPERTY>->hidden set to false` for properties the base class hides.
*Impact:* listing a property only to add `on_change`, `reject_change` or `persistent` also unhides it, and a `hidden = true` set in the device `on_attach` is silently overwritten.
*Workaround:* set the visibility in the block (`hidden = true;` or an expression), never in the device `on_attach`, and read the generator warnings.

<a id="ki-2"></a>
**2. Local properties are hidden only by the literal `hidden = true`.** `write_c_attach()` (lines 1886–1889) emits `hidden = true` only when the attribute text is exactly `true`; an expression or `false` emits nothing.
*Impact:* `hidden = SOME_CONDITION;` on a local property is ignored without a warning, so the property stays visible.
*Workaround:* set `<PROPERTY>->hidden = <expression>;` in the property's own `on_attach` block, which runs after the property is created.

<a id="ki-3"></a>
**3. `accept_while_busy` only affects `GUIDER_GUIDE_RA`/`GUIDER_GUIDE_DEC`.** The attribute is parsed for every property (line 720) but used only in the guider row of the dispatch in `write_c_change_property()` (line 2004).
*Impact:* on any other property it is accepted and silently ignored; the busy guard still drops requests while the property is BUSY.
*Workaround:* none in the generator. Use it only on the two guider properties; for other properties that must accept requests while BUSY, ask for a generator change.

<a id="ki-4"></a>
**4. `libusb { hotplug = false; }` and `hid { hotplug = false; }` generate no discovery code.** The non-hot-plug branches of the INIT case in the entry point are `// TBD` (lines 2551–2553 for `libusb`, 2600–2602 for `hid`).
*Impact:* the driver compiles and loads but never attaches a device.
*Workaround:* use hot-plug, `sdk { hotplug = false; ... }` with explicit `plug`/`unplug` code, or a hand-written driver.

<a id="ki-5"></a>
**5. Single-device drivers do not close the session when `on_connect` fails.** In `write_c_connection_change_handler()` the failure branch (lines 1695–1708) calls `<name>_close()` only for multi-device drivers (lines 1701–1705).
*Impact:* a single-device driver whose `on_connect` sets `connection_result = false` after a successful `<name>_open()` leaves the port or handle open while the device reports DISCONNECTED.
*Workaround:* in a single-device driver, `on_connect` calls `<name>_close(device)` itself before setting `connection_result = false` (see `focuser_moonlite`). Multi-device drivers must not close in `on_connect`.

<a id="ki-6"></a>
**6. Keywords are matched by prefix.** `match()` compares an identifier with `strncmp(value, begin, strlen(value))` (line 391), so `codec` matches `code` and `data_rate` matches `data`. The tokenizer does the same for booleans (lines 349–356): any word starting with `true` or `false` is read as a boolean token.
*Impact:* a driver-scope `NAME = expression;` or an attribute whose name starts with a keyword is parsed as that keyword and fails with a misleading error or produces unexpected output.
*Workaround:* give driver-scope constants `UPPER_CASE` names and avoid identifiers that begin with a keyword, `true` or `false`.

<a id="ki-7"></a>
**7. A non-existent `.driver` path silently switches to `-c` mode.** `main()` parses the definition only when the file exists (line 3487); otherwise it runs the reverse-extraction branch (lines 3512–3533), which creates a `.driver` file at the given path, rebuilt from a `.c`/`.cpp` of the same base name.
*Impact:* a typo in the path creates a `.driver` file from an unrelated or stale `.c` instead of reporting an error; with no matching `.c` it fails with "No base source file name found".
*Workaround:* check the path before running the generator, and check `git status` after the run for a new or changed `.driver` file.

<a id="ki-8"></a>
**8. Property `on_detach` code carries an `on_attach` marker.** `write_c_detach()` (line 2049) wraps each property `on_detach` block in `//+ <id>.<PROPERTY>.on_attach` markers.
*Impact:* the generated code is correct and runs in `<id>_detach()`, but the marker names the wrong block, so the driver editor's source mapping (and `-c`) attribute it to `on_attach`.
*Workaround:* none needed for the driver; keep this in mind when reading the generated source.

<a id="ki-9"></a>
**9. Mount manual-motion commit (API being finalized).** The shape of the generated record/commit is still being settled; verify against the current generator and a regenerated mount driver.
* The client is recorded in the change branch of `MOUNT_MOTION_DEC`/`MOUNT_MOTION_RA` for every change branch (lines 1979–1982), but the handler that commits exists only when the property has a non-empty `on_change` (line 1801). With an empty `on_change { }` (copy-only form, lines 1983–1990) the client is recorded and never committed, so a detaching client does not stop the motion. The same applies to `MOUNT_ABORT_MOTION`: with an empty `on_change` nothing unregisters.
* The commit is emitted inline at the end of the handler (line 1783) and before the `return` of the parked-mount guard (lines 1766–1768). An early `return` in the `on_change` code skips it, leaving a running motion unowned or a stopped motion registered.
* A request with a `NULL` client (sent with `indigo_change_property(NULL, ...)`, for example from driver code) records an empty reference; `indigo_mount_commit_motion_client()` ([indigo_mount_driver.c](../indigo_libs/indigo_mount_driver.c) lines 84–115) then fails to register and sends the release at once, so such motion stops immediately after it starts. **Open question:** whether that is the intended behaviour for requests without a client, or whether they should run unowned.
*Impact:* see each point.
*Workaround:* give `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA` and `MOUNT_ABORT_MOTION` a non-empty `on_change`, write it without early returns, and send manual motion requests with a client.

<a id="ki-10"></a>
**10. Detach-abort and motion ownership are runtime-tested on two mount drivers only.** `indigo_test/integration/test_detach_abort.c` covers the registry API and the generated record/commit on `mount_simulator`, on `mount_lx200` against its serial simulator and through `agent_mount`, with in-process owners that detach and, in its opt-in `--network` mode, with TCP peers of the in-process server that close, reset or lose the connection (see *Client-detach motion release* in [Driver Testing Basics](DRIVER_TESTING_BASICS.md#client-detach-motion-release)). The other eight generated mount drivers (`mount_ioptron`, `mount_nexstar`, `mount_nexstaraux`, `mount_pmc8`, `mount_rainbow`, `mount_starbook`, `mount_synscan`, `mount_temma`) are covered for this path only by compilation and by their own suites, which never detach the owner of a running motion.
*Impact:* a driver-specific regression in those drivers' motion or abort handlers (an early `return` that skips the commit, a property left ON or not ALERT after a failed start) is not detected automatically.
*Workaround:* when writing or changing a mount driver, add a case modelled on the `simulator_*`/`lx200_*` cases of `test_detach_abort`: start manual motion from an extra client, fence the device queue, detach the client, and assert the `Aborting` log marker, the stop of the motion and that `indigo_unregister_detach_abort()` finds no entry. See *Known Gaps* in [Driver Testing Basics](DRIVER_TESTING_BASICS.md#known-gaps).

<a id="ki-11"></a>
**11. Read-only detection relies on the literal `INDIGO_RO_PERM`.** The parser clears the change branch only when `perm` is exactly `INDIGO_RO_PERM` (line 745). The later checks in `write_c_high_level_code_section()` (line 1801) and `write_c_change_property()` (lines 1958, 1962) compare with the misspelled `"INDIGO_PERM_RO"` and are therefore always true.
*Impact:* a read-only property whose `perm` is written any other way (an expression, a macro) gets a change branch and handler.
*Workaround:* write `perm = INDIGO_RO_PERM;` exactly.

<a id="ki-12"></a>
**12. Handler prologue and epilogue are chosen by text search.** `write_c_property_change_handler()` (lines 1771–1779) drops both the OK prologue and the final `indigo_update_property()` when the `on_change` text contains `_finalizer` anywhere, including a comment or an `indigo_cancel_pending_handler(device, x_finalizer)` call; the prologue is also dropped only when the text *begins* with `<PROPERTY>->state = `. For virtual drivers, `connection_result` handling is enabled when the `on_connect` text contains that word (line 1625).
*Impact:* a handler that only cancels a finalizer publishes nothing unless it does so itself; a handler that sets the state later in the code still gets the OK prologue.
*Workaround:* when `_finalizer` appears in `on_change`, publish the property explicitly on every path.

<a id="ki-13"></a>
**13. Dispatch rules are tested in a fixed order.** In `write_c_change_property()` (lines 1991–2008) the `preserve_values` row precedes the `MOUNT_MOTION_*` and guider rows.
*Impact:* `preserve_values` on `MOUNT_MOTION_*` or `GUIDER_GUIDE_*` replaces the ANYTIME or TIME-priority dispatch with the ordinary busy-guarded one.
*Workaround:* do not set `preserve_values` on those properties.

<a id="ki-14"></a>
**14. The base-class scan uses a path relative to the working directory.** `parse_base_code()` opens `../../indigo_libs/indigo_<type>_driver.c` (line 2772) and skips the scan silently if it is not found.
*Impact:* run from another directory, the generator emits the same code but no warnings about unhidden inherited properties ([issue 1](#ki-1)).
*Workaround:* run the generator from the driver directory (as `Makefile.drv` does).

---

## Checklist for a New Generated Driver

* Directory `<type>_<name>`, definition `indigo_<type>_<name>.driver`, first device block of type `<type>`.
* `label`, `author`, `copyright`, `version`, transport, and `name` (plus `interface` for `aux`) on every device.
* `<name>_open()` / `<name>_close()` (and `<name>_match()` for libusb) in the shared `code` block, with transactional open.
* Custom properties with `X_` prefix, `label` on every property and item, documented in `PROPERTIES.md`.
* `preserve_values` for target-style numbers, `persistent` for settings, empty `on_change { }` for accept-only settings, `reject_change` instead of hand-written guards.
* No blocking in handlers; long operations via `*_finalizer`; every path ends in OK or ALERT.
* No `return` from `on_connect`/`on_disconnect`, and none from the mount motion and abort `on_change` code.
* Every entry in [Known Issues and Open Questions](#known-issues-and-open-questions) checked against the driver.
* Aborts cancel what they supersede and settle every affected property.
* Generated files regenerated, built, diffed and committed with the `.driver` file; files registered in the Xcode and Windows projects and, where applicable, the server's static driver table.
* Simulator or fake SDK and full class-standard tests; `README.md` testing record and `TEST_SUMMARY.md`.

---

## See Also

* [Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md)
* [Driver Generator Migration Guide](DRIVER_GENERATOR_MIGRATION.md)
* [Timers and Handler Queues](TIMERS_AND_QUEUES.md)
* [Driver Testing Basics](DRIVER_TESTING_BASICS.md)
* [Serial Device Simulators](SERIAL_DEVICE_SIMULATORS.md)
* [Makefiles](MAKEFILES.md)
* [indigo_tools/template.driver](../indigo_tools/template.driver) — catalogue of all generator blocks and attributes
* [indigo_tools/driver_editor](../indigo_tools/driver_editor/README.md) — browser editor with generated-source preview
* Example definitions: `indigo_drivers/aux_astromechanics` (minimal serial aux with a polling timer), `indigo_drivers/focuser_moonlite` (serial focuser with finalizer-based motion, `reject_change`, a custom `X_` property), `indigo_drivers/aux_dsusb` (libusb hot-plug, local abort and exposure properties), `indigo_drivers/mount_simulator` (virtual mount with guider device, park/home, manual motion, abort and guide pulses with `accept_while_busy`)
