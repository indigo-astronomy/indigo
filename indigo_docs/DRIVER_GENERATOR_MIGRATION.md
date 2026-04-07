# INDIGO Driver Generator — Migration Guide

Revision: 03.04.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview

The INDIGO driver generator (`indigo_generator`) is a two-way code tool that eliminates hand-written boilerplate from driver development. Instead of writing the repetitive scaffolding that every driver needs — property allocation, change dispatching, connect/disconnect handling, detach cleanup — you describe the driver in a compact `.driver` definition file and the generator produces the complete `.c`, `.h`, and `_main.c` sources.

The tool works in two directions:

* **`.driver` → `.c`** (default mode) — reads the definition file and generates all C sources.
* **`.c` → `.driver`** (`-c` / `--create` mode) — reads an existing C source file and extracts a skeleton definition file from it.

The `-c` direction is the **migration path** for drivers that were written before the generator existed.

## The Definition File Format at a Glance

A `.driver` file has this overall shape:

```c
driver <name> {
    label = "Human-readable label";
    author = "Author Name <email>";
    copyright = "Copyright notice";
    version = <integer>;
    serial;          // or libusb { ... } or hid { ... }

    define { /* #define constants */ }
    include { /* extra #include lines */ }
    data { /* extra fields added to the private_data struct */ }
    code { /* shared helper functions */ }

    <device_type> {
        name = "Device Name";
        interface = INDIGO_INTERFACE_XYZ;
        additional_instances = true;

        on_attach { /* runs during aux_attach */ }
        on_timer {
            /* Main polling loop — reads device state and updates related properties.
             * The callback is named <dev>_timer_callback (e.g. aux_timer_callback).
             * The user MUST reschedule it at the end:
             *   indigo_execute_handler_in(device, 1, <dev>_timer_callback);
             */
        }
        on_connect { /* body of the connected branch in the connection handler */ }
        on_disconnect { /* body of the disconnected branch */ }
        on_detach { /* extra cleanup before aux_detach */ }

        <type> <PROPERTY_HANDLE> {
            name = PROPERTY_NAME_PROPERTY_NAME;
            group = GROUP_CONSTANT;
            label = "Property label";
            perm = INDIGO_RO_PERM;        // optional, default RW
            rule = INDIGO_ANY_OF_MANY_RULE; // switches only
            persistent = true;             // optional
            always_defined = true;         // optional
            on_change { /* handler code */ }

            item <ITEM_HANDLE> {
                name = ITEM_NAME_ITEM_NAME;
                label = "Item label";
                value = <default>;
                min = ...; max = ...; step = ...; format = "..."; // numbers
            }
        }
    }
}
```

The `<device_type>` keyword matches the device's INDIGO class (`aux`, `wheel`, `focuser`, `ccd`, `mount`, `guider`, `rotator`, `dome`, `gps`, etc.).

Running the generator with a `.driver` file produces:

| Generated file | Contents |
|----------------|----------|
| `indigo_<type>_<name>.h` | Public header with the entry point declaration |
| `indigo_<type>_<name>.c` | Full driver implementation |
| `indigo_<type>_<name>_main.c` | Standalone executable wrapper |

The generated `.c` is marked `// This file generated from …driver` at the top and **must not be edited by hand** — all changes belong in the `.driver` file. After editing the `.driver`, re-run the generator to refresh the `.c`.

The build system picks this up automatically: `Makefile.drv` runs `indigo_generator` as a dependency of the object file whenever the `.driver` file is newer than the `.c` source.

---

## Approach 1: Write the `.driver` File from Scratch

This is the cleanest path when you are comfortable reading the existing `.c` thoroughly before writing the definition.

### Step-by-step

**1. Study the existing driver**

Read the `.c` file and note:

* What preprocessor `#define` constants it uses.
* What extra `#include` files it needs beyond the standard set.
* What extra fields are stored in the private data struct.
* Which helper functions are shared (`code`) vs. device-specific (`<device>.code`).
* The `on_attach` initialisation, including `INFO_PROPERTY->count` and default values.
* The connection / disconnection sequence (`on_connect` / `on_disconnect`).
* The timer callback body (`on_timer`).
* Every property: type (`switch`, `number`, `text`, `light`), name constant, group, label, permission, rule, items with their defaults, ranges, formats, and any `on_change` handler code.

