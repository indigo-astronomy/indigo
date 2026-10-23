#!/bin/sh
set -eu
probe_task_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
probe_root=$(git rev-parse --show-toplevel)
cd "$probe_root"
probe_build=$(mktemp -d /private/tmp/indigo-fits-005.XXXXXX)
trap 'rm -rf "$probe_build"' EXIT HUP INT TERM
clang --version
for probe_mode in plain sanitize; do
  probe_flags="-O2 -g -Wall -Wextra -Werror"
  if [ "$probe_mode" = sanitize ]; then
    probe_flags="$probe_flags -fsanitize=address,undefined"
  fi
  clang $probe_flags -Iindigo_libs -c indigo_libs/indigo_fits.c -o "$probe_build/fits.o"
  ar rcs "$probe_build/libfits.a" "$probe_build/fits.o"
  clang $probe_flags -Wno-unused-function -Iindigo_libs indigo_test/unit/test_raw_to_fits.c "$probe_build/libfits.a" -o "$probe_build/test-fits"
  "$probe_build/test-fits"
done
clang -g -fsanitize=address,undefined -Iindigo_libs -Drealloc=fits_test_realloc -c indigo_libs/indigo_fits.c -o "$probe_build/fault.o"
ar rcs "$probe_build/libfault.a" "$probe_build/fault.o"
clang -g -fsanitize=address,undefined -Iindigo_libs "$probe_task_dir/allocation-failure.c" "$probe_build/libfault.a" -o "$probe_build/test-fault"
"$probe_build/test-fault"
printf '%s\n' 'PASS allocation failure at all five allocation sites; success after fault budget exhausted'
make -B -C indigo_test build/unit/test_raw_to_fits TEST_CFLAGS='-O2 -g -Wall -Wextra -Werror -Wno-unused-function -I../indigo_libs -I..' TEST_LDFLAGS="$probe_build/libfits.a -fsanitize=address,undefined"
indigo_test/build/unit/test_raw_to_fits
make -C indigo_test test-clean
clang -g -Wno-deprecated-declarations -fsanitize=address,undefined -Iindigo_libs indigo_tools/indigo_raw_to_fits.c "$probe_build/libfits.a" -o "$probe_build/raw-to-fits"
python3 - "$probe_build" <<'PYCLI'
from pathlib import Path
import struct, subprocess, sys
root = Path(sys.argv[1])
valid = root / 'valid.raw'
valid.write_bytes(b'RAW1' + struct.pack('=II', 1, 1) + bytes([42]) + b'SIMPLE=T;' + b'GAIN=2;' * 100)
subprocess.run([str(root / 'raw-to-fits'), '-q', '-e', '1.5', '-b', 'RGGB', str(valid)], check=True)
fits = valid.with_suffix('.fits').read_bytes()
assert len(fits) == 5 * 2880 and fits[4 * 2880] == 42
assert b'BAYERPAT' in fits[:4 * 2880] and b'EXPTIME' in fits[:4 * 2880]
invalid = root / 'invalid.raw'
invalid.write_bytes(b'R')
result = subprocess.run([str(root / 'raw-to-fits'), '-q', str(invalid)])
assert result.returncode == 1 and not invalid.with_suffix('.fits').exists()
print('PASS command-line converter: metadata spans records; truncated file rejected')
PYCLI
