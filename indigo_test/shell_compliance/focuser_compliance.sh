#!/bin/bash
# INDIGO Focuser Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: focuser_compliance.sh "Device Name" [host:port]

# INDIGO tool executable - can be overridden by environment variable
: ${INDIGO_PROP_TOOL:="indigo_prop_tool"}

# Source the test framework
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/indigo_test_framework.sh"

# Print usage
print_usage() {
	echo "Usage: $0 \"Device Name\" [host:port]"
	echo ""
	echo "Examples:"
	echo "  $0 \"Focuser Simulator\""
	echo "  $0 \"Focuser Simulator\" localhost:7624"
	echo "  $0 \"Focuser Simulator\" 192.168.1.100:7624"
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

print_test_header "INDIGO Focuser Compliance Test" "$DEVICE" "$REMOTE"

# Verify device has focuser interface
echo "--- Verifying Device Interface ---"
if is_focuser "$DEVICE"; then
	print_test_result "Device implements FOCUSER interface" "PASS"
else
	print_test_result "Device implements FOCUSER interface" "FAIL" "This compliance test is only for focuser devices"
	exit 1
fi
echo ""

# Test 0: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory Properties ---"

test_property_exists "Test FOCUSER_STEPS property exists" "$DEVICE.FOCUSER_STEPS"
test_property_exists "Test FOCUSER_DIRECTION property exists" "$DEVICE.FOCUSER_DIRECTION"
test_property_exists "Test FOCUSER_POSITION property exists" "$DEVICE.FOCUSER_POSITION"
test_property_exists "Test FOCUSER_ABORT_MOTION property exists" "$DEVICE.FOCUSER_ABORT_MOTION"
test_property_exists "Test FOCUSER_ON_POSITION_SET property exists" "$DEVICE.FOCUSER_ON_POSITION_SET"

echo ""
echo "--- Getting Focuser Range ---"

# Get position range
POSITION_MIN=$(get_item_min "$DEVICE.FOCUSER_POSITION.POSITION")
POSITION_MAX=$(get_item_max "$DEVICE.FOCUSER_POSITION.POSITION")

if [ -z "$POSITION_MIN" ] || [ -z "$POSITION_MAX" ]; then
	echo "Error: Could not determine focuser position range"
	print_test_summary
	exit 1
fi

echo "Focuser position range: $POSITION_MIN to $POSITION_MAX"

# Get steps range
STEPS_MAX=$(get_item_max "$DEVICE.FOCUSER_STEPS.STEPS")
if [ -z "$STEPS_MAX" ]; then
	echo "Warning: Could not determine max steps, using 1000"
	STEPS_MAX=1000
fi

echo "Max steps: $STEPS_MAX"
echo ""

# Store original values
echo "--- Storing Original Values ---"

ORIGINAL_POSITION=$(get_item_value "$DEVICE.FOCUSER_POSITION.POSITION" "OK")
echo "Original position: $ORIGINAL_POSITION"

# Store original ON_POSITION_SET mode
ORIGINAL_GOTO=$(get_item_value "$DEVICE.FOCUSER_ON_POSITION_SET.GOTO" "OK")
ORIGINAL_SYNC=$(get_item_value "$DEVICE.FOCUSER_ON_POSITION_SET.SYNC" "OK")

# Store optional properties if they exist
if property_exists "$DEVICE.FOCUSER_REVERSE_MOTION"; then
	ORIGINAL_REVERSE_ENABLED=$(get_item_value "$DEVICE.FOCUSER_REVERSE_MOTION.ENABLED" "OK")
	ORIGINAL_REVERSE_DISABLED=$(get_item_value "$DEVICE.FOCUSER_REVERSE_MOTION.DISABLED" "OK")
	echo "Original reverse motion: ENABLED=$ORIGINAL_REVERSE_ENABLED DISABLED=$ORIGINAL_REVERSE_DISABLED"
fi

if property_exists "$DEVICE.FOCUSER_BACKLASH"; then
	ORIGINAL_BACKLASH=$(get_item_value "$DEVICE.FOCUSER_BACKLASH.BACKLASH" "OK")
	echo "Original backlash: $ORIGINAL_BACKLASH"
fi

echo ""
echo "--- Test 1: GOTO Position Test ---"

# Set GOTO mode
test_set_and_verify "Set GOTO mode" \
	"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO=ON" \
	"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO" \
	"ON" 5

# Test moving to position 200 if in range
TARGET_POS=200
if awk -v pos="$TARGET_POS" -v min="$POSITION_MIN" -v max="$POSITION_MAX" 'BEGIN { exit !(pos >= min && pos <= max) }'; then
	echo "Moving to position $TARGET_POS..."
	test_set_transition_smart "Move to position $TARGET_POS" \
		"$DEVICE.FOCUSER_POSITION.POSITION=$TARGET_POS" \
		"$DEVICE.FOCUSER_POSITION.POSITION" \
		"$TARGET_POS" 30 "number"

	test_get_value "Verify at position $TARGET_POS" \
		"$DEVICE.FOCUSER_POSITION.POSITION" \
		"$TARGET_POS" "number"
else
	echo "Position $TARGET_POS out of range, skipping GOTO test"
fi

echo ""
echo "--- Test 2: Relative Move Tests ---"

# Test small moves in and out
MOVE_STEPS=150

# Calculate safe move steps (not to exceed range)
if awk -v steps="$MOVE_STEPS" -v max="$STEPS_MAX" 'BEGIN { exit !(steps <= max) }'; then
	# Move inward
	echo "Moving $MOVE_STEPS steps inward..."
	test_set_and_verify "Set direction to MOVE_INWARD" \
		"$DEVICE.FOCUSER_DIRECTION.MOVE_INWARD=ON" \
		"$DEVICE.FOCUSER_DIRECTION.MOVE_INWARD" \
		"ON" 5

	test_set_transition_smart "Move $MOVE_STEPS steps" \
		"$DEVICE.FOCUSER_STEPS.STEPS=$MOVE_STEPS" \
		"$DEVICE.FOCUSER_STEPS.STEPS" \
		"$MOVE_STEPS" 30 "number"

	POS_AFTER_IN=$(get_item_value "$DEVICE.FOCUSER_POSITION.POSITION" "OK")
	echo "Position after inward move: $POS_AFTER_IN"

	# Move outward
	echo "Moving $MOVE_STEPS steps outward..."
	test_set_and_verify "Set direction to MOVE_OUTWARD" \
		"$DEVICE.FOCUSER_DIRECTION.MOVE_OUTWARD=ON" \
		"$DEVICE.FOCUSER_DIRECTION.MOVE_OUTWARD" \
		"ON" 5

	test_set_transition_smart "Move $MOVE_STEPS steps" \
		"$DEVICE.FOCUSER_STEPS.STEPS=$MOVE_STEPS" \
		"$DEVICE.FOCUSER_STEPS.STEPS" \
		"$MOVE_STEPS" 30 "number"

	POS_AFTER_OUT=$(get_item_value "$DEVICE.FOCUSER_POSITION.POSITION" "OK")
	echo "Position after outward move: $POS_AFTER_OUT"

	# Verify we're back near original position (within tolerance)
	if awk -v p1="$POS_AFTER_OUT" -v p2="$TARGET_POS" 'BEGIN { diff = p1 - p2; if (diff < 0) diff = -diff; exit !(diff <= 10) }'; then
		print_test_result "In/Out moves returned near original" "PASS"
	else
		print_test_result "In/Out moves returned near original" "FAIL" "Position difference too large"
	fi
else
	echo "Move steps $MOVE_STEPS exceeds max, skipping relative move test"
fi

echo ""
echo "--- Test 3: Abort Motion Test ---"

# Calculate halfway position
HALF_MAX=$(awk -v max="$POSITION_MAX" 'BEGIN { printf "%.0f", max / 2 }')

echo "Moving to halfway position ($HALF_MAX) then aborting..."

# Set GOTO mode
test_set_and_verify "Set GOTO mode for abort test" \
	"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO=ON" \
	"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO" \
	"ON" 5

# Start move to max (don't wait for completion)
$INDIGO_PROP_TOOL set -w BUSY $REMOTE_SERVER "$DEVICE.FOCUSER_POSITION.POSITION=$POSITION_MAX" >/dev/null 2>&1

# Wait 2 seconds before aborting
sleep 2

# Abort motion
echo "Sending abort command..."
test_state_transition "Abort motion" \
	"$DEVICE.FOCUSER_ABORT_MOTION.ABORT_MOTION=ON" \
	"BUSY" "OK" 10

# Check that both STEPS and POSITION are not BUSY (OK or ALERT acceptable)
test_property_state "Abort motion - STEPS state not BUSY" \
	"$DEVICE.FOCUSER_STEPS" \
	"OK,ALERT"

test_property_state "Abort motion - POSITION state not BUSY" \
	"$DEVICE.FOCUSER_POSITION" \
	"OK,ALERT"

echo ""
echo "--- Test 4: SYNC Mode Test ---"

# Check if FOCUSER_ON_POSITION_SET exists before running SYNC tests
if property_exists "$DEVICE.FOCUSER_ON_POSITION_SET"; then
	# Store position before sync test
	POS_BEFORE_SYNC=$(get_item_value "$DEVICE.FOCUSER_POSITION.POSITION" "OK")
	echo "Position before sync test: $POS_BEFORE_SYNC"

	# Set SYNC mode
	test_set_and_verify "Set SYNC mode" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.SYNC=ON" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.SYNC" \
		"ON" 5

	# Choose a sync position (midway in range)
	SYNC_POS=$(awk -v min="$POSITION_MIN" -v max="$POSITION_MAX" 'BEGIN { printf "%.0f", (min + max) / 2 }')

	echo "Syncing to position $SYNC_POS..."
	test_set_and_verify "Sync to position $SYNC_POS" \
		"$DEVICE.FOCUSER_POSITION.POSITION=$SYNC_POS" \
		"$DEVICE.FOCUSER_POSITION.POSITION" \
		"$SYNC_POS" 10 "number"

	# Sync back to position before sync test
	echo "Syncing back to position before sync ($POS_BEFORE_SYNC)..."
	test_set_and_verify "Sync back to position before sync" \
		"$DEVICE.FOCUSER_POSITION.POSITION=$POS_BEFORE_SYNC" \
		"$DEVICE.FOCUSER_POSITION.POSITION" \
		"$POS_BEFORE_SYNC" 10 "number"
else
	print_test_result "SYNC Mode Test" "SKIP" "Property FOCUSER_ON_POSITION_SET does not exist"
fi

echo ""
echo "--- Test 5: Optional Properties ---"

# Test FOCUSER_REVERSE_MOTION if exists
if property_exists "$DEVICE.FOCUSER_REVERSE_MOTION"; then
	echo "Testing FOCUSER_REVERSE_MOTION..."

	test_set_and_verify "Set reverse motion ENABLED" \
		"$DEVICE.FOCUSER_REVERSE_MOTION.ENABLED=ON" \
		"$DEVICE.FOCUSER_REVERSE_MOTION.ENABLED" \
		"ON" 5

	test_set_and_verify "Set reverse motion DISABLED" \
		"$DEVICE.FOCUSER_REVERSE_MOTION.DISABLED=ON" \
		"$DEVICE.FOCUSER_REVERSE_MOTION.DISABLED" \
		"ON" 5

	echo ""
fi

# Test FOCUSER_BACKLASH if exists
if property_exists "$DEVICE.FOCUSER_BACKLASH"; then
	echo "Testing FOCUSER_BACKLASH..."

	# Get backlash range
	BACKLASH_MAX=$(get_item_max "$DEVICE.FOCUSER_BACKLASH.BACKLASH")
	TEST_BACKLASH=50

	# Use smaller value if 50 exceeds max
	if [ -n "$BACKLASH_MAX" ]; then
		if awk -v test="$TEST_BACKLASH" -v max="$BACKLASH_MAX" 'BEGIN { exit !(test > max) }'; then
			TEST_BACKLASH=$(awk -v max="$BACKLASH_MAX" 'BEGIN { printf "%.0f", max / 2 }')
		fi
	fi

	test_set_and_verify "Set backlash to $TEST_BACKLASH" \
		"$DEVICE.FOCUSER_BACKLASH.BACKLASH=$TEST_BACKLASH" \
		"$DEVICE.FOCUSER_BACKLASH.BACKLASH" \
		"$TEST_BACKLASH" 5 "number"

	echo ""
fi

# Test FOCUSER_TEMPERATURE if exists
if property_exists "$DEVICE.FOCUSER_TEMPERATURE"; then
	echo "Testing FOCUSER_TEMPERATURE..."

	TEMPERATURE=$(get_item_value "$DEVICE.FOCUSER_TEMPERATURE.TEMPERATURE" "OK")

	if [ -n "$TEMPERATURE" ]; then
		echo "Temperature reading: $TEMPERATURE °C"

		# Check if temperature is reasonable (> -100 means sensor present)
		if awk -v temp="$TEMPERATURE" 'BEGIN { exit !(temp > -100) }'; then
			print_test_result "Temperature sensor present (>-100°C)" "PASS"
		else
			print_test_result "Temperature sensor present (>-100°C)" "FAIL" "Reading $TEMPERATURE suggests no sensor"
		fi
	fi

	echo ""
fi

echo ""
echo "--- Restore Initial State ---"

# Restore GOTO mode if it was originally on
if [ "$ORIGINAL_GOTO" = "ON" ]; then
	echo "Restoring GOTO mode..."
	test_set_and_verify "Restore GOTO mode" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO" \
		"ON" 5
elif [ "$ORIGINAL_SYNC" = "ON" ]; then
	echo "Restoring SYNC mode..."
	test_set_and_verify "Restore SYNC mode" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.SYNC=ON" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.SYNC" \
		"ON" 5
fi

# Restore reverse motion if it was tested
if [ -n "$ORIGINAL_REVERSE_ENABLED" ] && [ -n "$ORIGINAL_REVERSE_DISABLED" ]; then
	echo "Restoring reverse motion setting..."
	if [ "$ORIGINAL_REVERSE_ENABLED" = "ON" ]; then
		test_set_and_verify "Restore reverse ENABLED" \
			"$DEVICE.FOCUSER_REVERSE_MOTION.ENABLED=ON" \
			"$DEVICE.FOCUSER_REVERSE_MOTION.ENABLED" \
			"ON" 5
	else
		test_set_and_verify "Restore reverse DISABLED" \
			"$DEVICE.FOCUSER_REVERSE_MOTION.DISABLED=ON" \
			"$DEVICE.FOCUSER_REVERSE_MOTION.DISABLED" \
			"ON" 5
	fi
fi

# Restore backlash if it was tested
if [ -n "$ORIGINAL_BACKLASH" ]; then
	echo "Restoring backlash to $ORIGINAL_BACKLASH..."
	test_set_and_verify "Restore backlash" \
		"$DEVICE.FOCUSER_BACKLASH.BACKLASH=$ORIGINAL_BACKLASH" \
		"$DEVICE.FOCUSER_BACKLASH.BACKLASH" \
		"$ORIGINAL_BACKLASH" 5 "number"
fi

# Restore original position
echo "Restoring original position $ORIGINAL_POSITION..."
if [ "$ORIGINAL_GOTO" = "ON" ] || [ "$ORIGINAL_SYNC" != "ON" ]; then
	# Use GOTO mode to restore
	test_set_and_verify "Set GOTO mode for restore" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.FOCUSER_ON_POSITION_SET.GOTO" \
		"ON" 5

	test_set_transition_smart "Restore position $ORIGINAL_POSITION" \
		"$DEVICE.FOCUSER_POSITION.POSITION=$ORIGINAL_POSITION" \
		"$DEVICE.FOCUSER_POSITION.POSITION" \
		"$ORIGINAL_POSITION" 30 "number"
fi

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