**2. Write the `.driver` file**

Follow the format above. For the `serial` connection type, write `serial;`. If the device is identified by port pattern (vendor, product, or vid/pid) you can add a `serial { pattern { ... } }` block. Refer to existing `.driver` files in `indigo_drivers/` for examples.

Name the open/close helper functions `<driver_name>_open` and `<driver_name>_close`. The generator looks for functions with exactly these names in the `code` block and wires them into the connection handler automatically. For example, for driver `svbpowerbox`, name them `svbpowerbox_open` and `svbpowerbox_close`.

**3. Run the generator**

```sh
cd indigo_drivers/<driver_dir>
../../build/bin/indigo_generator indigo_<type>_<name>.driver
```

This overwrites (or creates) the `.c`, `.h`, and `_main.c` files in the same directory.

**4. Compile and test**

```sh
make -f ../../Makefile.drv
```

Check for compilation errors. Warnings about `snprintf` output truncation into 128-byte label fields are pre-existing and expected; no errors should appear.

**5. Clean up the directory**

Once the `.driver` file is in place and the generator runs cleanly, the old hand-written `.c` is replaced by the generated one. Keep only:

* The `.driver` file (the new source of truth).
* The generated `.c`, `.h`, `_main.c` (built artefacts, but committed).
* Any supporting files (`PROTOCOL.md`, simulator, etc.).

**Example driver**: `indigo_drivers/aux_wbplusv3/indigo_aux_wbplusv3.driver`

---

## Approach 2: Annotate the Existing `.c` and Extract with `-c`

This is the faster path when you do not want to write the full `.driver` by hand. You annotate the existing C source with structured comments that mark the code blocks the generator needs, then ask the generator to extract them into a skeleton `.driver` file.

### Annotation Comment Syntax

Annotations have the form:

```c
//+ <section_id>
... code to be extracted ...
//- <section_id>
```

The `//+` comment opens a section and `//-` closes it. The generator uses the `<section_id>` to decide where the code block goes in the `.driver` file.

### Available Section IDs

| Section ID | Where it goes in `.driver` | Typical location in `.c` |
|------------|---------------------------|--------------------------|
| `define` | `define { }` block | Near the top, after constant `#define`s |
| `include` | `include { }` block | Extra `#include` lines |
| `data` | `data { }` block | Inside the private_data `typedef struct` |
| `code` | Top-level `code { }` block | Shared helper functions |
| `<dev>.code` | `code { }` inside the device block | Device-specific helpers |
| `<dev>.on_attach` | `on_attach { }` in the device block | Body of `aux_attach` |
| `<dev>.on_timer` | `on_timer { }` in the device block | Body of the periodic timer callback (`<dev>_timer_callback`). This is the main polling loop that reads device state and updates properties. **The user must reschedule it at the end** with `indigo_execute_handler_in(device, 1, <dev>_timer_callback)`. |
| `<dev>.on_connect` | `on_connect { }` in the device block | Connected branch of the connection handler |
| `<dev>.on_disconnect` | `on_disconnect { }` in the device block | Disconnected branch |
| `<dev>.on_detach` | `on_detach { }` in the device block | Body of `aux_detach` |
| `<dev>.<PROP>.on_change` | `on_change { }` inside the matching property block | Inside the matching `*_handler` function |
| `<dev>.<PROP>.on_attach` | `on_attach { }` inside the matching property block | Property-specific attach initialisation |

`<dev>` is the lowercase device class (`aux`, `wheel`, `focuser`, …).
`<PROP>` is the property handle **without** the `_PROPERTY` suffix (e.g. `AUX_OUTLET_NAMES`, `WHEEL_SLOT`).

### Minimal Annotation Example

Below is a fragment of a hand-written driver with annotations added for migration:

