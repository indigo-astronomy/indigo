# INDIGO Test Framework Documentation

## Overview

The INDIGO Test Framework is a bash-based testing library for INDIGO device drivers. It provides reusable test functions for common operations like connection/disconnection, property verification, and state transitions.

## Configuration

### Environment Variables

- **`INDIGO_PROP_TOOL`**: Path to the `indigo_prop_tool` executable (default: `"indigo_prop_tool"`)
  ```bash
  export INDIGO_PROP_TOOL="/path/to/indigo_prop_tool"
  ```

### Setup Functions

#### `set_remote_server(host:port)`
Configure the remote INDIGO server to connect to.

**Parameters:**
- `host:port` - Remote server address (e.g., `"localhost:7624"` or `"192.168.1.100:7624"`)

**Example:**
```bash
set_remote_server "192.168.1.100:7624"
```

---

## Test Result Functions

#### `print_test_result(test_name, result, description)`
Print a test result with color-coded output and update test counters.

**Parameters:**
- `test_name` - Name/description of the test
- `result` - Test result: `"PASS"` or `"FAIL"`
- `description` - (Optional) Reason for failure

**Example:**
```bash
print_test_result "Connect device" "PASS"
print_test_result "Set value" "FAIL" "Timeout exceeded"
```

#### `print_test_summary()`
Print final test statistics (passed/failed counts).

**Example:**
```bash
print_test_summary
# Output:
# ========================================
# Test Summary: 15 passed, 0 failed out of 15 tests
# All tests passed!
# ========================================
```

---

## Property Test Functions

### Basic Property Tests

#### `test_set_wait_state(test_name, property_set, expected_state, timeout)`
Set a property and wait for it to reach a specific state.

**Parameters:**
- `test_name` - Name of the test
- `property_set` - Property to set (format: `"device.property.item=value"`)
- `expected_state` - Expected final state: `"OK"`, `"BUSY"`, `"ALERT"`, or `"IDLE"`
- `timeout` - Timeout in seconds (default: `10`)

**Example:**
```bash
test_set_wait_state "Set exposure time" \
    "CCD Imager.CCD_EXPOSURE.EXPOSURE=1.0" \
    "OK" 5
```

#### `test_get_value(test_name, property_item, expected_value, value_type)`
Get a property item value and compare it to expected value.

**Parameters:**
- `test_name` - Name of the test
- `property_item` - Property item to get (format: `"device.property.item"`)
- `expected_value` - Expected value
- `value_type` - Comparison type: `"string"` (default) or `"number"`

**Example:**
```bash
# String comparison
test_get_value "Check connection" \
    "CCD Imager.CONNECTION.CONNECTED" \
    "ON"

# Numeric comparison (1, 1.0, 1.000000 are all equal)
test_get_value "Check slot" \
    "Filter Wheel.WHEEL_SLOT.SLOT" \
    "3" "number"
```

#### `test_set_and_verify(test_name, property_set, property_get, expected_value, timeout, value_type)`
Set a property and verify it was set correctly (for instant operations).

**Parameters:**
- `test_name` - Name of the test
- `property_set` - Property to set (format: `"device.property.item=value"`)
- `property_get` - Property to verify (format: `"device.property.item"`)
- `expected_value` - Expected value after setting
- `timeout` - Timeout in seconds (default: `10`)
- `value_type` - Comparison type: `"string"` (default) or `"number"`

**Example:**
```bash
test_set_and_verify "Set filter name" \
    "Filter Wheel.WHEEL_SLOT_NAME.SLOT_NAME_1=\"Red\"" \
    "Filter Wheel.WHEEL_SLOT_NAME.SLOT_NAME_1" \
    "Red"
```

---

### State Transition Tests

#### `test_state_transition(test_name, property_set, initial_state, final_state, timeout, require_initial)`
Test state transition (e.g., BUSY → OK).

