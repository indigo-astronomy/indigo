#!/bin/bash
set -euo pipefail
if [[ ${1:-} != --bounded ]]; then
  exec sudo -n timeout --signal=TERM --kill-after=5 90 bash "$0" --bounded
fi
root=/tmp/indigo-pentax-20261027181603
cd "$root"
expected=b7b78630bfeb2855891ec1450885e5dd9d53736e9132bdf000ee3c4584651830
actual=$(sha256sum indigo_drivers/ccd_pentax/indigo_ccd_pentax.c)
[[ ${actual%% *} == "$expected" ]] || { echo 'Driver source identity mismatch'; exit 2; }
status=0
pgrep -f '(^|/)(indigo_server|test_ccd_pentax_hw)( |$)' >/dev/null || status=$?
[[ $status == 1 ]] || { echo 'Existing server/test or process inspection failure'; exit 2; }
run_dir=$(mktemp -d "$root/bulb-accuracy-XXXXXXXX")
echo "RESULTS=$run_dir"
for duration in 1.2 3; do
  sudo -n env LD_LIBRARY_PATH="$root/build/lib" PENTAX_HW_EXPOSURE="$duration" PENTAX_HW_IMAGE_PATH="$run_dir/$duration.native" timeout --signal=TERM --kill-after=5 40 python3 tools/run_driver_test.py ccd_pentax --hw --no-build --no-record --type 'PENTAX K-m bulb timing (single frame)' -- PENTAX_HW_ARGS=--single-frame >"$run_dir/$duration.log" 2>&1 || { cat "$run_dir/$duration.log"; exit 1; }
  tail -8 "$run_dir/$duration.log"
done
echo "COMPLETE=$run_dir"
