#!/usr/bin/env python3
"""Task-local strict and sanitizer checks of changed Mount production code."""
from pathlib import Path
import os
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
os.chdir(root)
values = dict(line.split(" = ", 1) for line in Path("Makefile.inc").read_text().splitlines() if " = " in line)
cc = shlex.split(values["CC"])
def run(command, **kwargs):
    print("+ " + shlex.join(map(str, command)), flush=True)
    result = subprocess.run(list(map(str, command)), **kwargs)
    print("EXIT " + str(result.returncode), flush=True)
    result.check_returncode()
source = "indigo_drivers/agent_mount/indigo_agent_mount.c"
flags = shlex.split(values["CFLAGS"])
if sys.argv[1] == "strict":
    run([*cc, *flags, "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wno-unused-but-set-variable", "-Wno-sign-compare", "-Wno-missing-field-initializers", "-fsyntax-only", source])
else:
    build = root / "indigo_test/build/task-mount-sanitize"
    build.mkdir(parents=True, exist_ok=True)
    flags = ["-O1" if x == "-O3" else x for x in flags]
    flags += ["-D_GNU_SOURCE", "-I.", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    hooks = ["time=mount_test_time", "indigo_usleep=mount_test_sleep", "indigo_uni_open_tcp_server_socket_with_callback=mount_test_server", "indigo_uni_read_available=mount_test_read", "indigo_uni_write=mount_test_write", "indigo_uni_close=mount_test_close", "indigo_uni_set_socket_read_timeout=mount_test_read_timeout"]
    obj = build / "agent_mount_sanitize.o"
    run([*cc, *flags, *("-D"+x for x in hooks), "-c", source, "-o", obj])
    lib = Path(values["BUILD_LIB"])
    static = [lib / ("lib" + name + ".a") for name in ("indigo", "indigocat", "jpeg", "tiff", "raw")]
    common = ["-lusb-1.0", "-lhidapi", "-lz", "-lstdc++", "-lm", "-lpthread", "-Wl,-rpath," + str(lib)]
    test = build / "test_agent_mount_sanitize"
    run([*cc, *flags, "indigo_test/integration/test_agent_mount.c", "indigo_test/build/integration/agent_mount_framework.o", "indigo_test/build/integration/agent_mount_filter.o", obj, "-o", test, *shlex.split(values["LDFLAGS"]), "-fsanitize=address,undefined", *static, *common])
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:abort_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    run([test], env=environment)
