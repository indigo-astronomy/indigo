#!/bin/bash
# INDIGO Rotator Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: rotator_compliance.sh "Device Name" [host:port]

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
	echo "  $0 \"Rotator Simulator\""
	echo "  $0 \"Rotator Simulator\" localhost:7624"
	echo "  $0 \"Rotator Simulator\" 192.168.1.100:7624"
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

print_test_header "INDIGO Rotator Compliance Test" "$DEVICE" "$REMOTE"

# Verify device has rotator interface
echo "--- Verifying Device Interface ---"
if is_rotator "$DEVICE"; then
	print_test_result "Device implements ROTATOR interface" "PASS"
else
	print_test_result "Device implements ROTATOR interface" "FAIL" "This compliance test is only for rotator devices"
	exit 1
fi
echo ""

# Test 0: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory Properties ---"

test_property_exists "Test ROTATOR_POSITION property exists" "$DEVICE.ROTATOR_POSITION"
test_property_exists "Test ROTATOR_ABORT_MOTION property exists" "$DEVICE.ROTATOR_ABORT_MOTION"

echo ""
echo "--- Getting Rotator Range ---"

# Get position range
POSITION_MIN=$(get_item_min "$DEVICE.ROTATOR_POSITION.POSITION")
POSITION_MAX=$(get_item_max "$DEVICE.ROTATOR_POSITION.POSITION")

if [ -z "$POSITION_MIN" ] || [ -z "$POSITION_MAX" ]; then
	echo "Error: Could not determine rotator position range"
	print_test_summary
	exit 1
fi

echo "Rotator position range: $POSITION_MIN to $POSITION_MAX degrees"
echo ""

# Store original values
echo "--- Storing Original Values ---"

ORIGINAL_POSITION=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
echo "Original position: $ORIGINAL_POSITION degrees"

# Store optional properties if they exist
if property_exists "$DEVICE.ROTATOR_ON_POSITION_SET"; then
	ORIGINAL_GOTO=$(get_item_value "$DEVICE.ROTATOR_ON_POSITION_SET.GOTO" "OK")
	ORIGINAL_SYNC=$(get_item_value "$DEVICE.ROTATOR_ON_POSITION_SET.SYNC" "OK")
	echo "Original ON_POSITION_SET: GOTO=$ORIGINAL_GOTO SYNC=$ORIGINAL_SYNC"
fi

if property_exists "$DEVICE.ROTATOR_DIRECTION"; then
	ORIGINAL_NORMAL=$(get_item_value "$DEVICE.ROTATOR_DIRECTION.NORMAL" "OK")
	ORIGINAL_REVERSED=$(get_item_value "$DEVICE.ROTATOR_DIRECTION.REVERSED" "OK")
	echo "Original direction: NORMAL=$ORIGINAL_NORMAL REVERSED=$ORIGINAL_REVERSED"
fi

if property_exists "$DEVICE.ROTATOR_BACKLASH"; then
	ORIGINAL_BACKLASH=$(get_item_value "$DEVICE.ROTATOR_BACKLASH.BACKLASH" "OK")
	echo "Original backlash: $ORIGINAL_BACKLASH"
fi

if property_exists "$DEVICE.ROTATOR_LIMITS"; then
	ORIGINAL_MIN_LIMIT=$(get_item_value "$DEVICE.ROTATOR_LIMITS.MIN_POSITION" "OK")
	ORIGINAL_MAX_LIMIT=$(get_item_value "$DEVICE.ROTATOR_LIMITS.MAX_POSITION" "OK")
	echo "Original limits: MIN=$ORIGINAL_MIN_LIMIT MAX=$ORIGINAL_MAX_LIMIT"
fi

echo ""
echo "--- Test 1: GOTO Position Test ---"