**Parameters:**
- `test_name` - Name of the test
- `property_set` - Property to set (format: `"device.property.item=value"`)
- `initial_state` - Expected initial state: `"BUSY"`, `"OK"`, `"ALERT"`, or `"IDLE"`
- `final_state` - Expected final state: `"BUSY"`, `"OK"`, `"ALERT"`, or `"IDLE"`
- `timeout` - Timeout in seconds (default: `10`)
- `require_initial` - Whether initial state is mandatory: `"required"` (default) or `"optional"`

**Example:**
```bash
# Require both BUSY and OK states
test_state_transition "Connect device" \
    "CCD Imager.CONNECTION.CONNECTED=ON" \
    "BUSY" "OK" 10

# Initial state is optional
test_state_transition "Set value" \
    "Device.PROPERTY.ITEM=value" \
    "BUSY" "OK" 10 "optional"
```

#### `test_set_transition_smart(test_name, property_set, property_get, target_value, timeout, value_type)`
Smart state transition test that automatically determines if BUSY state is required.
- BUSY is **mandatory** when target differs from current value
- BUSY is **optional** when target equals current value

**Parameters:**
- `test_name` - Name of the test
- `property_set` - Property to set (format: `"device.property.item=value"`)
- `property_get` - Property to get current value (format: `"device.property.item"`)
- `target_value` - Target value to set
- `timeout` - Timeout in seconds (default: `10`)
- `value_type` - Comparison type: `"number"` (default) or `"string"`

**Example:**
```bash
# Moving filter wheel to slot 3
# - If currently at slot 1: BUSY state is MANDATORY
# - If currently at slot 3: BUSY state is OPTIONAL
test_set_transition_smart "Move to slot 3" \
    "Filter Wheel.WHEEL_SLOT.SLOT=3" \
    "Filter Wheel.WHEEL_SLOT.SLOT" \
    "3" 30 "number"
```

---

## Property State Tests

#### `test_property_state(test_name, property, expected_states)`
Test if a property's current state matches one of the expected states.

**Parameters:**
- `test_name` - Name of the test
- `property` - Full property path (format: `"device.property"`)
- `expected_states` - Comma-separated list of acceptable states (e.g., `"OK,ALERT"`)

**Valid states:** `OK`, `BUSY`, `ALERT`, `IDLE`

**Example:**
```bash
# Check that focuser is not busy (OK, ALERT, or IDLE acceptable)
test_property_state "Focuser not busy" \
    "Focuser.FOCUSER_POSITION" \
    "OK,ALERT,IDLE"

# Check that property is in OK state only
test_property_state "Camera ready" \
    "Camera.CCD_EXPOSURE" \
    "OK"

# Check for BUSY or OK (operation in progress or completed)
test_property_state "Mount moving or stopped" \
    "Mount.EQUATORIAL_COORDINATES" \
    "BUSY,OK"
```

**Behavior:**
- Returns PASS if current state matches any state in the list
- Returns FAIL if current state doesn't match any expected state
- Returns SKIP if property doesn't exist

---

## Connection Test Functions

#### `test_connect(device_name, timeout)`
Connect device with BUSY → OK state transition verification.

**Parameters:**
- `device_name` - Name of the device
- `timeout` - Timeout in seconds (default: `10`)

**Example:**
```bash
test_connect "CCD Imager" 10
```

**Tests performed:**
1. Verifies BUSY → OK state transition
2. Verifies `CONNECTED=ON`

#### `test_disconnect(device_name, timeout)`
Disconnect device with BUSY → OK state transition verification.

**Parameters:**
- `device_name` - Name of the device
- `timeout` - Timeout in seconds (default: `10`)

**Example:**
```bash
test_disconnect "CCD Imager" 10
```

**Tests performed:**
1. Verifies BUSY → OK state transition
2. Verifies `DISCONNECTED=ON`
3. Verifies `CONNECTED=OFF`

#### `test_connect_when_connected(device_name, timeout)`
Test connecting an already connected device (should be ignored).

**Parameters:**
- `device_name` - Name of the device
- `timeout` - Timeout in seconds (default: `10`)

**Example:**
```bash
test_connect_when_connected "CCD Imager" 10
```

