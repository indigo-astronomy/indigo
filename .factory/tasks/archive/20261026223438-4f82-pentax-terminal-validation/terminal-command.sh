#!/bin/bash
set -euo pipefail
cd /Users/polakovic/Development/indigo
for pattern in '(^|/)indigo_server( |$)' '[t]est_ccd_pentax_hw --'; do
  status=0
  /usr/bin/pgrep -f "$pattern" >/dev/null || status=$?
  if [[ $status == 0 ]]; then
    echo 'Stop the existing INDIGO server or Pentax hardware test before running this test.' >&2
    exit 1
  elif [[ $status != 1 ]]; then
    echo 'Cannot check competing processes; stopping before hardware access.' >&2
    exit 1
  fi
done
expected=$(/usr/bin/shasum -a 256 indigo_drivers/ccd_pentax/indigo_ccd_pentax.c)
metadata=$(build/bin/indigo_driver_metadata build/drivers/indigo_ccd_pentax.dylib)
printf '%s\n' "$metadata"
if [[ "$metadata" != *03000003* ]]; then
  echo 'Expected Pentax driver version 3.0.0.3; stopping before hardware access.' >&2
  exit 1
fi
printf 'Pentax K-m: one grouped hardware test, 60-second watchdog, no USB unplug.\n'
printf 'Source: %s\n' "$expected"
sudo /usr/bin/python3 tools/run_driver_test.py ccd_pentax --hw --no-build --type 'PENTAX K-m' 2>&1 | /usr/bin/tee /private/tmp/pentax-km-terminal-hardware.log
