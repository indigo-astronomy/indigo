#!/bin/bash
# INDIGO Filter Wheel Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: filter_wheel_compliance.sh "Device Name" [host:port]

# INDIGO tool executable - can be overridden by environment variable
: ${INDIGO_PROP_TOOL:="indigo_prop_tool"}

# Source the test framework
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/indigo_test_framework.sh"

# Helper: Get number of filter slots (wheel-specific)
get_wheel_slot_count() {
    local device="$1"
    local max_slot
    # List all SLOT_NAME items and find the highest slot number
    max_slot=$($INDIGO_PROP_TOOL list $REMOTE_SERVER "$device.WHEEL_SLOT_NAME" 2>&1 | \
               grep -oE 'SLOT_NAME_[0-9]+' | \
               sed 's/SLOT_NAME_//' | \
               sort -n | \
               tail -1)
    echo "$max_slot"
}

# Print usage
print_usage() {
    echo "Usage: $0 \"Device Name\" [host:port]"
    echo ""
    echo "Examples:"
    echo "  $0 \"CCD Imager Simulator (wheel)\""
    echo "  $0 \"CCD Imager Simulator (wheel)\" localhost:7624"
    echo "  $0 \"CCD Imager Simulator (wheel)\" 192.168.1.100:7624"
    echo ""
    exit 1
}

# Check parameters
if [ -z "$1" ]; then
    echo "Error: Device name is required"
    echo ""
    print_usage
fi

DEVICE="$1"
REMOTE="${2:-localhost:7624}"

# Set remote server
set_remote_server "$REMOTE"

print_test_header "INDIGO Filter Wheel Compliance Test" "$DEVICE" "$REMOTE"

# Test 1: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory properties ---"

test_property_exists "Test WHEEL_SLOT property exists" "$DEVICE.WHEEL_SLOT"
test_property_exists "Test WHEEL_SLOT_NAME property exists" "$DEVICE.WHEEL_SLOT_NAME"
test_property_exists "Test WHEEL_SLOT_OFFSET property exists" "$DEVICE.WHEEL_SLOT_OFFSET"

echo ""
echo "--- Filter Wheel Properties Tests ---"

# Get number of filter slots
SLOT_COUNT=$(get_wheel_slot_count "$DEVICE")
if [ -z "$SLOT_COUNT" ] || [ "$SLOT_COUNT" -eq 0 ]; then
    echo "Error: Could not determine number of filter slots"
    print_test_summary
    exit 1
fi

echo "Detected $SLOT_COUNT filter slots"
echo ""

# Store original slot names for restoration
echo "--- Storing Original Values ---"
declare -A ORIGINAL_SLOT_NAMES
declare -A ORIGINAL_SLOT_OFFSETS
for slot in $(seq 1 $SLOT_COUNT); do
    ORIGINAL_SLOT_NAMES[$slot]=$($INDIGO_PROP_TOOL get $REMOTE_SERVER "$DEVICE.WHEEL_SLOT_NAME.SLOT_NAME_$slot" 2>&1)
    ORIGINAL_SLOT_OFFSETS[$slot]=$($INDIGO_PROP_TOOL get $REMOTE_SERVER "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot" 2>&1)
done

echo ""
echo "--- Filter Slot Name Tests ---"
# Test setting filter slot names
for slot in $(seq 1 $SLOT_COUNT); do
    test_set_and_verify "Set slot $slot name to 'Filter_$slot'" \
        "$DEVICE.WHEEL_SLOT_NAME.SLOT_NAME_$slot=\"Filter_$slot\"" \
        "$DEVICE.WHEEL_SLOT_NAME.SLOT_NAME_$slot" \
        "Filter_$slot" 5
done

echo ""
echo "--- Filter Slot Offset Tests ---"
# Test SLOT_OFFSET for each slot (numeric property)
for slot in $(seq 1 $SLOT_COUNT); do
    test_set_and_verify "Set slot $slot offset to 1" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot=1" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot" \
        "1" 5 "number"
    
    test_set_and_verify "Set slot $slot offset to -1" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot=-1" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot" \
        "-1" 5 "number"
    
    test_set_and_verify "Set slot $slot offset to 0" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot=0" \
        "$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot" \
        "0" 5 "number"
done

echo ""
echo "--- Filter Movement Tests ---"

# Test 4: Move to each filter slot with smart state transition
# BUSY is mandatory when moving to different slot, optional when already there
for slot in $(seq 1 $SLOT_COUNT); do
    test_set_transition_smart "Move to slot $slot" \
        "$DEVICE.WHEEL_SLOT.SLOT=$slot" \
        "$DEVICE.WHEEL_SLOT.SLOT" \
        "$slot" 30 "number"
    
    # Verify we're at the correct slot (numeric comparison)
    test_get_value "Verify at slot $slot" \
        "$DEVICE.WHEEL_SLOT.SLOT" \
        "$slot" "number"
done

echo ""
echo "--- Boundary Tests ---"

# Test 5: Try to move beyond last slot (should wrap to last slot)
BEYOND_LAST=$((SLOT_COUNT + 1))
echo "Testing move beyond last slot ($BEYOND_LAST)..."
$INDIGO_PROP_TOOL set -w OK -t 10 $REMOTE_SERVER "$DEVICE.WHEEL_SLOT.SLOT=$BEYOND_LAST" >/dev/null 2>&1

# Should be at last slot (numeric comparison)
test_get_value "Beyond last slot -> should be at slot $SLOT_COUNT" \
    "$DEVICE.WHEEL_SLOT.SLOT" \
    "$SLOT_COUNT" "number"

# Test 6: Try to move below first slot (should wrap to first slot)
echo "Testing move below first slot (0)..."
$INDIGO_PROP_TOOL set -w OK -t 10 $REMOTE_SERVER "$DEVICE.WHEEL_SLOT.SLOT=0" >/dev/null 2>&1

# Should be at first slot (numeric comparison)
test_get_value "Below first slot -> should be at slot 1" \
    "$DEVICE.WHEEL_SLOT.SLOT" \
    "1" "number"

echo ""
echo "--- Restore Initial State ---"

# Restore original slot names
echo "Restoring original slot names..."
for slot in $(seq 1 $SLOT_COUNT); do
	if [ -n "${ORIGINAL_SLOT_NAMES[$slot]}" ]; then
		$INDIGO_PROP_TOOL set -w OK -t 5 $REMOTE_SERVER \
			"$DEVICE.WHEEL_SLOT_NAME.SLOT_NAME_$slot=\"${ORIGINAL_SLOT_NAMES[$slot]}\"" >/dev/null 2>&1
	fi
done

# Restore original slot offsets
echo "Restoring original slot offsets..."
for slot in $(seq 1 $SLOT_COUNT); do
	if [ -n "${ORIGINAL_SLOT_OFFSETS[$slot]}" ]; then
		$INDIGO_PROP_TOOL set -w OK -t 5 $REMOTE_SERVER \
			"$DEVICE.WHEEL_SLOT_OFFSET.SLOT_OFFSET_$slot=${ORIGINAL_SLOT_OFFSETS[$slot]}" >/dev/null 2>&1
	fi
done

echo ""
echo "--- Final Disconnect Tests ---"

# Disconnect
test_disconnect "$DEVICE" 10

# Test disconnect when already disconnected
test_disconnect_when_disconnected "$DEVICE" 10

echo ""
# Print summary
print_test_summary

# Exit with appropriate code
if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