# Check if ROTATOR_ON_POSITION_SET exists before running GOTO tests
if property_exists "$DEVICE.ROTATOR_ON_POSITION_SET"; then
	# Set GOTO mode
	test_set_and_verify "Set GOTO mode" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO" \
		"ON" 5

	# Test moving to position 90 degrees if in range
	TARGET_POS=90
	if awk -v pos="$TARGET_POS" -v min="$POSITION_MIN" -v max="$POSITION_MAX" 'BEGIN { exit !(pos >= min && pos <= max) }'; then
		echo "Moving to position $TARGET_POS degrees..."
		test_set_transition_smart "Move to position $TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION=$TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION" \
			"$TARGET_POS" 120 "number"

		test_get_value "Verify at position $TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION" \
			"$TARGET_POS" "number"
	else
		echo "Position $TARGET_POS out of range, skipping GOTO test"
	fi

	# Test moving to position 180 degrees if in range
	TARGET_POS=180
	if awk -v pos="$TARGET_POS" -v min="$POSITION_MIN" -v max="$POSITION_MAX" 'BEGIN { exit !(pos >= min && pos <= max) }'; then
		echo "Moving to position $TARGET_POS degrees..."
		test_set_transition_smart "Move to position $TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION=$TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION" \
			"$TARGET_POS" 120 "number"

		test_get_value "Verify at position $TARGET_POS" \
			"$DEVICE.ROTATOR_POSITION.POSITION" \
			"$TARGET_POS" "number"
	else
		echo "Position $TARGET_POS out of range, skipping test"
	fi
else
	print_test_result "GOTO Position Test" "SKIP" "Property ROTATOR_ON_POSITION_SET does not exist"
fi

echo ""
echo "--- Test 2: Relative Move Test ---"