```c
/* in the private_data typedef struct: */
typedef struct {
    indigo_uni_handle *handle;
    //+ data
    int slot_count;
    int current_slot;
    //- data
} mywheel_private_data;

/* shared low-level helpers: */
//+ code
static bool mywheel_command(indigo_device *device, const char *cmd, char *response) {
    ...
}
//- code

/* device-level code (device-specific helpers): */
//+ wheel.code
static bool mywheel_get_slot(indigo_device *device) {
    ...
}
//- wheel.code

/* in aux_attach, the custom on_attach part: */
static indigo_result aux_attach(indigo_device *device) {
    if (indigo_wheel_attach(...) == INDIGO_OK) {
        //+ wheel.on_attach
        WHEEL_SLOT_PROPERTY->count = 8;
        //- wheel.on_attach
        ...
    }
}

/* in the connection handler, connected branch: */
//+ wheel.on_connect
PRIVATE_DATA->handle = mywheel_open(DEVICE_PORT_ITEM->text.value);
if (PRIVATE_DATA->handle) {
    mywheel_get_slot(device);
    ...
}
//- wheel.on_connect

/* in the connection handler, disconnected branch: */
//+ wheel.on_disconnect
mywheel_close(device);
//- wheel.on_disconnect

/* in the timer callback body — the generated function is named wheel_timer_callback: */
    //+ wheel.on_timer
    if (mywheel_get_slot(device)) {
        WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
        indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
    }
    // Reschedule the timer — this line is REQUIRED at the end of on_timer:
    indigo_execute_handler_in(device, 1, wheel_timer_callback);
    //- wheel.on_timer

/* in the WHEEL_SLOT change handler: */
    //+ wheel.WHEEL_SLOT.on_change
    mywheel_set_slot(device, (int)WHEEL_SLOT_ITEM->number.target);
    //- wheel.WHEEL_SLOT.on_change
```

### Step-by-step

**1. Add annotations to the existing `.c` file**

Place `//+` / `//-` pairs around every code region that belongs to a named section.
You do not need to annotate everything at once — the generator uses what it finds and leaves the rest as placeholders in the output.

**2. Run the generator with `-c`**

```sh
cd indigo_drivers/<driver_dir>
../../build/bin/indigo_generator -c indigo_<type>_<name>.driver
```

The generator reads `indigo_<type>_<name>.c` (the `.driver` extension is swapped to `.c` to find the source), extracts all annotated sections, scans `indigo_init_*_property` and `indigo_init_*_item` calls for property skeletons, and writes the `.driver` file.

**3. Review and complete the `.driver` file**

The extracted `.driver` will be mostly correct but may need manual completion:

* Property `perm`, `rule`, `persistent`, `always_defined` flags — set them if the defaults are wrong.
* Item `value`, `min`, `max`, `step`, `format` — the generator extracts what it can from init calls, but complex expressions may be left as `0`.
* The `name`, `interface`, `additional_instances` fields on the device block need to be verified.
* Connection type (`serial;`, `libusb { }`, etc.) needs to be added if it was not detected.

**4. Regenerate the C sources**

```sh
../../build/bin/indigo_generator indigo_<type>_<name>.driver
```

From this point forward the `.driver` file is the source of truth and the `.c` is generated. Do not edit the `.c` by hand — make all changes in the `.driver` file and regenerate.

**5. Compile and verify**

```sh
make -f ../../Makefile.drv
```

---

## Choosing an Approach

| Situation | Recommended approach |
|-----------|----------------------|
| New driver being written from scratch | **Approach 1** (write `.driver` directly) |
| Large, complex existing driver with intricate connection logic | **Approach 1** (read and rewrite fully) |
| Simple existing driver with clear separation of concerns | **Approach 2** (annotate and extract) |
| Quick first pass, intending to clean up later | **Approach 2** (annotate and extract), then review and tighten |

In practice, **Approach 2** is a good way to get started quickly. After extraction you may still want to restructure or clean up the resulting `.driver`, but the annotation pass guarantees that no code is lost and the first generated `.c` is functionally equivalent to the original.

