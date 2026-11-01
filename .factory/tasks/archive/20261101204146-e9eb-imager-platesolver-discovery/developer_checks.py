#!/usr/bin/env python3
"""Task-local checks; no shared build outputs are mutated."""
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

def native(flags):
    result = []
    skip = False
    for flag in shlex.split(flags):
        if skip:
            skip = False
            continue
        if flag == "-arch":
            skip = True
            continue
        result.append("-O1" if flag == "-O3" else flag)
    return [*result, "-arch", "arm64"]

source = "indigo_drivers/agent_imager/indigo_agent_imager.c"
if sys.argv[1] == "strict":
    flags = shlex.split(values["CFLAGS"])
    run([*cc, *flags, "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wno-unused-but-set-variable", "-Wno-sign-compare", "-Wno-missing-field-initializers", "-fsyntax-only", source])
else:
    build = root / "indigo_test/build/task-imager-sanitize"
    build.mkdir(parents=True, exist_ok=True)
    flags = [*native(values["CFLAGS"]), "-D_GNU_SOURCE", "-I.", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    link = [*native(values["LDFLAGS"]), "-fsanitize=address,undefined"]
    obj = build / "agent_imager_sanitize.o"
    run([*cc, *flags, "-c", source, "-o", obj])
    lib = Path(values["BUILD_LIB"])
    static = [lib / ("lib" + name + ".a") for name in ("indigo", "indigocat", "jpeg", "tiff", "raw")]
    common = ["-lusb-1.0", "-lhidapi", "-lz", "-lstdc++", "-lm", "-lpthread", "-Wl,-rpath," + str(lib)]
    test = build / "test_agent_imager_sanitize"
    run([*cc, *flags, "indigo_test/integration/test_agent_imager.c", "indigo_test/build/integration/agent_imager_framework.o", "indigo_test/build/integration/agent_imager_filter.o", obj, "build/drivers/indigo_ccd_simulator.a", "-o", test, *link, *static, *common])
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:abort_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    run([test], env=environment)
    numeric = build / "test_agent_imager_numerics_sanitize"
    numeric_environment = dict(environment, CC=values["CC"], CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer", LDFLAGS="-fsanitize=address,undefined")
    run([sys.executable, "indigo_test/integration/test_agent_imager_numerics.py", numeric], env=numeric_environment)
    run([numeric], env=environment)