**Tests performed:**
1. Verifies no property updates are received
2. Verifies device remains `CONNECTED=ON`

#### `test_disconnect_when_disconnected(device_name, timeout)`
Test disconnecting an already disconnected device (should be ignored).

**Parameters:**
- `device_name` - Name of the device
- `timeout` - Timeout in seconds (default: `10`)

**Example:**
```bash
test_disconnect_when_disconnected "CCD Imager" 10
```

**Tests performed:**
1. Verifies no property updates are received
2. Verifies device remains `DISCONNECTED=ON`

#### `test_connection_battery(device_name, timeout)`
Run comprehensive connection test battery for any device type.

**Parameters:**
- `device_name` - Name of the device
- `timeout` - Timeout in seconds (default: `10`)

**Returns:**
- `0` if device was originally connected
- `1` if device was originally disconnected

**Example:**
```bash
test_connection_battery "CCD Imager" 10
WAS_CONNECTED=$?
```

**Tests performed:**
1. Detects initial connection state
2. Tests redundant connect when already connected (if connected)
3. Disconnects device with full verification
4. Tests redundant disconnect when already disconnected
5. Connects device with full verification
6. Returns original state for restoration logic

**Note:** This is a universal function that should be used by all device-specific compliance scripts to ensure consistent connection testing.

#### `test_property_exists(test_name, property, [items])`
Test if a property exists, optionally checking for specific items within the property.

**Parameters:**
- `test_name` - Name of the test for output
- `property` - Full property path (e.g., `"device.PROPERTY_NAME"`)
- `items` - Optional comma-separated list of item names to verify (e.g., `"ITEM1,ITEM2"`)

**Example:**
```bash
# Test for mandatory property
test_property_exists "Check CONNECTION property" "CCD Imager.CONNECTION"

# Test for property with specific items
test_property_exists "Check WHEEL_SLOT with items" \
    "Filter Wheel.WHEEL_SLOT" \
    "SLOT"

# Test for multiple items
test_property_exists "Check INFO property items" \
    "CCD Imager.INFO" \
    "DEVICE_NAME,DEVICE_VERSION,DEVICE_INTERFACE"
```

**Tests performed:**
1. Verifies property exists
2. If items specified, verifies each item exists in the property

---

## Helper Functions

#### `is_device_connected(device_name)`
Check if device is currently connected.

**Parameters:**
- `device_name` - Name of the device

**Returns:**
- `0` (true) if connected
- `1` (false) if disconnected

**Example:**
```bash
if is_device_connected "CCD Imager"; then
    echo "Device is connected"
fi
```

#### `property_exists(property, [items])`
Check if a property exists, optionally verifying specific items.

**Parameters:**
- `property` - Full property path (e.g., `"device.PROPERTY_NAME"`)
- `items` - Optional comma-separated list of item names to verify

**Returns:**
- `0` if property exists (and all items if specified)
- `1` if property or any item not found

**Example:**
```bash
if property_exists "CCD Imager.CCD_INFO" "WIDTH,HEIGHT"; then
    echo "CCD_INFO property with WIDTH and HEIGHT items exists"
fi
```

#### `get_item_value(property_item, [state])`

Get the value of a property item with optional wait for property state. If timeout occurs it will return the value regradless of the state.

**Parameters:**
- `property_item` - Full property item path (e.g., `"device.PROPERTY.ITEM"`)
- `state` - Optional property state to wait for (default: `"ANY"`)
  - Valid states: `OK`, `BUSY`, `ALERT`, `IDLE`, `ANY`

**Returns:**
- Prints the item value to stdout
- Returns the exit code from `indigo_prop_tool get`

