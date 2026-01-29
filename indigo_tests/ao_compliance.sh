#!/bin/bash
# INDIGO AO Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: ao_compliance.sh "Device Name" [host:port]

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

print_test_header "INDIGO AO Compliance Test" "$DEVICE" "$REMOTE"

# Test 0: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory Properties ---"

test_property_exists "Test AO_GUIDE_DEC property exists" "$DEVICE.AO_GUIDE_DEC"
test_property_exists "Test AO_GUIDE_RA property exists" "$DEVICE.AO_GUIDE_RA"

echo ""
echo "--- Test 1: AO RA Tests ---"

# Get RA guide limits (RA is EAST/WEST)
RA_EAST_MAX=$(get_item_max "$DEVICE.AO_GUIDE_RA.EAST")
RA_WEST_MAX=$(get_item_max "$DEVICE.AO_GUIDE_RA.WEST")

if [ -z "$RA_EAST_MAX" ] || [ -z "$RA_WEST_MAX" ]; then
	echo "Error: Could not determine AO RA limits"
else
	echo "AO RA limits: EAST max=$RA_EAST_MAX, WEST max=$RA_WEST_MAX"

	# Use 500ms or max, whichever is smaller
	EAST_PULSE=$(awk -v max="$RA_EAST_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')
	WEST_PULSE=$(awk -v max="$RA_WEST_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')

	echo "Testing EAST pulse: ${EAST_PULSE}ms"
	test_state_transition "Guide EAST ${EAST_PULSE}ms" \
		"$DEVICE.AO_GUIDE_RA.EAST=$EAST_PULSE" \
		"BUSY" "OK" 10

	test_get_value "Verify AO_GUIDE_RA.EAST = 0" \
	"$DEVICE.AO_GUIDE_RA.EAST" \
	0 "number"

	echo "Testing WEST pulse: ${WEST_PULSE}ms"
	test_state_transition "Guide WEST ${WEST_PULSE}ms" \
		"$DEVICE.AO_GUIDE_RA.WEST=$WEST_PULSE" \
		"BUSY" "OK" 10

	test_get_value "Verify AO_GUIDE_RA.WEST = 0" \
	"$DEVICE.AO_GUIDE_RA.WEST" \
	0 "number"
fi

echo ""
echo "--- Test 2: AO DEC Tests ---"

# Get DEC guide limits (DEC is NORTH/SOUTH)
DEC_NORTH_MAX=$(get_item_max "$DEVICE.AO_GUIDE_DEC.NORTH")
DEC_SOUTH_MAX=$(get_item_max "$DEVICE.AO_GUIDE_DEC.SOUTH")
if [ -z "$DEC_NORTH_MAX" ] || [ -z "$DEC_SOUTH_MAX" ]; then
	echo "Error: Could not determine AO DEC limits"
else
	echo "AO DEC limits: NORTH max=$DEC_NORTH_MAX, SOUTH max=$DEC_SOUTH_MAX"

	# Use 500ms or max, whichever is smaller
	NORTH_PULSE=$(awk -v max="$DEC_NORTH_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')
	SOUTH_PULSE=$(awk -v max="$DEC_SOUTH_MAX" 'BEGIN { pulse = (500 < max) ? 500 : max; printf "%.0f", pulse }')

	echo "Testing NORTH pulse: ${NORTH_PULSE}ms"
	test_state_transition "Guide NORTH ${NORTH_PULSE}ms" \
		"$DEVICE.AO_GUIDE_DEC.NORTH=$NORTH_PULSE" \
		"BUSY" "OK" 10

	test_get_value "Verify AO_GUIDE_DEC.NORTH = 0" \
	"$DEVICE.AO_GUIDE_DEC.NORTH" \
	0 "number"

	echo "Testing SOUTH pulse: ${SOUTH_PULSE}ms"
	test_state_transition "Guide SOUTH ${SOUTH_PULSE}ms" \
		"$DEVICE.AO_GUIDE_DEC.SOUTH=$SOUTH_PULSE" \
		"BUSY" "OK" 10

	test_get_value "Verify AO_GUIDE_DEC.SOUTH = 0" \
	"$DEVICE.AO_GUIDE_DEC.SOUTH" \
	0 "number"
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