# Check if ROTATOR_RELATIVE_MOVE exists
if property_exists "$DEVICE.ROTATOR_RELATIVE_MOVE"; then
	echo "Testing ROTATOR_RELATIVE_MOVE property..."

	# Get current position
	CURRENT_POS=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
	echo "Current position: $CURRENT_POS degrees"

	# Test relative move of 30 degrees
	RELATIVE_MOVE=30
	EXPECTED_POS=$(awk -v curr="$CURRENT_POS" -v move="$RELATIVE_MOVE" -v min="$POSITION_MIN" -v max="$POSITION_MAX" '
		BEGIN {
			result = curr + move
			# Handle wrapping if needed (0-360 range)
			if (max - min > 359) {
				while (result > max) result -= 360
				while (result < min) result += 360
			}
			printf "%.2f", result
		}')

	echo "Moving $RELATIVE_MOVE degrees relative (expecting position ~$EXPECTED_POS)..."
	test_set_transition_smart "Relative move $RELATIVE_MOVE degrees" \
		"$DEVICE.ROTATOR_RELATIVE_MOVE.RELATIVE_MOVE=$RELATIVE_MOVE" \
		"$DEVICE.ROTATOR_POSITION.POSITION" \
		"$EXPECTED_POS" 30 "number"

	# Verify position changed (allow tolerance)
	NEW_POS=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
	echo "Position after relative move: $NEW_POS degrees"

	# Test relative move back
	RELATIVE_MOVE=-30
	echo "Moving $RELATIVE_MOVE degrees relative..."
	test_set_transition_smart "Relative move $RELATIVE_MOVE degrees" \
		"$DEVICE.ROTATOR_RELATIVE_MOVE.RELATIVE_MOVE=$RELATIVE_MOVE" \
		"$DEVICE.ROTATOR_POSITION.POSITION" \
		"$CURRENT_POS" 30 "number"

	RESTORED_POS=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
	echo "Position after return move: $RESTORED_POS degrees"

	# Verify we're back near original position (within 2 degree tolerance)
	if awk -v p1="$RESTORED_POS" -v p2="$CURRENT_POS" 'BEGIN { diff = p1 - p2; if (diff < 0) diff = -diff; exit !(diff <= 2) }'; then
		print_test_result "Relative moves returned near original" "PASS"
	else
		print_test_result "Relative moves returned near original" "FAIL" "Position difference too large"
	fi
else
	print_test_result "Relative Move Test" "SKIP" "Property ROTATOR_RELATIVE_MOVE does not exist"
fi

echo ""
echo "--- Test 3: Abort Motion Test ---"

# Only test abort if GOTO is supported
if property_exists "$DEVICE.ROTATOR_ON_POSITION_SET"; then
	# Set GOTO mode
	test_set_and_verify "Set GOTO mode for abort test" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO" \
		"ON" 5

	# Calculate a far position to move to (for testing abort)
	CURRENT_POS=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
	FAR_POS=$(awk -v curr="$CURRENT_POS" -v max="$POSITION_MAX" 'BEGIN { 
		# Move to opposite side
		target = curr + 180
		if (target > max) target -= 360
		printf "%.2f", target
	}')

	echo "Moving to far position ($FAR_POS degrees) then aborting..."

	# Start move (don't wait for completion)
	$INDIGO_PROP_TOOL set -w BUSY $REMOTE_SERVER "$DEVICE.ROTATOR_POSITION.POSITION=$FAR_POS" >/dev/null 2>&1 &

	# Wait 2 seconds before aborting
	sleep 2

	# Abort motion
	echo "Sending abort command..."
	test_state_transition "Abort motion" \
		"$DEVICE.ROTATOR_ABORT_MOTION.ABORT_MOTION=ON" \
		"BUSY" "OK" 10

	# Check that POSITION is not BUSY (OK or ALERT acceptable)
	test_property_state "Abort motion - POSITION state not BUSY" \
		"$DEVICE.ROTATOR_POSITION" \
		"OK,ALERT"
else
	print_test_result "Abort Motion Test" "SKIP" "Property ROTATOR_ON_POSITION_SET does not exist"
fi

echo ""
echo "--- Test 4: SYNC Mode Test ---"

# Check if ROTATOR_ON_POSITION_SET exists before running SYNC tests
if property_exists "$DEVICE.ROTATOR_ON_POSITION_SET"; then
	# Read current position before SYNC test
	POSITION_BEFORE_SYNC=$(get_item_value "$DEVICE.ROTATOR_POSITION.POSITION" "OK")
	echo "Current position before SYNC test: $POSITION_BEFORE_SYNC degrees"

	# Set SYNC mode
	test_set_and_verify "Set SYNC mode" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.SYNC=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.SYNC" \
		"ON" 5

	# Choose a sync position
	SYNC_POS=0

	echo "Syncing to position $SYNC_POS..."
	test_set_and_verify "Sync to position $SYNC_POS" \
		"$DEVICE.ROTATOR_POSITION.POSITION=$SYNC_POS" \
		"$DEVICE.ROTATOR_POSITION.POSITION" \
		"$SYNC_POS" 10 "number"

	# Sync back to the position we read before the test
	echo "Syncing back to position $POSITION_BEFORE_SYNC..."
	test_set_and_verify "Sync back to position before test" \
		"$DEVICE.ROTATOR_POSITION.POSITION=$POSITION_BEFORE_SYNC" \
		"$DEVICE.ROTATOR_POSITION.POSITION" \
		"$POSITION_BEFORE_SYNC" 120 "number"
else
	print_test_result "SYNC Mode Test" "SKIP" "Property ROTATOR_ON_POSITION_SET does not exist"
fi

echo ""
echo "--- Test 5: Optional Properties ---"

# Test ROTATOR_DIRECTION if exists
if property_exists "$DEVICE.ROTATOR_DIRECTION"; then
	echo "Testing ROTATOR_DIRECTION..."

	test_set_and_verify "Set direction to REVERSED" \
		"$DEVICE.ROTATOR_DIRECTION.REVERSED=ON" \
		"$DEVICE.ROTATOR_DIRECTION.REVERSED" \
		"ON" 5

	test_set_and_verify "Set direction to NORMAL" \
		"$DEVICE.ROTATOR_DIRECTION.NORMAL=ON" \
		"$DEVICE.ROTATOR_DIRECTION.NORMAL" \
		"ON" 5

	echo ""
fi

# Test ROTATOR_BACKLASH if exists
if property_exists "$DEVICE.ROTATOR_BACKLASH"; then
	echo "Testing ROTATOR_BACKLASH..."

	# Store current backlash value
	CURRENT_BACKLASH=$(get_item_value "$DEVICE.ROTATOR_BACKLASH.BACKLASH" "OK")
	echo "Current backlash: $CURRENT_BACKLASH"

	# Get backlash range
	BACKLASH_MAX=$(get_item_max "$DEVICE.ROTATOR_BACKLASH.BACKLASH")
	TEST_BACKLASH=50

	# Use smaller value if 50 exceeds max
	if [ -n "$BACKLASH_MAX" ]; then
		if awk -v test="$TEST_BACKLASH" -v max="$BACKLASH_MAX" 'BEGIN { exit !(test > max) }'; then
			TEST_BACKLASH=$(awk -v max="$BACKLASH_MAX" 'BEGIN { printf "%.0f", max / 2 }')
		fi
	fi

	test_set_and_verify "Set backlash to $TEST_BACKLASH" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH=$TEST_BACKLASH" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH" \
		"$TEST_BACKLASH" 5 "number"

	# Restore current backlash value
	echo "Restoring backlash to $CURRENT_BACKLASH..."
	test_set_and_verify "Restore backlash to original value" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH=$CURRENT_BACKLASH" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH" \
		"$CURRENT_BACKLASH" 5 "number"

	echo ""
fi

# Test ROTATOR_LIMITS if exists
if property_exists "$DEVICE.ROTATOR_LIMITS"; then
	echo "Testing ROTATOR_LIMITS..."

	MIN_LIMIT=$(get_item_value "$DEVICE.ROTATOR_LIMITS.MIN_POSITION" "OK")
	MAX_LIMIT=$(get_item_value "$DEVICE.ROTATOR_LIMITS.MAX_POSITION" "OK")

	echo "Rotator limits: MIN=$MIN_LIMIT MAX=$MAX_LIMIT degrees"

	# Verify limits are reasonable
	if awk -v min="$MIN_LIMIT" -v max="$MAX_LIMIT" 'BEGIN { exit !(max > min) }'; then
		print_test_result "Rotator limits are valid (MAX > MIN)" "PASS"
	else
		print_test_result "Rotator limits are valid (MAX > MIN)" "FAIL" "MAX ($MAX_LIMIT) not greater than MIN ($MIN_LIMIT)"
	fi

	echo ""
fi

# Test ROTATOR_STEPS_PER_REVOLUTION if exists
if property_exists "$DEVICE.ROTATOR_STEPS_PER_REVOLUTION"; then
	echo "Testing ROTATOR_STEPS_PER_REVOLUTION..."

	STEPS=$(get_item_value "$DEVICE.ROTATOR_STEPS_PER_REVOLUTION.STEPS_PER_REVOLUTION" "OK")

	if [ -n "$STEPS" ]; then
		echo "Steps per revolution: $STEPS"

		# Verify steps is reasonable (> 0)
		if awk -v steps="$STEPS" 'BEGIN { exit !(steps > 0) }'; then
			print_test_result "Steps per revolution is positive" "PASS"
		else
			print_test_result "Steps per revolution is positive" "FAIL" "Value is $STEPS"
		fi
	fi

	echo ""
fi

echo ""
echo "--- Restore Initial State ---"

# Restore direction if it was tested
if [ -n "$ORIGINAL_NORMAL" ] && [ -n "$ORIGINAL_REVERSED" ]; then
	echo "Restoring direction setting..."
	if [ "$ORIGINAL_NORMAL" = "ON" ]; then
		test_set_and_verify "Restore direction NORMAL" \
			"$DEVICE.ROTATOR_DIRECTION.NORMAL=ON" \
			"$DEVICE.ROTATOR_DIRECTION.NORMAL" \
			"ON" 5
	else
		test_set_and_verify "Restore direction REVERSED" \
			"$DEVICE.ROTATOR_DIRECTION.REVERSED=ON" \
			"$DEVICE.ROTATOR_DIRECTION.REVERSED" \
			"ON" 5
	fi
fi

# Restore backlash if it was tested
if [ -n "$ORIGINAL_BACKLASH" ]; then
	echo "Restoring backlash to $ORIGINAL_BACKLASH..."
	test_set_and_verify "Restore backlash" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH=$ORIGINAL_BACKLASH" \
		"$DEVICE.ROTATOR_BACKLASH.BACKLASH" \
		"$ORIGINAL_BACKLASH" 5 "number"
fi

# Restore original position (must do this before restoring mode)
if property_exists "$DEVICE.ROTATOR_ON_POSITION_SET"; then
	echo "Restoring original position $ORIGINAL_POSITION..."
	# Switch to GOTO mode to actually move (not sync) to original position
	test_set_and_verify "Set GOTO mode for restore" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO" \
		"ON" 5

	test_set_transition_smart "Restore position $ORIGINAL_POSITION" \
		"$DEVICE.ROTATOR_POSITION.POSITION=$ORIGINAL_POSITION" \
		"$DEVICE.ROTATOR_POSITION.POSITION" \
		"$ORIGINAL_POSITION" 120 "number"
fi

# Restore original ON_POSITION_SET mode (after position is restored)
if [ "$ORIGINAL_GOTO" = "ON" ]; then
	echo "Restoring GOTO mode..."
	test_set_and_verify "Restore GOTO mode" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.GOTO" \
		"ON" 5
elif [ "$ORIGINAL_SYNC" = "ON" ]; then
	echo "Restoring SYNC mode..."
	test_set_and_verify "Restore SYNC mode" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.SYNC=ON" \
		"$DEVICE.ROTATOR_ON_POSITION_SET.SYNC" \
		"ON" 5
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
