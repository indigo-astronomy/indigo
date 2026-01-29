#!/bin/bash
# INDIGO GPS Driver Compliance Tool
# Copyright (c) 2026 Rumen G. Bogdanovski
#
# Usage: gps_compliance.sh "Device Name" [host:port]

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
	echo "  $0 \"GPS Simulator\""
	echo "  $0 \"GPS Simulator\" localhost:7624"
	echo "  $0 \"GPS Simulator\" 192.168.1.100:7624"
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

print_test_header "INDIGO GPS Driver Compliance Test" "$DEVICE" "$REMOTE"

# Verify device has GPS interface
echo "--- Verifying Device Interface ---"
if is_gps "$DEVICE"; then
	print_test_result "Device implements GPS interface" "PASS"
else
	print_test_result "Device implements GPS interface" "FAIL" "This compliance test is only for GPS devices"
	exit 1
fi
echo ""

# Test 0: Run standard connection test battery
test_connection_battery "$DEVICE" 10
WAS_CONNECTED=$?

echo ""
echo "--- Testing Mandatory Properties ---"

# GEOGRAPHIC_COORDINATES is mandatory for GPS devices
test_property_exists "Test GEOGRAPHIC_COORDINATES property exists" "$DEVICE.GEOGRAPHIC_COORDINATES"
test_property_exists "Test GEOGRAPHIC_COORDINATES.LATITUDE item exists" "$DEVICE.GEOGRAPHIC_COORDINATES" "LATITUDE"
test_property_exists "Test GEOGRAPHIC_COORDINATES.LONGITUDE item exists" "$DEVICE.GEOGRAPHIC_COORDINATES" "LONGITUDE"
test_property_exists "Test GEOGRAPHIC_COORDINATES.ELEVATION item exists" "$DEVICE.GEOGRAPHIC_COORDINATES" "ELEVATION"

echo ""
echo "--- Testing Optional GPS-Specific Properties ---"

# UTC_TIME is optional - will show SKIP if not present
test_property_exists "Test UTC_TIME property exists" "$DEVICE.UTC_TIME"
test_property_exists "Test UTC_TIME.TIME item exists" "$DEVICE.UTC_TIME" "TIME"

# UTC_TIME.OFFSET is optional item (like GUIDER_RATE.DEC_RATE)
if property_exists "$DEVICE.UTC_TIME" "OFFSET"; then
	print_test_result "Test UTC_TIME.OFFSET item exists" "PASS"
else
	print_test_result "Optional item UTC_TIME.OFFSET does not exist" "SKIP"
fi

# GPS_STATUS is optional (light property)
test_property_exists "Test GPS_STATUS property exists" "$DEVICE.GPS_STATUS"
test_property_exists "Test GPS_STATUS.NO_FIX item exists" "$DEVICE.GPS_STATUS" "NO_FIX"
test_property_exists "Test GPS_STATUS.2D_FIX item exists" "$DEVICE.GPS_STATUS" "2D_FIX"
test_property_exists "Test GPS_STATUS.3D_FIX item exists" "$DEVICE.GPS_STATUS" "3D_FIX"

# GPS_ADVANCED is optional
test_property_exists "Test GPS_ADVANCED property exists" "$DEVICE.GPS_ADVANCED"
test_property_exists "Test GPS_ADVANCED.ENABLED item exists" "$DEVICE.GPS_ADVANCED" "ENABLED"
test_property_exists "Test GPS_ADVANCED.DISABLED item exists" "$DEVICE.GPS_ADVANCED" "DISABLED"

echo ""
echo "--- Testing GEOGRAPHIC_COORDINATES Values ---"

