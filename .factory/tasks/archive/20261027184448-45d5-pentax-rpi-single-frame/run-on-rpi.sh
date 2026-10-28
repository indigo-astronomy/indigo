#!/bin/bash
set -euo pipefail
cd /tmp/indigo-pentax-20261027181603
status=0
pgrep -x indigo_server >/dev/null || status=$?
if [[ $status != 1 ]]; then
  echo 'Existing server found or process inspection failed; stopping.' >&2
  exit 2
fi
status=0
pgrep -f '(^|/)test_ccd_pentax_hw --' >/dev/null || status=$?
if [[ $status != 1 ]]; then
  echo 'Existing test found or process inspection failed; stopping.' >&2
  exit 2
fi
run_dir=$(mktemp -d /tmp/indigo-pentax-20261027181603/single-frame-XXXXXXXX)
printf 'One 0.1s exposure; native image and log: %s\n' "$run_dir"
sudo -n env LD_LIBRARY_PATH=/tmp/indigo-pentax-20261027181603/build/lib PENTAX_HW_IMAGE_PATH="$run_dir/frame.native" python3 tools/run_driver_test.py ccd_pentax --hw --no-build --type 'PENTAX K-m (single frame)' -- PENTAX_HW_ARGS=--single-frame 2>&1 | tee "$run_dir/test.log"
ls -lh "$run_dir/frame.native"
