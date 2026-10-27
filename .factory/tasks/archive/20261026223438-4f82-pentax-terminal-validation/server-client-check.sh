#!/bin/bash
set -euo pipefail
cd /Users/polakovic/Development/indigo
prop=build/bin/indigo_prop_tool
camera='PENTAX K-m'
cleanup() {
  "$prop" set -r 127.0.0.1:7624 -t 5 -w OK "$camera.CONNECTION.DISCONNECTED=ON" || true
}
# This client is diagnostic only: it never loads a driver or opens USB itself.
# Require the expected camera to have been enumerated by the actual server.
listing=$("$prop" list -r 127.0.0.1:7624 -t 1 "$camera.CONNECTION")
printf '%s\n' "$listing"
[[ "$listing" == *"$camera.CONNECTION.DISCONNECTED"* ]] || { echo 'K-m not enumerated by server.' >&2; exit 1; }
trap cleanup EXIT
"$prop" set -r 127.0.0.1:7624 -t 8 -w OK "$camera.CONNECTION.CONNECTED=ON"
state=$("$prop" get_state -r 127.0.0.1:7624 -t 0.3 "$camera.CONNECTION")
value=$("$prop" get -r 127.0.0.1:7624 -t 0.3 "$camera.CONNECTION.CONNECTED")
printf 'Connection state=%s connected=%s\n' "$state" "$value"
[[ "$state" == OK && "$value" == ON ]] || { echo 'Camera connection failed.' >&2; exit 1; }
"$prop" list -r 127.0.0.1:7624 -t 0.5 "$camera.INFO"
"$prop" list -r 127.0.0.1:7624 -t 0.5 "$camera.DSLR_ISO"
"$prop" list -r 127.0.0.1:7624 -t 0.5 "$camera.DSLR_SHUTTER"
"$prop" list -r 127.0.0.1:7624 -t 0.5 "$camera.DSLR_APERTURE"
"$prop" set -r 127.0.0.1:7624 -t 5 -w OK "$camera.CONNECTION.DISCONNECTED=ON"
state=$("$prop" get_state -r 127.0.0.1:7624 -t 0.3 "$camera.CONNECTION")
value=$("$prop" get -r 127.0.0.1:7624 -t 0.3 "$camera.CONNECTION.DISCONNECTED")
[[ "$state" == OK && "$value" == ON ]] || { echo 'Camera disconnect failed.' >&2; exit 1; }
trap - EXIT
echo 'PASS: actual server connected to K-m, exposed status and disconnected.'