# Test reading GEOGRAPHIC_COORDINATES values (read-only for GPS)
if property_exists "$DEVICE.GEOGRAPHIC_COORDINATES"; then
	# Get current values
	LATITUDE=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GEOGRAPHIC_COORDINATES.LATITUDE" 2>&1)
	LONGITUDE=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GEOGRAPHIC_COORDINATES.LONGITUDE" 2>&1)
	ELEVATION=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GEOGRAPHIC_COORDINATES.ELEVATION" 2>&1)
	
	if [ -n "$LATITUDE" ] && [ -n "$LONGITUDE" ] && [ -n "$ELEVATION" ]; then
		echo "Current coordinates: LAT=$LATITUDE, LON=$LONGITUDE, ELEV=$ELEVATION"
		print_test_result "Read GEOGRAPHIC_COORDINATES values" "PASS"
	else
		print_test_result "Read GEOGRAPHIC_COORDINATES values" "FAIL" "Could not read coordinate values"
	fi
	
	# Test ACCURACY if present (GPS-specific optional item)
	if property_exists "$DEVICE.GEOGRAPHIC_COORDINATES" "ACCURACY"; then
		ACCURACY=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GEOGRAPHIC_COORDINATES.ACCURACY" 2>&1)
		if [ -n "$ACCURACY" ]; then
			echo "GPS accuracy: $ACCURACY"
			print_test_result "Read GEOGRAPHIC_COORDINATES.ACCURACY value" "PASS"
		else
			print_test_result "Read GEOGRAPHIC_COORDINATES.ACCURACY value" "FAIL" "Could not read accuracy value"
		fi
	fi
fi

echo ""
echo "--- Testing UTC_TIME Values ---"

# Test reading UTC_TIME if available
if property_exists "$DEVICE.UTC_TIME"; then
	UTC=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.UTC_TIME.TIME" 2>&1)
	
	if [ -n "$UTC" ]; then
		echo "Current UTC time: TIME=$UTC"
		# OFFSET is optional
		OFFSET=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.UTC_TIME.OFFSET" 2>&1)
		if [ -n "$OFFSET" ]; then
			echo "UTC OFFSET: $OFFSET"
		fi
		print_test_result "Read UTC_TIME values" "PASS"
	else
		print_test_result "Read UTC_TIME values" "FAIL" "Could not read UTC TIME value"
	fi
fi

echo ""
echo "--- Testing GPS_ADVANCED Property ---"

# Test GPS_ADVANCED if available
if property_exists "$DEVICE.GPS_ADVANCED"; then
	# Get current state
	ADVANCED_ENABLED=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$DEVICE.GPS_ADVANCED.ENABLED" 2>&1)
	echo "GPS_ADVANCED current state: ENABLED=$ADVANCED_ENABLED"
	
	# Try to enable advanced status
	test_set_wait_state "Enable GPS advanced status" \
		"$DEVICE.GPS_ADVANCED.ENABLED=ON" \
		"OK" 5
	
	# Try to disable advanced status
	test_set_wait_state "Disable GPS advanced status" \
		"$DEVICE.GPS_ADVANCED.DISABLED=ON" \
		"OK" 5
fi

echo ""
echo "--- Testing GPS_STATUS State ---"

# Test GPS_STATUS if available (read-only light property)
if property_exists "$DEVICE.GPS_STATUS"; then
	NO_FIX=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GPS_STATUS.NO_FIX" 2>&1)
	FIX_2D=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GPS_STATUS.2D_FIX" 2>&1)
	FIX_3D=$($INDIGO_PROP_TOOL get -w ANY $REMOTE_SERVER "$DEVICE.GPS_STATUS.3D_FIX" 2>&1)
	
	echo "GPS fix status: NO_FIX=$NO_FIX, 2D_FIX=$FIX_2D, 3D_FIX=$FIX_3D"
	
	# At least one should be ON
	if [ "$NO_FIX" = "ON" ] || [ "$FIX_2D" = "ON" ] || [ "$FIX_3D" = "ON" ]; then
		print_test_result "GPS_STATUS has valid state" "PASS"
	else
		print_test_result "GPS_STATUS has valid state" "FAIL" "No fix status indicator is ON"
	fi
fi

echo ""
echo "--- Restoring Connection State ---"

# Restore original connection state
if [ $WAS_CONNECTED -eq 1 ]; then
	echo "Device was originally connected, leaving it connected"
else
	echo "Device was originally disconnected, disconnecting..."
	test_disconnect "$DEVICE" 10
fi

echo ""
print_test_summary
