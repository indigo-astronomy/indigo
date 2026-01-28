#!/bin/bash
# INDIGO Guider Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: guider_compliance.sh "Device Name" [host:port]

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
	echo "  $0 \"Guider Simulator\""
	echo "  $0 \"Guider Simulator\" localhost:7624"
	echo "  $0 \"Guider Simulator\" 192.168.1.100:7624"
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

print_test_header "INDIGO Guider Compliance Test" "$DEVICE" "$REMOTE"

# Test 0: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory Properties ---"

test_property_exists "Test GUIDER_GUIDE_DEC property exists" "$DEVICE.GUIDER_GUIDE_DEC"
test_property_exists "Test GUIDER_GUIDE_RA property exists" "$DEVICE.GUIDER_GUIDE_RA"

echo ""
echo "--- Test 1: GUIDE RA Tests ---"

# Get RA guide limits (RA is EAST/WEST)
RA_EAST_MAX=$(get_item_max "$DEVICE.GUIDER_GUIDE_RA.EAST")
RA_WEST_MAX=$(get_item_max "$DEVICE.GUIDER_GUIDE_RA.WEST")

if [ -z "$RA_EAST_MAX" ] || [ -z "$RA_WEST_MAX" ]; then
	echo "Error: Could not determine GUIDE RA limits"
else
	echo "GUIDE RA limits: EAST max=$RA_EAST_MAX, WEST max=$RA_WEST_MAX"

	# Use 500ms or max, whichever is smaller
	EAST_PULSE=$(awk -v max="$RA_EAST_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')
	WEST_PULSE=$(awk -v max="$RA_WEST_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')

	echo "Testing EAST pulse: ${EAST_PULSE}ms"
	test_state_transition "Guide EAST ${EAST_PULSE}ms" \
		"$DEVICE.GUIDER_GUIDE_RA.EAST=$EAST_PULSE" \
		"BUSY" "OK" 10

	sleep 1

	echo "Testing WEST pulse: ${WEST_PULSE}ms"
	test_state_transition "Guide WEST ${WEST_PULSE}ms" \
		"$DEVICE.GUIDER_GUIDE_RA.WEST=$WEST_PULSE" \
		"BUSY" "OK" 10
fi

echo ""
echo "--- Test 2: GUIDE DEC Tests ---"

# Get DEC guide limits (DEC is NORTH/SOUTH)
DEC_NORTH_MAX=$(get_item_max "$DEVICE.GUIDER_GUIDE_DEC.NORTH")
DEC_SOUTH_MAX=$(get_item_max "$DEVICE.GUIDER_GUIDE_DEC.SOUTH")

if [ -z "$DEC_NORTH_MAX" ] || [ -z "$DEC_SOUTH_MAX" ]; then
	echo "Error: Could not determine GUIDE DEC limits"
else
	echo "GUIDE DEC limits: NORTH max=$DEC_NORTH_MAX, SOUTH max=$DEC_SOUTH_MAX"

	# Use 500ms or max, whichever is smaller
	NORTH_PULSE=$(awk -v max="$DEC_NORTH_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')
	SOUTH_PULSE=$(awk -v max="$DEC_SOUTH_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')

	echo "Testing NORTH pulse: ${NORTH_PULSE}ms"
	test_state_transition "Guide NORTH ${NORTH_PULSE}ms" \
		"$DEVICE.GUIDER_GUIDE_DEC.NORTH=$NORTH_PULSE" \
		"BUSY" "OK" 10

	sleep 1

	echo "Testing SOUTH pulse: ${SOUTH_PULSE}ms"
	test_state_transition "Guide SOUTH ${SOUTH_PULSE}ms" \
		"$DEVICE.GUIDER_GUIDE_DEC.SOUTH=$SOUTH_PULSE" \
		"BUSY" "OK" 10
fi

echo ""
echo "--- Test 3: Optional GUIDER_RATE Property ---"

if property_exists "$DEVICE.GUIDER_RATE" "RATE"; then
	echo "Testing GUIDER_RATE.RATE..."

	# Get original rate
	ORIGINAL_RATE=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$DEVICE.GUIDER_RATE.RATE" 2>&1)
	echo "Original rate: $ORIGINAL_RATE"

	# Get rate max
	RATE_MAX=$(get_item_max "$DEVICE.GUIDER_RATE.RATE")
	if [ -n "$RATE_MAX" ]; then
		TEST_RATE=$(awk -v max="$RATE_MAX" 'BEGIN { printf "%.2f", max / 2 }')
		echo "Testing rate: $TEST_RATE (max/2 of $RATE_MAX)"

		# Use test_set_wait_state which accepts any final state (OK directly without BUSY is acceptable)
		test_set_wait_state "Set RATE to $TEST_RATE" \
			"$DEVICE.GUIDER_RATE.RATE=$TEST_RATE" \
			"OK" 10

		test_get_value "Verify RATE is $TEST_RATE" \
			"$DEVICE.GUIDER_RATE.RATE" \
			"$TEST_RATE" "number"

		# Restore original rate
		echo "Restoring original rate: $ORIGINAL_RATE"
		test_set_wait_state "Restore RATE to $ORIGINAL_RATE" \
			"$DEVICE.GUIDER_RATE.RATE=$ORIGINAL_RATE" \
			"OK" 10
	else
		echo "Warning: Could not determine RATE max"
	fi
fi

echo ""
echo "--- Test 3.1: Optional GUIDER_RATE.DEC_RATE Item ---"

if property_exists "$DEVICE.GUIDER_RATE" "DEC_RATE"; then
	echo "Testing GUIDER_RATE.DEC_RATE..."

	# Get original DEC rate
	ORIGINAL_DEC_RATE=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$DEVICE.GUIDER_RATE.DEC_RATE" 2>&1)
	echo "Original DEC rate: $ORIGINAL_DEC_RATE"

	# Get DEC rate max
	DEC_RATE_MAX=$(get_item_max "$DEVICE.GUIDER_RATE.DEC_RATE")
	if [ -n "$DEC_RATE_MAX" ]; then
		TEST_DEC_RATE=$(awk -v max="$DEC_RATE_MAX" 'BEGIN { printf "%.2f", max / 2 }')
		echo "Testing DEC rate: $TEST_DEC_RATE (max/2 of $DEC_RATE_MAX)"

		# Use test_set_wait_state which accepts any final state (OK directly without BUSY is acceptable)
		test_set_wait_state "Set DEC_RATE to $TEST_DEC_RATE" \
			"$DEVICE.GUIDER_RATE.DEC_RATE=$TEST_DEC_RATE" \
			"OK" 10

		test_get_value "Verify DEC_RATE is $TEST_DEC_RATE" \
			"$DEVICE.GUIDER_RATE.DEC_RATE" \
			"$TEST_DEC_RATE" "number"

		# Restore original DEC rate
		echo "Restoring original DEC rate: $ORIGINAL_DEC_RATE"
		test_set_wait_state "Restore DEC_RATE to $ORIGINAL_DEC_RATE" \
			"$DEVICE.GUIDER_RATE.DEC_RATE=$ORIGINAL_DEC_RATE" \
			"OK" 10
	else
		echo "Warning: Could not determine DEC_RATE max"
	fi
else
	print_test_result "Optional item GUIDER_RATE.DEC_RATE does not exist" "SKIP"

fi

echo ""
echo "--- Test 5: Disconnect ---"

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