---

## What the Generator Does and Does Not Do

### What it handles automatically

* All property allocation, registration, deletion, and release.
* The `aux_attach` / `aux_detach` / `aux_enumerate_properties` / `aux_change_property` scaffolding.
* The `indigo_execute_handler` dispatch for every `on_change` block.
* The connection handler structure including the `svbpowerbox_open` / `svbpowerbox_close` calls (functions with the `<driver_name>_open` / `<driver_name>_close` naming convention are wired in automatically).
* The `indigo_execute_handler_in(device, 1, aux_timer_callback)` loop.
* `DRIVER_VERSION`, `DRIVER_NAME`, `DRIVER_LABEL`, and the `indigo_<type>_<name>()` entry point.
* `indigo_safe_malloc`, `indigo_attach_device`, `INDIGO_DRIVER_INIT/SHUTDOWN/INFO` lifecycle.
* `_main.c` for standalone driver executables.

### What you still write

* All actual device communication (open/close/read/write, protocol framing, checksums).
* The body of `on_timer`, `on_connect`, `on_disconnect`, `on_attach`.
* The body of every `on_change` handler.
* Any extra helper functions in `code` or `<dev>.code`.
* Extra fields in `data`.
* Extra `#define` constants in `define`.

### Special handling

Four mount-specific properties receive special treatment in the generated change handlers: MOUNT_EQUATORIAL_COORDINATES, MOUNT_MOTION_RA, MOUNT_MOTION_DEC, and MOUNT_TRACKING.
For each of these, the generator inserts a park-state guard at the very top of the handler — before any user-supplied on_change code runs.
The guard checks whether MOUNT_PARK_PROPERTY is active and the mount is in the parked state; if so, it sends an alert message ("Mount is parked!"), sets the property state to INDIGO_ALERT_STATE, and returns immediately.
This ensures that slewing, tracking, or motion commands are silently rejected while the mount is parked, without requiring the driver author to replicate this logic manually.

MOUNT_EQUATORIAL_COORDINATES has an additional distinction in how the handler is finalised.
For every other property the generator appends a call to indigo_update_property() at the end of the handler (unless a custom _finalizer is detected in the on_change block).
For MOUNT_EQUATORIAL_COORDINATES this call is replaced by indigo_update_coordinates(), which performs the standard INDIGO coordinate update and notification sequence appropriate for equatorial position reporting.
This means a driver author does not need to call indigo_update_coordinates() explicitly inside on_change — the generator guarantees it is always the final step of the coordinates handler.

---

## Tips and Caveats

* **Do not edit generated `.c` files.** The next `make` will overwrite them. All changes go in the `.driver` file.
* **Naming convention for open/close.** Name the functions `<driver_name>_open(device)` → `bool` and `<driver_name>_close(device)` → `void`. This is the convention the generator recognises and wires into the connection handler.
* **One device block per device class.** If the driver exposes both a focuser and a rotator (like `indigo_focuser_primaluce`), there are two device blocks in the `.driver` file, each with its own `on_connect`, `on_timer`, properties, etc.
* **Property handle vs. property name.** In `//+` markers, use the C handle **without** `_PROPERTY` (e.g. `WHEEL_SLOT`, not `WHEEL_SLOT_PROPERTY`).
* **Re-running `-c` overwrites the `.driver` file.** Keep it under version control before running with `-c` again.
* **Build integration.** The `Makefile.drv` already runs `indigo_generator` automatically when the `.driver` file is newer than the `.c` file. No extra make target is needed.

---

## See Also

* [INDIGO Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md) — overview of the INDIGO driver model.
* `indigo_tools/indigo_generator.c` — the generator source.
* `indigo_drivers/aux_wbplusv3/` — reference driver fully managed by the generator.
* `indigo_drivers/wheel_trutek/` — example of annotation markers (`//+` / `//-`) in a converted driver.
* `indigo_drivers/focuser_primaluce/` — multi-device driver with both `focuser` and `rotator` blocks and `on_connect` / `on_disconnect` annotations.

---

Clear skies!