**Example:**
```bash
# Get value with default state (ANY)
width=$(get_item_value "CCD Imager.CCD_INFO.WIDTH")
echo "CCD width: $width pixels"

# Get the value when the property becomes OK or when the timeout occurs.
position=$(get_item_value "Focuser.FOCUSER_POSITION.POSITION" "OK")

# Get connection state
connected=$(get_item_value "CCD Imager.CONNECTION.CONNECTED" "ANY")
if [ "$connected" = "ON" ]; then
    echo "Device is connected"
fi

# Store multiple values
latitude=$(get_item_value "GPS.GEOGRAPHIC_COORDINATES.LATITUDE" "ANY")
longitude=$(get_item_value "GPS.GEOGRAPHIC_COORDINATES.LONGITUDE" "ANY")
echo "Location: $latitude, $longitude"
```

**Note:** This is the preferred way to retrieve property values in tests. It provides a cleaner interface than calling `$INDIGO_PROP_TOOL get` directly and ensures consistent usage across all test scripts.

#### `get_item_min(property_item)`

Get the minimum value from a numeric property item's range.

**Parameters:**

- `property_item` - Full property item path (e.g., `"device.PROPERTY.ITEM"`)

**Returns:**

- Echoes the minimum value for numeric properties with a defined range
- Returns empty string (exit code 1) if property is non-numeric or has no range

**Example:**

```bash
# Get minimum value for a numeric property
min_value=$(get_item_min "Filter Wheel.WHEEL_SLOT.SLOT")
if [ -n "$min_value" ]; then
    echo "Minimum slot number: $min_value"
fi

# Check if property has a min value
if get_item_min "CCD Imager.CCD_EXPOSURE.EXPOSURE" >/dev/null; then
    echo "Property has a defined minimum"
fi
```

**Note:** This function uses the `-e` (extended) flag of `indigo_prop_tool` to retrieve property metadata. It parses output like `32227.000000 [0.000000,65535.000000]` to extract the minimum value.

#### `get_item_max(property_item)`

Get the maximum value from a numeric property item's range.

**Parameters:**

- `property_item` - Full property item path (e.g., `"device.PROPERTY.ITEM"`)

**Returns:**

- Echoes the maximum value for numeric properties with a defined range
- Returns empty string (exit code 1) if property is non-numeric or has no range

**Example:**

```bash
# Get maximum value for a numeric property
max_value=$(get_item_max "Filter Wheel.WHEEL_SLOT.SLOT")
if [ -n "$max_value" ]; then
    echo "Maximum slot number: $max_value"
    # For WHEEL_SLOT, max value indicates number of slots
    slot_count=$(printf "%.0f" "$max_value")
fi

# Use in conditional
if max_exp=$(get_item_max "CCD Imager.CCD_EXPOSURE.EXPOSURE"); then
    echo "Maximum exposure: $max_exp seconds"
fi
```

**Note:** This function uses the `-e` (extended) flag of `indigo_prop_tool` to retrieve property metadata. It parses output like `32227.000000 [0.000000,65535.000000]` to extract the maximum value.

---

## Device Interface Verification Functions

These functions check whether a device implements specific INDIGO interface types by examining the `INFO.DEVICE_INTERFACE` bitmask property. This ensures compliance scripts only test features the device actually supports.

### get_device_interface(device_name)

Retrieves the device interface bitmask value from the `INFO.DEVICE_INTERFACE` property.

**Parameters:**
- `device_name` - Full device name (e.g., "Filter Wheel", "CCD Guider Simulator (guider)")

**Returns:**
- Integer bitmask value representing all interfaces the device implements
- Returns 0 if device not found or property unavailable

**Example:**
```bash
interface=$(get_device_interface "Filter Wheel")
echo "Interface bitmask: $interface"  # e.g., "16" for wheel-only device
```

### has_interface(device_name, bitmask)

Checks if a device implements a specific interface by performing bitwise AND with the required bitmask.

**Parameters:**
- `device_name` - Full device name
- `bitmask` - Interface bitmask value to check (see table below)

**Returns:**
- Exit code 0 (true) if device has the interface
- Exit code 1 (false) if device does not have the interface

**Example:**
```bash
if has_interface "Filter Wheel" 16; then
    echo "Device implements WHEEL interface"
fi
```

### Interface Type Functions

Convenience functions for checking common device interfaces. Each function checks if the device implements the corresponding interface type.

