#!/bin/bash
set -euo pipefail
cd /Users/polakovic/Development/indigo
status=0
/usr/bin/pgrep -x indigo_server >/dev/null || status=$?
if [[ $status == 0 ]]; then
  echo 'An INDIGO server is already running; stopping.' >&2
  /usr/bin/pgrep -lx indigo_server >&2
  exit 1
elif [[ $status != 1 ]]; then
  echo 'Cannot inspect competing processes; stopping.' >&2
  exit 1
fi
status=0
/usr/bin/pgrep -f '(^|/)test_ccd_pentax_hw --' >/dev/null || status=$?
if [[ $status != 1 ]]; then
  echo 'An existing test was found or process inspection failed; stopping.' >&2
  exit 1
fi
metadata=$(build/bin/indigo_driver_metadata build/drivers/indigo_ccd_pentax.dylib)
printf '%s\n' "$metadata"
[[ "$metadata" == *03000004* ]] || { echo 'Expected driver 3.0.0.4.' >&2; exit 1; }
echo 'Starting the actual INDIGO server. A separate client will test connection and status.'
echo 'After the client disconnects the camera, press Ctrl-C here to stop the server.'
sudo build/bin/indigo_server -vv indigo_ccd_pentax 2>&1 | /usr/bin/tee /private/tmp/pentax-km-server.log
