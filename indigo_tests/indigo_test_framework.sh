#!/bin/bash
# INDIGO Test Framework for bash
# Copyright (c) 2026 Rumen G. Bogdanovski

# INDIGO tool executable - can be overridden by setting before sourcing this script
: ${INDIGO_PROP_TOOL:="indigo_prop_tool"}

# Global counters
TESTS_PASSED=0
TESTS_FAILED=0

# Remote server option for indigo_prop_tool
REMOTE_SERVER=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Set remote server if provided
set_remote_server() {
    if [ -n "$1" ]; then
        REMOTE_SERVER="-r $1"
    fi
}

# Print test result
print_test_result() {
    local test_name="$1"
    local result="$2"
    local description="$3"
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}[FAIL]${NC} $test_name"
        if [ -n "$description" ]; then
            echo -e "       ${YELLOW}Reason:${NC} $description"
        fi
        ((TESTS_FAILED++))
    fi
}

# Print test summary
print_test_summary() {
    local total=$((TESTS_PASSED + TESTS_FAILED))
    echo ""
    echo "========================================"
    echo "Test Summary: $TESTS_PASSED passed, $TESTS_FAILED failed out of $total tests"
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${RED}Some tests failed!${NC}"
    fi
    echo "========================================"
}

# Test: Set property and wait for specific state
# Usage: test_set_wait_state "test_name" "device.property.item=value" "expected_state" timeout
test_set_wait_state() {
    local test_name="$1"
    local property_set="$2"
    local expected_state="$3"
    local timeout="${4:-10}"
    
    local output
    output=$($INDIGO_PROP_TOOL set -w "$expected_state" -s -t "$timeout" $REMOTE_SERVER "$property_set" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        # Check if final state matches expected
        if echo "$output" | tail -1 | grep -q "\[$expected_state\]"; then
            print_test_result "$test_name" "PASS"
            return 0
        else
            print_test_result "$test_name" "FAIL" "Expected state [$expected_state] not reached"
            return 1
        fi
    else
        print_test_result "$test_name" "FAIL" "Command failed or timeout"
        return 1
    fi
}

# Test: Set property with smart state transition check
# Automatically determines if BUSY state is required by checking current value
# BUSY is mandatory when target differs from current, optional when same
# Usage: test_set_transition_smart "test_name" "device.property.item=value" "device.property.item" "value" timeout [value_type]
test_set_transition_smart() {
    local test_name="$1"
    local property_set="$2"
    local property_get="$3"
    local target_value="$4"
    local timeout="${5:-10}"
    local value_type="${6:-number}"
    
    # Get current value
    local current_value
    current_value=$($INDIGO_PROP_TOOL get $REMOTE_SERVER "$property_get" 2>&1 | tr -d '[:space:]')
    local target_normalized=$(echo "$target_value" | tr -d '[:space:]')
    
    # Determine if values match
    local values_match=0
    if [ "$value_type" = "number" ]; then
        if command -v bc >/dev/null 2>&1; then
            if [ "$(echo "$current_value == $target_normalized" | bc 2>/dev/null)" = "1" ]; then
                values_match=1
            fi
        else
            if awk -v a="$current_value" -v e="$target_normalized" 'BEGIN { exit !(a == e) }' 2>/dev/null; then
                values_match=1
            fi
        fi
    else
        if [ "$current_value" = "$target_normalized" ]; then
            values_match=1
        fi
    fi
    
    # Execute the set command
    local output
    output=$($INDIGO_PROP_TOOL set -w OK -s -t "$timeout" $REMOTE_SERVER "$property_set" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_test_result "$test_name" "FAIL" "Command failed or timeout"
        return 1
    fi
    
    # Check state transitions
    local has_busy=0
    local has_ok=0
    
    if echo "$output" | grep -q "\[BUSY\]"; then
        has_busy=1
    fi
    if echo "$output" | tail -1 | grep -q "\[OK\]"; then
        has_ok=1
    fi
    
    # If target differs from current, BUSY state is MANDATORY
    if [ $values_match -eq 0 ]; then
        if [ $has_busy -eq 1 ] && [ $has_ok -eq 1 ]; then
            print_test_result "$test_name" "PASS"
            return 0
        else
            local reason=""
            if [ $has_busy -eq 0 ]; then
                reason="Missing mandatory [BUSY] state (target differs from current: $current_value -> $target_normalized)"
            elif [ $has_ok -eq 0 ]; then
                reason="Missing [OK] state"
            fi
            print_test_result "$test_name" "FAIL" "$reason"
            return 1
        fi
    else
        # Target same as current, BUSY is OPTIONAL, just need OK
        if [ $has_ok -eq 1 ]; then
            print_test_result "$test_name" "PASS"
            return 0
        else
            print_test_result "$test_name" "FAIL" "Missing [OK] state"
            return 1
        fi
    fi
}

# Test: Check state transition (e.g., BUSY -> OK)
# Usage: test_state_transition "test_name" "device.property.item=value" "initial_state" "final_state" timeout [require_initial]
# If require_initial is "optional", initial_state is not required (useful when already at target)
test_state_transition() {
    local test_name="$1"
    local property_set="$2"
    local initial_state="$3"
    local final_state="$4"
    local timeout="${5:-10}"
    local require_initial="${6:-required}"
    
    local output
    output=$($INDIGO_PROP_TOOL set -w "$final_state" -s -t "$timeout" $REMOTE_SERVER "$property_set" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        # Check for state transition
        local has_initial=0
        local has_final=0
        
        if echo "$output" | grep -q "\[$initial_state\]"; then
            has_initial=1
        fi
        if echo "$output" | tail -1 | grep -q "\[$final_state\]"; then
            has_final=1
        fi
        
        # If initial state is optional (e.g., already at target), just check final state
        if [ "$require_initial" = "optional" ]; then
            if [ $has_final -eq 1 ]; then
                print_test_result "$test_name" "PASS"
                return 0
            else
                print_test_result "$test_name" "FAIL" "Missing [$final_state] state"
                return 1
            fi
        else
            # Require both states
            if [ $has_initial -eq 1 ] && [ $has_final -eq 1 ]; then
                print_test_result "$test_name" "PASS"
                return 0
            else
                local reason=""
                if [ $has_initial -eq 0 ]; then
                    reason="Missing [$initial_state] state"
                elif [ $has_final -eq 0 ]; then
                    reason="Missing [$final_state] state"
                fi
                print_test_result "$test_name" "FAIL" "State transition [$initial_state] -> [$final_state] not observed. $reason"
                return 1
            fi
        fi
    else
        print_test_result "$test_name" "FAIL" "Command failed or timeout"
        return 1
    fi
}

# Test: Get property item value and compare
# Usage: test_get_value "test_name" "device.property.item" "expected_value" [type]
# type can be "string" (default) or "number" for numeric comparison
test_get_value() {
    local test_name="$1"
    local property_item="$2"
    local expected_value="$3"
    local value_type="${4:-string}"
    
    local actual_value
    actual_value=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$property_item" 2>&1 | tr -d '[:space:]')
    expected_value=$(echo "$expected_value" | tr -d '[:space:]')
    
    local match=0
    if [ "$value_type" = "number" ]; then
        # Compare as numbers using bc if available, otherwise awk
        if command -v bc >/dev/null 2>&1; then
            if [ "$(echo "$actual_value == $expected_value" | bc 2>/dev/null)" = "1" ]; then
                match=1
            fi
        else
            # Fallback to awk
            if awk -v a="$actual_value" -v e="$expected_value" 'BEGIN { exit !(a == e) }' 2>/dev/null; then
                match=1
            fi
        fi
    else
        # String comparison
        if [ "$actual_value" = "$expected_value" ]; then
            match=1
        fi
    fi
    
    if [ $match -eq 1 ]; then
        print_test_result "$test_name" "PASS"
        return 0
    else
        print_test_result "$test_name" "FAIL" "Expected '$expected_value', got '$actual_value'"
        return 1
    fi
}

# Test: Set value and verify it was set (for instant operations, no BUSY state required)
# Usage: test_set_and_verify "test_name" "device.property.item=value" "device.property.item" "expected_value" timeout [value_type]
test_set_and_verify() {
    local test_name="$1"
    local property_set="$2"
    local property_get="$3"
    local expected_value="$4"
    local timeout="${5:-10}"
    local value_type="${6:-string}"
    
    # Set the value
    $INDIGO_PROP_TOOL set -w OK -t "$timeout" $REMOTE_SERVER "$property_set" >/dev/null 2>&1
    local set_result=$?
    
    if [ $set_result -ne 0 ]; then
        print_test_result "$test_name" "FAIL" "Failed to set property"
        return 1
    fi
    
    # Verify the value
    local actual_value
    actual_value=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$property_get" 2>&1 | tr -d '[:space:]')
    expected_value=$(echo "$expected_value" | tr -d '[:space:]')
    
    local match=0
    if [ "$value_type" = "number" ]; then
        if command -v bc >/dev/null 2>&1; then
            if [ "$(echo "$actual_value == $expected_value" | bc 2>/dev/null)" = "1" ]; then
                match=1
            fi
        else
            if awk -v a="$actual_value" -v e="$expected_value" 'BEGIN { exit !(a == e) }' 2>/dev/null; then
                match=1
            fi
        fi
    else
        if [ "$actual_value" = "$expected_value" ]; then
            match=1
        fi
    fi
    
    if [ $match -eq 1 ]; then
        print_test_result "$test_name" "PASS"
        return 0
    else
        print_test_result "$test_name" "FAIL" "Expected '$expected_value', got '$actual_value'"
        return 1
    fi
}

# Test: Connect device with state transition check
# Usage: test_connect "device_name" timeout
test_connect() {
    local device="$1"
    local timeout="${2:-10}"
    
    test_state_transition "Connect device" \
        "$device.CONNECTION.CONNECTED=ON" \
        "BUSY" "OK" "$timeout"
    
    # Verify connection
    test_get_value "Verify CONNECTED=ON" \
        "$device.CONNECTION.CONNECTED" \
        "ON"
}

# Test: Disconnect device with state transition check
# Usage: test_disconnect "device_name" timeout
test_disconnect() {
    local device="$1"
    local timeout="${2:-10}"
    
    test_state_transition "Disconnect device" \
        "$device.CONNECTION.DISCONNECTED=ON" \
        "BUSY" "OK" "$timeout"
    
    # Verify disconnection
    test_get_value "Verify DISCONNECTED=ON" \
        "$device.CONNECTION.DISCONNECTED" \
        "ON"
    
    test_get_value "Verify CONNECTED=OFF" \
        "$device.CONNECTION.CONNECTED" \
        "OFF"
}

# Test: Connect when already connected (should be ignored or stay connected)
# Usage: test_connect_when_connected "device_name" timeout
test_connect_when_connected() {
    local device="$1"
    local timeout="${2:-10}"
    
    # Request connect again and capture output
    local output
    output=$($INDIGO_PROP_TOOL set -w OK -s -t "$timeout" $REMOTE_SERVER "$device.CONNECTION.CONNECTED=ON" 2>&1)
    
    # Check if any property updates were received (should be none)
    local has_updates=0
    if echo "$output" | grep -q "$device.CONNECTION"; then
        has_updates=1
    fi
    
    if [ $has_updates -eq 1 ]; then
        print_test_result "Connect when already connected (no updates expected)" "FAIL" "Received property updates when none expected"
    else
        print_test_result "Connect when already connected (no updates expected)" "PASS"
    fi
    
    # Should still be connected
    test_get_value "Verify still CONNECTED=ON" \
        "$device.CONNECTION.CONNECTED" \
        "ON"
}

# Test: Disconnect when already disconnected (should be ignored or stay disconnected)
# Usage: test_disconnect_when_disconnected "device_name" timeout
test_disconnect_when_disconnected() {
    local device="$1"
    local timeout="${2:-10}"
    
    # Request disconnect again and capture output
    local output
    output=$($INDIGO_PROP_TOOL set -w OK -s -t "$timeout" $REMOTE_SERVER "$device.CONNECTION.DISCONNECTED=ON" 2>&1)
    
    # Check if any property updates were received (should be none)
    local has_updates=0
    if echo "$output" | grep -q "$device.CONNECTION"; then
        has_updates=1
    fi
    
    if [ $has_updates -eq 1 ]; then
        print_test_result "Disconnect when already disconnected (no updates expected)" "FAIL" "Received property updates when none expected"
    else
        print_test_result "Disconnect when already disconnected (no updates expected)" "PASS"
    fi
    
    # Should still be disconnected
    test_get_value "Verify still DISCONNECTED=ON" \
        "$device.CONNECTION.DISCONNECTED" \
        "ON"
}

# Test: Standard connection test battery for all devices
# Performs comprehensive connection/disconnection tests
# Usage: test_connection_battery "device_name" timeout
# Returns: 0 if device ends connected, 1 if device ends disconnected
test_connection_battery() {
    local device="$1"
    local timeout="${2:-10}"
    
    echo "--- Connection Test Battery ---"
    
    local was_connected=0
    if is_device_connected "$device"; then
        echo "Device is already connected"
        was_connected=1
        
        # Test connect when already connected
        test_connect_when_connected "$device" "$timeout"
        
        # Disconnect
        test_disconnect "$device" "$timeout"
        
        # Test disconnect when already disconnected
        test_disconnect_when_disconnected "$device" "$timeout"
        
        sleep 1
    else
        echo "Device is already disconnected"
        
        # Test disconnect when already disconnected
        test_disconnect_when_disconnected "$device" "$timeout"
    fi
    
    # Connect with full state transition check
    test_connect "$device" "$timeout"
    test_connect_when_connected "$device" "$timeout"
    
    return $was_connected
}

# Helper: Check if device is connected
is_device_connected() {
    local device="$1"
    local connected
    connected=$($INDIGO_PROP_TOOL get $REMOTE_SERVER "$device.CONNECTION.CONNECTED" 2>&1)
    
    if [ "$connected" = "ON" ]; then
        return 0
    else
        return 1
    fi
}

# Helper: Get driver information from INFO property
# Usage: get_driver_info "device_name"
# Returns: Prints "driver_name driver_version" to stdout, or empty if not available
get_driver_info() {
    local device="$1"
    local driver_name
    local driver_version
    
    driver_name=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$device.INFO.DEVICE_DRIVER" 2>&1 | tr -d '"')
    driver_version=$($INDIGO_PROP_TOOL get -w OK $REMOTE_SERVER "$device.INFO.DEVICE_VERSION" 2>&1 | tr -d '"')
    
    if [ -n "$driver_name" ] && [ -n "$driver_version" ]; then
        echo "$driver_name v.$driver_version"
    fi
}

# Print test header with device, server, and driver information
# Usage: print_test_header "Test Title" "Device Name" "host:port"
print_test_header() {
    local test_title="$1"
    local device="$2"
    local remote="$3"
    
    echo "========================================"
    echo "$test_title"
    echo "Device: $device"
    echo "Server: $remote"
    
    # Get driver info if device is connected
    local driver_info
    driver_info=$(get_driver_info "$device")
    if [ -n "$driver_info" ]; then
        echo "Driver: $driver_info"
    fi
    
    echo "========================================"
    echo ""
}

# Helper: Check if a property exists, optionally check for specific items
# Usage: property_exists "device.property" ["item1,item2,..."]
# Returns: 0 if exists (and all items if specified), 1 otherwise
property_exists() {
    local property="$1"
    local items="$2"
    
    # Check if property exists
    local output
    output=$($INDIGO_PROP_TOOL list -w OK $REMOTE_SERVER "$property" 2>&1)
    
    if [ -z "$output" ] || echo "$output" | grep -qiE "error|not found|no property"; then
        return 1
    fi
    
    # If items specified, check each one
    if [ -n "$items" ]; then
        IFS=',' read -ra ITEM_ARRAY <<< "$items"
        for item in "${ITEM_ARRAY[@]}"; do
            item=$(echo "$item" | xargs)  # trim whitespace
            if ! echo "$output" | grep -q "$item"; then
                return 1
            fi
        done
    fi
    
    return 0
}

# Test: Check if a property exists with optional item checking
# Usage: test_property_exists "test_name" "device.property" ["item1,item2,..."]
test_property_exists() {
    local test_name="$1"
    local property="$2"
    local items="$3"
    
    if property_exists "$property" "$items"; then
        if [ -n "$items" ]; then
            print_test_result "$test_name" "PASS"
        else
            print_test_result "$test_name" "PASS"
        fi
        return 0
    else
        if [ -n "$items" ]; then
            print_test_result "$test_name" "FAIL" "Property '$property' or required items not found"
        else
            print_test_result "$test_name" "FAIL" "Property '$property' not found"
        fi
        return 1
    fi
}