| Function | Bitmask | Enum Value | Description |
|----------|---------|------------|-------------|
| `is_mount(device)` | 1 | `INDIGO_INTERFACE_MOUNT` | Mount controller |
| `is_ccd(device)` | 2 | `INDIGO_INTERFACE_CCD` | CCD or camera |
| `is_guider(device)` | 4 | `INDIGO_INTERFACE_GUIDER` | Guider interface |
| `is_focuser(device)` | 8 | `INDIGO_INTERFACE_FOCUSER` | Focuser controller |
| `is_wheel(device)` | 16 | `INDIGO_INTERFACE_WHEEL` | Filter wheel |
| `is_dome(device)` | 32 | `INDIGO_INTERFACE_DOME` | Dome controller |
| `is_gps(device)` | 64 | `INDIGO_INTERFACE_GPS` | GPS receiver |
| `is_ao(device)` | 256 | `INDIGO_INTERFACE_AO` | Adaptive optics |
| `is_rotator(device)` | 4096 | `INDIGO_INTERFACE_ROTATOR` | Field rotator |
| `is_agent(device)` | 16384 | `INDIGO_INTERFACE_AGENT` | Agent device |
| `is_aux(device)` | 32768 | `INDIGO_INTERFACE_AUX` | Auxiliary device |

**Note:** A device may implement multiple interfaces. For example, a camera with built-in guider port would have both CCD (2) and GUIDER (4) interfaces, resulting in bitmask value 6.

**Examples:**
```bash
# Check single interface
if is_wheel "Filter Wheel"; then
    echo "Device has wheel interface"
fi

# Use in compliance script header
if ! is_focuser "$DEVICE"; then
    echo -e "${RED}ERROR: Device does not implement FOCUSER interface${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Device implements FOCUSER interface${NC}"

# Check for multi-interface device
if is_ccd "CCD Imager" && is_guider "CCD Imager"; then
    echo "Camera has built-in guider port"
fi
```

### Usage in Compliance Scripts

All device-specific compliance scripts should verify the device implements the required interface before running tests:

```bash
#!/bin/bash

source "indigo_test_framework.sh"

DEVICE="$1"
if [ -z "$DEVICE" ]; then
    echo "Usage: $0 <device_name>"
    exit 1
fi

# Verify device implements WHEEL interface
if ! is_wheel "$DEVICE"; then
    echo -e "${RED}ERROR: Device '$DEVICE' does not implement WHEEL interface${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Device '$DEVICE' implements WHEEL interface${NC}"
echo ""

# Continue with wheel-specific tests...
test_connection_battery "$DEVICE" 10
```

This pattern prevents tests from attempting operations on devices that don't support them (e.g., trying to run focuser tests on a filter wheel).

---

## Global Variables

### Test Counters
- `TESTS_PASSED` - Number of tests passed
- `TESTS_FAILED` - Number of tests failed
- `TESTS_SKIPPED` - Number of tests skipped (property/item not present)

### Configuration
- `REMOTE_SERVER` - Remote server argument for indigo_prop_tool (set via `set_remote_server()`)
- `INDIGO_PROP_TOOL` - Path to indigo_prop_tool executable

---

## Complete Example

```bash
#!/bin/bash

# Source the framework
source "indigo_test_framework.sh"

# Configure
set_remote_server "localhost:7624"

# Run tests
test_connect "CCD Imager" 10
test_set_and_verify "Set binning" \
    "CCD Imager.CCD_BINNING.HORIZONTAL=2" \
    "CCD Imager.CCD_BINNING.HORIZONTAL" \
    "2" 5 "number"

test_disconnect "CCD Imager" 10

# Print results
print_test_summary
```

---

## Notes

- All property paths use the format: `device.property.item` or `device.property.item=value`
- Device names with spaces or special characters should be quoted
- Numeric comparisons handle floating-point equality (1 == 1.0 == 1.000000)
- State transitions follow INDIGO specifications:
  - Operations changing values: BUSY → OK
  - Instant operations: OK → OK
  - Failed operations: BUSY → ALERT or OK → ALERT
