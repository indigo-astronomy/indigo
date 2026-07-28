# INDIGO Scripting Guide
Revision: 27.07.2026 (v1)

Author: **Rumen Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Introduction

INDIGO scripting allows custom code to run **inside** an INDIGO bus instance (an *indigo_server* process, or any application that embeds the *agent_scripting* driver) and to read, watch and manipulate INDIGO properties there, i.e. to drive devices, other agents and even the server itself. The engine that interprets and executes the scripts is [Duktape](https://duktape.org), an [ECMAScript 5 / ES5](https://262.ecma-international.org/5.1/) (JavaScript) engine, with a set of custom bindings that connect the script to the INDIGO **bus**.

This guide explains the scripting *language* and the INDIGO scripting *API* on their own, without assuming any particular client application is used to write, store or run the scripts. Scripts live on the **Scripting Agent** as ordinary INDIGO properties (see [Properties Defined by the Scripting Agent](#properties-defined-by-the-scripting-agent)), so any INDIGO client capable of changing a property - the command line tool *indigo_prop_tool*, INDIGO Control Panel, INDIGO Dashboard, a custom client written against the [Client Development Basics](CLIENT_DEVELOPMENT_BASICS.md), or a dedicated script editor - can create, run, and delete scripts. This document uses *indigo_prop_tool* for its examples only because it is the simplest, universally available tool; nothing in this guide depends on it.

Before reading further it helps to be familiar with the core INDIGO concepts (bus, device, client, agent, property, item, state, permission) described in [PROPERTIES.md](PROPERTIES.md) and [Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md). This guide repeats the concepts that matter for scripting, but does not repeat the full driver API.

The reference implementation of the Scripting Agent lives in [indigo_drivers/agent_scripting](../indigo_drivers/agent_scripting), whose [README.md](../indigo_drivers/agent_scripting/README.md) also contains a low level API listing and example scripts.

## How Scripting Fits into INDIGO

INDIGO is a platform for communication between software entities over a software **bus**. Entities are either **devices** or **clients**; **agents** act as both at once. A script is executed by the **Scripting Agent**, which is itself an agent: from the outside it looks like a device (it has properties that can be defined/updated/changed), and from the inside, towards the rest of the bus, it behaves like a client (it can enumerate, watch and change any property visible on the same bus instance).

This has a direct, practical consequence: **a script can see every device, agent and property that lives on the same bus instance as the Scripting Agent** 

This means that a script is *not* limited to a single physical host. INDIGO bus instances can be linked hierarchically: a server or application can itself connect, as a client, to one or more remote INDIGO services (for example a remote *indigo_server*, discovered on the network or added by host/port). Once such a remote service is connected, its devices are mirrored onto the local bus and become reachable by the script exactly like local ones, simply addressed with the `"<device name> @ <service name>"` naming convention, e.g. `indigo_devices["CCD Imager Simulator @ indigosky"]`, or `devices: [ "CCD Imager Simulator @ indigosky" ]` in an event handler. A script cannot establish that remote link itself, only address existing devices - the local service must already be connected to the remote service before the script can see anything from it.

A single process (server or application) can host more than one Scripting Agent instance; each is a completely independent embedded Duktape context (its own set of loaded scripts, global variables, timers and event handlers).

All variables declared at the top level of a script (outside any function or code block) are **global** to the Duktape context that runs them, i.e. they are shared with every other script running in the *same* Scripting Agent instance. This is a deliberate and useful feature - it lets a set of cooperating scripts exchange data cheaply - but it is also the most common source of "impossible" bugs, usually caused by two independently written scripts, or two copies of the same script, using the same global variable name.

INDIGO, and script execution inside it, is fully **asynchronous**. There is no blocking wait or "sleep" primitive. A script statement that starts executing runs to completion and returns immediately; if something must happen later (a property changing state, a fixed delay passing) the script must arrange to be called again through an **event handler** or a **timer** (see [Script Execution Model](#script-execution-model)). Non-trivial scripts are therefore usually written as a small state machine implemented as a cascade of event handler callbacks.

## The Scripting Language

### The engine

The language is [ECMAScript 2009 / ES5](https://262.ecma-international.org/5.1/), the same standard used by most "classic" JavaScript engines. The full language reference is not repeated here; only the constructs that occur most often in INDIGO scripts are listed below. For anything not covered, any general purpose JavaScript / ES5 reference applies, keeping in mind that Duktape is not a browser engine: there is no DOM, no `window`, no `console`, etc. - all interaction with the outside world goes through the *indigo_...* functions described further down.

### Statements and comments

Executable statements are terminated with a semicolon:
```JS
indigo_log("This is a statement");
```
Comments are introduced with `//` (rest of line) or `/* ... */` (block):
```JS
// a single line comment
/* a
   multi-line
   comment */
```

### Quotes and hyphens

String values can be quoted with either `'` or `"`. Be careful when pasting text copied from a word processor or a web page - "smart quotes" are not valid ECMAScript quote characters and will cause a syntax error; retype the quotes if this happens.

Hyphens (`-`) are not allowed in identifiers, since the hyphen is the subtraction operator. Use underscores instead, e.g. `my_variable`. INDIGO itself, including the low level API and the Script based Sequencer, consistently names things with `underscore_case` rather than `camelCase`; this guide follows the same convention, and so should your scripts.

### Declaring variables

```JS
var my_count = 3;
var my_number = 10.4;
var my_string = "this is a string";
var my_boolean = true;
```
A variable declared at the top level of a script is global, as explained above. A variable declared inside a code block (a function body, an `if`, a `for`, ...) is local to that block.

### Code blocks and functions

Statements are grouped into code blocks with curly braces. Functions are the most common place code blocks are used:
```JS
function my_function(a, b) {
	var product = a * b; // local variable
	return product;
}

var result = my_function(2, 3);
indigo_log("Result = " + String(result));
```

### Conditional execution

```JS
if (condition) {
	// ...
} else if (other_condition) {
	// ...
} else {
	// ...
}
```
```JS
switch (value) {
	case 1:
		// ...
		break;
	case 2:
		// ...
		break;
	default:
		// ...
}
```

### Comparisons and logical operators

| Operator | Meaning |
| --- | --- |
| `==` | equal to (with type coercion) |
| `===` | equal value and type |
| `!=` | not equal |
| `!==` | not equal value or not equal type |
| `>`, `<`, `>=`, `<=` | ordering |
| `!` | logical not |
| `&&`, `\|\|` | logical and / or |

Always prefer `===` and `!==` over `==` and `!=` unless the type coercion is intentional; INDIGO property values arrive from the bus with specific types (string, number, boolean) and comparing them against the wrong JavaScript type is a common source of subtle bugs.

### Loops

```JS
for (var i = 0; i < 3; i++) {
	indigo_log(String(i));
}
```
```JS
var i = 0;
while (i < 3) {
	indigo_log(String(i));
	i++;
}
```
Loops must terminate quickly. Because the script engine and the bus callbacks share the same execution context, an endless (or very long running) loop freezes the whole Scripting Agent, and possibly the hosting server/application, requiring a restart. Never rely on a loop to "wait" for something - use a timer or an event handler instead.

### Numeric operators

```JS
c = a + b;  // addition
c = a - b;  // subtraction
c = a * b;  // multiplication
c = a / b;  // division
c = a % b;  // modulus
c = a++;    // increment
c = a--;    // decrement
```

### Strings, Math and Date

```JS
String(1.05);              // "1.05"
(1.05).toFixed(2);         // "1.05"
Number("1.05") * 2;        // 2.1
"TESTING".length;          // 7
"TESTING".substr(0, 3);    // "TES"

Math.abs(-1.05);           // 1.05
Math.floor(1.55);          // 1
Math.round(1.55);          // 2
Math.PI;                   // 3.141592653589793

var now = Date.now();                              // current time, ms since epoch
var then = Date.parse("2026-06-30 16:06:01+02:00"); // a specific moment
var seconds = (now - then) / 1000;
```

## Script Execution Model

### Timers

A timer runs a function once, after a given delay expressed in seconds:
```JS
var my_timer;

function on_timer() {
	indigo_log("my_timer has triggered");
}

my_timer = indigo_set_timer(on_timer, 5);
```
To repeat, re-arm the timer from inside the callback; to stop, simply do not re-arm it, or cancel it explicitly:
```JS
indigo_cancel_timer(my_timer);
```
A small periodic task, executed 3 times every 5 seconds and then stopping itself, looks like this:
```JS
var my_timer;
var count = 0;

function on_timer() {
	indigo_log("tick " + String(++count));
	if (count < 3) {
		my_timer = indigo_set_timer(on_timer, 5);
	}
}

my_timer = indigo_set_timer(on_timer, 5);
```

### Event handlers

An event handler reacts to bus events coming from one or more named devices (which can also be agents, or the server itself). It is registered by adding a member object to the global `indigo_event_handlers` registry:
```JS
indigo_event_handlers.my_handler = {
	devices: [ "GPS Simulator" ],   // only events from these devices are delivered

	on_define: function(property) {
		// a NEW property has just been defined by "GPS Simulator"
	},
	on_update: function(property) {
		// an existing property of "GPS Simulator" changed value or state
	},
	on_delete: function(property) {
		// a property of "GPS Simulator" is gone
	},
	on_message: function(message) {
		// a message was broadcast by "GPS Simulator" (see Messages below)
	},
};
```
`property` is a JavaScript object with fields `device`, `name`, `label`, `state` ("Idle"/"Ok"/"Busy"/"Alert"), `perm` ("RW"/"RO"/"WO"), `message` and `items` (an object keyed by item name), plus a `change(items)` method, described in [Updating (Changing) Properties](#updating-changing-properties).

Two additional handlers exist for scripts that define their own custom properties (see [Defining Custom Properties](#defining-custom-properties)):
```JS
indigo_event_handlers.my_handler.on_enumerate_properties = function(property) {
	// somebody asked to (re-)enumerate properties; property.device / property.name
	// may be null (meaning "everything"), or restrict the request
};
indigo_event_handlers.my_handler.on_change_property = function(property) {
	// a client asked to change a value of a property THIS script has defined
};
```
An agent that hosts the Scripting Agent normally asks every client (including every script) to (re-)announce its properties once, right after the script is loaded; if a script defines custom properties in reaction to `on_enumerate_properties`, it should also call `indigo_enumerate_properties()` once itself right after registering the handler, to trigger that first announcement (see the complete example further down).

A handler is removed with the ordinary JavaScript `delete` operator, most often at the very end of an `on_update` callback that only needed to fire once:
```JS
delete indigo_event_handlers.my_handler;
```
Since script execution is asynchronous and the order in which properties are defined or updated is never guaranteed, always check `property.state` before trusting or changing `property.items` (see [State Descriptions and State Transitions](DRIVER_DEVELOPMENT_BASICS.md#state-descriptions-and-state-transitions) for the full rules) - values are only reliable while the state is `"Ok"`, and are transient (but not garbage) while it is `"Busy"`.

## Reading INDIGO Properties

Every device known to the running Scripting Agent is represented as an entry in the global `indigo_devices` object, keyed by device name:
```JS
indigo_devices["Server"]              // the server process itself
indigo_devices["Unihedron SQM"]       // a device driver
indigo_devices["Imager Agent"]        // an agent
```
A device entry only appears **after** its properties have actually been defined on the bus and observed by the script (i.e. after an `on_define` event has fired for it, directly or through `indigo_enumerate_properties()`). Do not assume `indigo_devices["some device"]` exists immediately after the script starts; either use an event handler, or enumerate properties first and process them from `on_define`.

A property is reached with `device.PROPERTY_NAME`, an item with `.items.ITEM_NAME`:
```JS
if (indigo_devices["Unihedron SQM"].AUX_INFO.state == "Ok") {
	var sqm = indigo_devices["Unihedron SQM"].AUX_INFO.items.SKY_BRIGHTNESS;
	indigo_log("SQM value: " + String(sqm));
}
```
As explained in [PROPERTIES.md](PROPERTIES.md), a property has a **type** (`TEXT_VECTOR`, `NUMBER_VECTOR`, `SWITCH_VECTOR`, `LIGHT_VECTOR`, `BLOB_VECTOR`), a **state** (`Idle`/`Ok`/`Busy`/`Alert`) and a **permission** (`RO`/`RW`/`WO`). The scripting API does not expose the type explicitly - it is implied by the JavaScript type of the item values (`string`, `number`, `boolean`, or a light state string) - but the same rules about state and permission apply as for any other INDIGO client.

## Updating (Changing) Properties

Reading is trivial; every property object obtained through an event handler or through `indigo_devices` carries a `change()` method that requests new item values from whoever owns the property (a driver, another agent, or another script):
```JS
if (indigo_devices["GPS Simulator"].CONNECTION.state == "Ok") {
	indigo_devices["GPS Simulator"].CONNECTION.change({ CONNECTED: true });
}
```
```JS
indigo_devices["Imager Agent"].CCD_SET_FITS_HEADER.change({ KEYWORD: "MYSTUFF", VALUE: "some text" });
```
For a `SWITCH_VECTOR` with the `ONE_OF_MANY` rule, setting one item to `true` implicitly clears the others (exactly as it would with any other INDIGO client); for `ANY_OF_MANY` switches (independent checkboxes) each item is toggled independently. Only properties with permission `RW` or `WO` can be changed, and never while their state is already `Busy` (the change would either be rejected or queued, depending on the target driver).

Under the hood `change()` calls one of these low level functions, selected automatically from the JavaScript type of the values:
```JS
function indigo_change_text_property(device_name, property_name, items)
function indigo_change_number_property(device_name, property_name, items)
function indigo_change_switch_property(device_name, property_name, items)
```
`LIGHT_VECTOR` and `BLOB_VECTOR` properties cannot be changed by a client (lights are always read-only; BLOBs are produced by the device, see [BLOBs](#blobs)).

## Defining Custom Properties

A script can define its own properties, exactly as a driver would, to expose data or controls to every other client and script attached to the same bus instance. This is the recommended way to exchange information between independent scripts or between a script and an external client - **preferred over sending messages**, which are meant for human readable, unstructured text only (see [Messages](#messages)).

```JS
function indigo_define_text_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, message)
function indigo_define_number_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, message)
function indigo_define_switch_property(device_name, property_name, property_group, property_label, items, item_defs, state, perm, rule, message)
function indigo_define_light_property(device_name, property_name, property_group, property_label, items, item_defs, state, message)
```
- `items` is an object with the **initial values** keyed by item name (a string for text, a number for number, a boolean for switch, a state string for light).
- `item_defs` is an object, also keyed by item name, giving the per-item metadata:

| Field | Applies to | Meaning |
| --- | --- | --- |
| `label` | all types | human readable item label |
| `format` | number only | `printf`-style display format, e.g. `"%m"` for sexagesimal |
| `min`, `max`, `step` | number only | allowed range and increment |

- `state` is one of `"Idle"`, `"Ok"`, `"Busy"`, `"Alert"` (case insensitive).
- `perm` is one of `"RW"`, `"RO"`, `"WO"` (default `"RW"` if omitted/unrecognized).
- `rule` (switches only) is one of `"ONE_OF_MANY"`, `"AT_MOST_ONE"`, `"ANY_OF_MANY"` (default `"ANY_OF_MANY"`).
- `message` is an optional human readable text sent along with the definition, or `null`.

A property is normally (re-)defined from the `on_enumerate_properties` handler, so that it is announced whenever a client (or the server itself, at startup) asks for it:
```JS
indigo_event_handlers.my_handler = {
	latitude: 0,
	longitude: 0,

	on_enumerate_properties: function(property) {
		if (property.device == "Scripting Agent" && (property.name == null || property.name == "MY_COORDS")) {
			indigo_define_number_property(
				"Scripting Agent", "MY_COORDS", "My GPS", "Coordinates",
				{ LATITUDE: this.latitude, LONGITUDE: this.longitude },
				{ LATITUDE: { label: "Latitude", format: "%m", min: -90, max: 90 },
				  LONGITUDE: { label: "Longitude", format: "%m", min: -180, max: 360 } },
				"Ok", "RO", null
			);
		}
	}
};

indigo_enumerate_properties(); // trigger the first announcement immediately
```
`indigo_redefine_text_property`, `indigo_redefine_number_property`, `indigo_redefine_switch_property` and `indigo_redefine_light_property` have the exact same signatures and are used instead of the `define` variants when the **item set**, group or label of an *already defined* property must change; unlike `update`, they delete and re-issue the property definition, so clients are told to forget the old shape.

## Updating Properties You Defined

Once a custom property is defined, its values and state are changed with:
```JS
function indigo_update_text_property(device_name, property_name, items, state, message)
function indigo_update_number_property(device_name, property_name, items, state, message)
function indigo_update_switch_property(device_name, property_name, items, state, message)
function indigo_update_light_property(device_name, property_name, items, state, message)
```
`items` only needs to contain the item(s) whose value actually changed; omitted items keep their previous value. Always send an explicit `state`, following the normal INDIGO [state transition rules](DRIVER_DEVELOPMENT_BASICS.md#state-descriptions-and-state-transitions) (`Ok` -> `Busy` -> `Ok`/`Alert`, etc.) - this is exactly as important for a script-defined property as it is for a driver.

A client that wants to change a script defined property calls `.change()` on it exactly as on any other property; the script is notified through the `on_change_property` handler:
```JS
indigo_event_handlers.my_handler.on_change_property = function(property) {
	if (property.name == "MY_COORDS") {
		this.latitude = property.items.LATITUDE;
		this.longitude = property.items.LONGITUDE;
		// ... act on the new values ...
		indigo_update_number_property("Scripting Agent", "MY_COORDS",
			{ LATITUDE: this.latitude, LONGITUDE: this.longitude }, "Ok", null);
	}
};
```
Always send back a value for **every** item of the property; item names left out of the `items` object passed to `on_change_property`'s reply are **not** automatically preserved by the client that requested the change, and appear to it as `undefined` if the script does not answer with a full property.

## Deleting Properties You Defined

```JS
function indigo_delete_property(device_name, property_name, message)
```
Use `property_name = null` to delete every property owned by `device_name` at once. A property a script has defined is not automatically deleted when the script that defined it finishes running (top level code in a script is executed once and then returns like any other function call) - it stays defined until the Scripting Agent unloads or another script/event explicitly deletes it. Custom properties are **not persistent** across Scripting Agent restarts, so a script that defines them should normally be marked to execute on agent load (see [AGENT_SCRIPTING_ON_LOAD_SCRIPT](#agent_scripting_on_load_script)) if the property must be available every time the server/application starts.

## Messages

```JS
function indigo_send_message(message)
```
broadcasts a plain text message on the bus; a receiver, in another script or a connected client, watches for it with the `on_message` handler bound to the sending device:
```JS
indigo_event_handlers.my_handler = {
	devices: [ "Server" ],
	on_message: function(message) {
		indigo_log("received: " + message);
	}
};
```
A message received through `on_message` is always prefixed with the sender's device name (`"<device>: <text>"`), so exact matching should generally be done with `message.endsWith(...)` rather than `==`. Messages are meant for presenting unstructured, human readable information; they are **not** delivered reliably enough, nor structured enough, to be used as a control channel between scripts - use a custom (shared) property instead, as shown above.

## BLOBs

BLOB properties (raw image data and similar binary payloads) are handled specially, since their content is normally far too large to be represented as ordinary JavaScript values:
```JS
function indigo_enable_blob(device_name, property_name, state)
function indigo_populate_blob(blob_item)
function indigo_save_blob(file_name, blob_item)
```
A script must call `indigo_enable_blob(device, property, true)` before a driver will actually send BLOB data for that property to it (this mirrors the `enableBLOB`/BLOB mode behaviour of every other INDIGO/INDI client, and avoids flooding scripts that never asked for image data). Once enabled and the property has updated, `indigo_populate_blob()` makes the data available in memory for further processing, and `indigo_save_blob()` writes it directly to a file. See the driver/agent README files for the exact BLOB property involved (for example `CCD_IMAGE` on a camera, or `CCD_IMAGE_FILE` in the Imager Agent).

## Time and Date Helpers

```JS
function indigo_dtos(value, format);   // number  -> formatted string, e.g. sexagesimal
function indigo_stod(value);           // formatted string -> number
function indigo_utc_to_time(utc);      // UTC ISO string -> local time-of-day (hours)
function indigo_utc_to_delay(utc);     // UTC ISO string -> delay in seconds from now
function indigo_time_to_delay(time);   // local time-of-day (hours) -> delay in seconds from now
function indigo_delay_to_utc(delay);   // delay in seconds from now -> UTC ISO string
function indigo_time_to_utc(time);     // local time-of-day (hours) -> UTC ISO string
```
These are convenience wrappers used when scheduling timers against a wall clock time (for example "run 30 minutes before sunset") rather than a fixed delay; combine them with `indigo_set_timer`.

## Low Level API Quick Reference

| Function | Purpose |
| --- | --- |
| `indigo_error(message)` | log with level *error* |
| `indigo_log(message)` | log with level *info* |
| `indigo_debug(message)` | log with level *debug* |
| `indigo_trace(message)` | log with level *trace* |
| `indigo_log_with_property(message, property)` | log a message followed by a full dump of a property |
| `indigo_send_message(message)` | broadcast a text message |
| `indigo_enumerate_properties(device_name, property_name)` | ask for (re-)definition of matching properties; both arguments optional/`null` |
| `indigo_enable_blob(device_name, property_name, state)` | enable/disable BLOB delivery |
| `indigo_save_blob(file_name, blob_item)` | save a BLOB item to a file |
| `indigo_populate_blob(blob_item)` | load a BLOB item into memory |
| `indigo_change_text_property(device_name, property_name, items)` | request a text property change |
| `indigo_change_number_property(device_name, property_name, items)` | request a number property change |
| `indigo_change_switch_property(device_name, property_name, items)` | request a switch property change |
| `indigo_define_text_property(...)` / `indigo_redefine_text_property(...)` | define / redefine a text property |
| `indigo_define_number_property(...)` / `indigo_redefine_number_property(...)` | define / redefine a number property |
| `indigo_define_switch_property(...)` / `indigo_redefine_switch_property(...)` | define / redefine a switch property |
| `indigo_define_light_property(...)` / `indigo_redefine_light_property(...)` | define / redefine a light property |
| `indigo_update_text_property(device_name, property_name, items, state, message)` | update a text property this script owns |
| `indigo_update_number_property(device_name, property_name, items, state, message)` | update a number property this script owns |
| `indigo_update_switch_property(device_name, property_name, items, state, message)` | update a switch property this script owns |
| `indigo_update_light_property(device_name, property_name, items, state, message)` | update a light property this script owns |
| `indigo_delete_property(device_name, property_name, message)` | delete a property this script owns |
| `indigo_dtos(value, format)` / `indigo_stod(value)` | number <-> formatted string |
| `indigo_utc_to_time` / `indigo_utc_to_delay` / `indigo_time_to_delay` / `indigo_delay_to_utc` / `indigo_time_to_utc` | time/date conversions |
| `indigo_set_timer(function, delay)` | schedule `function` to run once, `delay` seconds from now |
| `indigo_cancel_timer(timer)` | cancel a pending timer |

Callback style functions, called *by* INDIGO into the script if a global function of that name exists (already implemented for you by `boot.js`, see [indigo_event_handlers](#event-handlers) instead of using these directly):
```
function indigo_on_define_property(device_name, property_name, property_label, items, item_defs, state, perm, message)
function indigo_on_update_property(device_name, property_name, items, state, message)
function indigo_on_delete_property(device_name, property_name, message)
function indigo_on_send_message(device_name, message)
function indigo_on_enumerate_properties(device_name, property_name)
function indigo_on_change_property(device_name, property_name, items, state)
```

## Properties Defined by the Scripting Agent

The Scripting Agent manages the whole lifecycle of scripts - creating, running, deleting them, and choosing which run automatically - purely through INDIGO properties, so it can be fully driven by *any* INDIGO client, without a dedicated script editor. All control properties below live in the `Agent` group; every stored script additionally gets its own property in the `Scripts` group.

| Property | Type | Perm | Items | Purpose |
| --- | --- | --- | --- | --- |
| `AGENT_SCRIPTING_RUN_SCRIPT` | text | RW | `SCRIPT` | Run the given code once, immediately. Not persisted, has no name and does not appear in any other property. |
| `AGENT_SCRIPTING_ADD_SCRIPT` | text | RW | `NAME`, `SCRIPT` | Create and persist a new named script. Fails (state `Alert`) if `NAME` is empty or already used. |
| `AGENT_SCRIPTING_EXECUTE_SCRIPT` | switch (`ONE_OF_MANY`) | RW | one per stored script | Turning an item on runs the corresponding stored script once. |
| `AGENT_SCRIPTING_DELETE_SCRIPT` | text | RW | `NAME` | Deletes the stored script whose name matches `NAME`. |
| `AGENT_SCRIPTING_ON_LOAD_SCRIPT` | switch (`ANY_OF_MANY`) | RW | one per stored script | Scripts whose item is `true` run automatically whenever the agent's configuration is (re-)loaded - normally exactly once, when the hosting server/application starts. |
| `AGENT_SCRIPTING_ON_UNLOAD_SCRIPT` | switch (`ANY_OF_MANY`) | RW | one per stored script | Scripts whose item is `true` run automatically right before the agent is detached - normally when the hosting server/application shuts down. |
| `AGENT_SCRIPTING_SCRIPT_<n>` | text | RW | `NAME`, `SCRIPT` | One instance per stored script (`<n>` is an internal slot number assigned when the script is created and stable for its lifetime). Holds the script's persisted source code; the property label is always kept equal to the current `NAME`. |

A few implementation details worth knowing when scripting against these properties directly:
- Every item of `AGENT_SCRIPTING_EXECUTE_SCRIPT`, `AGENT_SCRIPTING_ON_LOAD_SCRIPT` and `AGENT_SCRIPTING_ON_UNLOAD_SCRIPT` is named after the corresponding `AGENT_SCRIPTING_SCRIPT_<n>` property, and labeled with the script's name; the item set is redefined automatically whenever a script is added, deleted or renamed.
- Renaming a script is done by changing the `NAME` item of its own `AGENT_SCRIPTING_SCRIPT_<n>` property; this also updates its label and the matching item labels in the three list properties above.
- Adding/deleting a script, and changing which scripts run on load/unload, is saved to the current configuration profile automatically (there is no separate `CONFIG.SAVE` step to remember).
- Up to 128 scripts can be stored per Scripting Agent instance.

## Managing Scripts With Any Client

Because script management is just property manipulation, the following *indigo_prop_tool* examples work identically from any other client - a GUI script editor is only a convenience layer on top of exactly these properties.

Run a one-off piece of code (not saved):
```
indigo_prop_tool set "Scripting Agent.AGENT_SCRIPTING_RUN_SCRIPT.SCRIPT=\"indigo_log('hello world');\""
```
Create and persist a named script:
```
indigo_prop_tool set "Scripting Agent.AGENT_SCRIPTING_ADD_SCRIPT.NAME=\"my_script\";SCRIPT=\"indigo_log('hi from my_script');\""
```
*indigo_prop_tool* also accepts a script's source directly from a file with `set_script`:
```
indigo_prop_tool set_script "Scripting Agent.AGENT_SCRIPTING_ADD_SCRIPT.SCRIPT=my_script.js;NAME=my_script"
```
List the stored scripts and find out the internal property name assigned to one of them:
```
indigo_prop_tool list -e "Scripting Agent.AGENT_SCRIPTING_EXECUTE_SCRIPT"
```
Run a stored script by turning its switch item on (use the item name discovered above, e.g. `AGENT_SCRIPTING_SCRIPT_0`):
```
indigo_prop_tool set "Scripting Agent.AGENT_SCRIPTING_EXECUTE_SCRIPT.AGENT_SCRIPTING_SCRIPT_0=ON"
```
Mark a stored script to run automatically when the agent (re)loads:
```
indigo_prop_tool set "Scripting Agent.AGENT_SCRIPTING_ON_LOAD_SCRIPT.AGENT_SCRIPTING_SCRIPT_0=ON"
```
Delete a stored script by name:
```
indigo_prop_tool set "Scripting Agent.AGENT_SCRIPTING_DELETE_SCRIPT.NAME=\"my_script\""
```

## A Complete Example

The following script, independent of any particular client, defines a shared `MY_COORDS` property that mirrors the coordinates reported by a GPS device, and cleans up correctly:
```JS
indigo_event_handlers.gps_bridge = {
	latitude: 0,
	longitude: 0,
	ready: false,

	process_coordinates: function(property) {
		if (property.state == "Ok") {
			this.latitude = property.items.LATITUDE;
			this.longitude = property.items.LONGITUDE;
			this.ready = true;
			indigo_update_number_property("Scripting Agent", "MY_COORDS",
				{ LATITUDE: this.latitude, LONGITUDE: this.longitude }, "Ok", null);
		}
	},

	on_define: function(property) {
		if (property.name == "CONNECTION" && !property.items.CONNECTED) {
			property.change({ CONNECTED: true }); // connect the GPS as soon as we see it
		} else if (property.name == "GEOGRAPHIC_COORDINATES") {
			this.process_coordinates(property);
		}
	},

	on_update: function(property) {
		if (property.name == "GEOGRAPHIC_COORDINATES") {
			this.process_coordinates(property);
		}
	},

	on_enumerate_properties: function(property) {
		if (property.device == "Scripting Agent" && (property.name == null || property.name == "MY_COORDS")) {
			indigo_define_number_property(
				"Scripting Agent", "MY_COORDS", "My GPS", "Coordinates",
				{ LATITUDE: this.latitude, LONGITUDE: this.longitude },
				{ LATITUDE: { label: "Latitude", format: "%m", min: -90, max: 90 },
				  LONGITUDE: { label: "Longitude", format: "%m", min: -180, max: 360 } },
				this.ready ? "Ok" : "Busy", "RO", null
			);
		}
	}
};

indigo_event_handlers.gps_bridge.devices = [ "GPS Simulator" ];
indigo_enumerate_properties(); // announce MY_COORDS right away
```
To stop the bridge and clean up its property (for example from a second, short lived script):
```JS
delete indigo_event_handlers.gps_bridge;
indigo_delete_property("Scripting Agent", "MY_COORDS", null);
```

## Best Practices

- Never assume any particular order of property definitions or updates; always check `property.state` before trusting item values.
- Never use a loop, or any other blocking construct, to wait for a condition - use event handlers and/or timers.
- Give global variables distinctive names (or namespace them, e.g. as fields of a single object) to avoid clashing with other scripts running in the same agent.
- Remove event handlers with `delete` as soon as they are no longer needed, especially any handler created only for temporary monitoring/debugging.
- Prefer custom (shared) properties over `indigo_send_message()` for structured communication between scripts or with clients; messages are for human readable notifications only.
- Follow the same state machine discipline (`Ok -> Busy -> Ok`/`Alert`, etc.) for properties a script defines as is required from any INDIGO driver - see [State Descriptions and State Transitions](DRIVER_DEVELOPMENT_BASICS.md#state-descriptions-and-state-transitions).
- Mark a script to run "on agent load" only after it has been fully tested; a broken script that runs on every server/application startup can make troubleshooting difficult, since the same script runs again on the next restart.

## Further Reading

- [Properties](PROPERTIES.md)
- [Property Manipulation](PROPERTY_MANIPULATION.md) (the *indigo_prop_tool* reference used throughout this guide)
- [Driver Development Basics](DRIVER_DEVELOPMENT_BASICS.md) (property life cycle and state transition rules apply equally to scripting)
- [Client Development Basics](CLIENT_DEVELOPMENT_BASICS.md)
- [Scripting Agent README](../indigo_drivers/agent_scripting/README.md) (low level API listing and additional examples)
- [Script based Sequencer](../indigo_drivers/agent_scripting/library/Sequencer.md) (a real-world library of scripts built on top of this API)
