#!/usr/bin/env python3

# Run the automated tests of one driver and record the run.
#
# The script builds the driver and its tests, runs them, and writes the result
# into the '## Testing' section of the driver's README.md the way
# indigo_test/AGENTS.md describes it, then regenerates TEST_SUMMARY.md with
# tools/make_test_summary.py. Nothing is committed.
#
# Hardware-free run (the default): runs every test in INTEGRATION_TESTS or
# OPT_IN_DRIVER_TESTS that is named after the driver, 'test_<driver>' or 'test_<driver>_<kind>', without the
# sanitizer builds. The run is a 'fake SDK' run when one of the tests stands in
# for a vendor SDK or system interface (kind sdk, usb, hid, sysfs or ica), and a
# 'simulator' run otherwise.
#
# Hardware run (--hw): runs the make target 'test-<driver>-hw' with '_' replaced
# by '-', or the targets given with --target. Arguments after '--' are passed to
# make, e.g. 'HW_DRIVER=indigo_ccd_touptek'. The type recorded is the model of
# the device the suite connected to, as the driver detected it; --type overrides
# it.
#
# Hot-plug run (--hot-plug): a hardware run that includes the opt-in unplug and
# replug case. It runs 'test-<driver>-hw' with HW_HOTPLUG=1 when that target
# supports it, otherwise 'test-<driver>-hotplug-hw'. The type gets the suffix
# ' (hot-plug)', so a hot-plug run and a plain hardware run of the same device
# keep separate records. Passing HW_HOTPLUG=1 to make makes a run a hot-plug
# run too.
#
# Per-device records: a hardware suite that tests several devices in one run tags
# each of its cases with the device it tests ('case_device <suite> <case> <driver>
# <device name>', written by hw_record_case_device() of
# indigo_test/hardware/hardware_device_record.h). When the run has such tags for
# the driver, one line is recorded per tagged device instead of one per run: the
# type is the model of the device (from its 'device' record, the device name
# without one; --type replaces it), the counts are the device's own cases (tagged
# cases planned, their pass records passed), and the line is OK only when all of
# them passed. Devices of the same model share one line. A run without tags is
# recorded exactly as before.
#
# Every test writes its results into the file named by INDIGO_TEST_RESULTS (see
# indigo_test/AGENTS.md), so the recorded counts do not depend on the text a
# suite prints. A run passes when every test binary or target exits with 0 and
# every planned case passed.
#
# Usage: run_driver_test.py <driver> [--hw | --hot-plug] [--target <target> ...] [--type <type>]
#                           [--no-build] [--no-record] [--dry-run] [-- <make arguments>]

import argparse
import datetime
import os
import platform
import re
import subprocess
import sys
import tempfile

# Importing make_test_summary would otherwise leave tools/__pycache__ behind in the checkout.
sys.dont_write_bytecode = True
import make_test_summary

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DIR = os.path.join(ROOT, "indigo_test")

HOT_PLUG = "HW_HOTPLUG=1"
HOT_PLUG_SUFFIX = " (hot-plug)"

FAKE_SDK_KINDS = ("sdk", "usb", "hid", "sysfs", "ica")
SANITIZER_KINDS = ("sanitize", "asan")

# Interface bits of indigo_bus.h, by driver name prefix. A hardware run records
# the devices of the driver's own class only, so a camera's filter wheel or a
# powerbox a suite switches does not become part of the name.
CLASS_INTERFACES = {
	"mount": 1 << 0,
	"ccd": 1 << 1,
	"guider": 1 << 2,
	"focuser": 1 << 3,
	"wheel": 1 << 4,
	"dome": 1 << 5,
	"gps": 1 << 6,
	"ao": 1 << 8,
	"polaralign": 1 << 9,
	"rotator": 1 << 12,
	"agent": 1 << 14,
	"aux": 1 << 15,
}

# Environment variables that select only some cases. A run with any of them set
# is not a complete run, so it is never recorded.
FILTER_VARIABLES = re.compile(r"^(INDIGO_TEST_CASE_FILTER|[A-Z0-9_]*_TEST_FILTER)$")

EXAMPLES = """
The driver is built first, then its tests run, and the result is written as one
line into the '## Testing' section of the driver's README.md, replacing the
previous run of the same operating system, architecture and type, after which
TEST_SUMMARY.md is regenerated. Nothing is committed. Build the INDIGO library
with 'make all' in the project root before the first run.

A hardware-free run executes every test named test_<driver> or
test_<driver>_<kind>, and is recorded as 'simulator' or 'fake SDK'. A hardware
run executes 'make test-<driver>-hw' and is recorded under the model of the
device the suite connected to. A hot-plug run adds the unplug and replug case
and is recorded under the model followed by ' (hot-plug)'.

examples:
  run the simulator tests of a driver and record them
    tools/run_driver_test.py focuser_dsd

  see what would be recorded, without touching any file
    tools/run_driver_test.py mount_lx200 --dry-run

  run the hardware suite, telling it which port or camera to use; anything
  else a suite needs, such as the site, goes after '--' as variables, and a
  suite prints the ones it needs when they are missing
    tools/run_driver_test.py mount_pmc8 --hw --port /dev/cu.usbserial-AK06KTVZ
    tools/run_driver_test.py ccd_atik --hw --device "Atik One"
    tools/run_driver_test.py mount_synscan --hw -- SYNSCAN_HW_LATITUDE=48.1 SYNSCAN_HW_LONGITUDE=17.1

  run the hardware suite with its hot-plug case
    tools/run_driver_test.py ccd_touptek --hot-plug

  run a hardware suite through another target and name the hardware yourself
    tools/run_driver_test.py ccd_touptek --hw --target test-ccd-touptek-hotplug-hw --type "Touptek GPCMOS01200KMB"
"""


def fail(message):
	print("run_driver_test: %s" % message, file=sys.stderr)
	sys.exit(2)


def find_driver(driver):
	"""Return the directory of the given driver."""
	for driver_root in make_test_summary.DRIVER_ROOTS:
		path = os.path.join(ROOT, driver_root, driver)
		if os.path.isdir(path):
			return path
	fail("no driver '%s' in %s" % (driver, ", ".join(make_test_summary.DRIVER_ROOTS)))


def driver_version(driver_dir, driver):
	"""Return the driver version the way INDIGO reports it, e.g. 3.0.0.17."""
	# Drivers are written in C, C++ (ccd_qhy2, ccd_qsi) or Objective-C (aux_joystick), and the
	# version can live in a local header (ccd_ptp) or in another driver's source that a variant
	# includes (ccd_altair and the other ToupTek OEM drivers include ccd_touptek), so the local
	# includes of the main source are followed until the definition turns up.
	pending = [os.path.join(driver_dir, "indigo_%s.%s" % (driver, extension)) for extension in ("c", "cpp", "m")]
	seen = set()
	match = None
	while pending and match is None:
		source = os.path.normpath(pending.pop(0))
		if source in seen or not os.path.isfile(source):
			continue
		seen.add(source)
		with open(source, "r", encoding="utf-8", errors="replace") as file:
			text = file.read()
		match = re.search(r"^#define\s+DRIVER_VERSION\s+(0x[0-9A-Fa-f]+)", text, re.M)
		pending += [os.path.join(os.path.dirname(source), include) for include in re.findall(r'^#include\s+"([^"]+)"', text, re.M)]
	if match is None:
		fail("no DRIVER_VERSION in the sources of %s" % driver)
	value = int(match.group(1), 16)
	return "%d.%d.%d.%d" % ((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF)


def host_os():
	system = platform.system()
	if system == "Darwin":
		return "mac"
	if system == "Linux":
		return "linux"
	if system == "Windows":
		return "windows"
	fail("unsupported operating system '%s'" % system)


def host_architecture():
	machine = platform.machine().lower()
	if machine in ("arm64", "aarch64"):
		return "arm64"
	if machine in ("x86_64", "amd64"):
		return "x64"
	if machine in ("i386", "i486", "i586", "i686", "x86"):
		return "x86"
	if machine.startswith("arm"):
		return "arm"
	return machine


def run(command, cwd, env=None):
	"""Run a command with its output going straight to the terminal."""
	print("+ %s" % " ".join(command), flush=True)
	return subprocess.call(command, cwd=cwd, env=env)


def build_driver(driver_dir):
	if not any(os.path.exists(os.path.join(ROOT, "build", "lib", name)) for name in ("libindigo.a", "libindigo.so", "libindigo.dylib")):
		fail("the INDIGO library is not built, run 'make all' in %s first" % ROOT)
	command = ["make", "-C", driver_dir]
	if not os.path.isfile(os.path.join(driver_dir, "Makefile")):
		command += ["-f", "../../Makefile.drv"]
	if run(command + ["all"], ROOT) != 0:
		fail("building the driver failed")


def driver_tests(driver):
	"""Return the hardware-free test binaries of the driver, relative to indigo_test.

	They come from INTEGRATION_TESTS and from OPT_IN_DRIVER_TESTS, the driver tests
	test-integration leaves out because they open loopback sockets.
	"""
	output = ""
	for variable in ("INTEGRATION_TESTS", "OPT_IN_DRIVER_TESTS"):
		output += " " + subprocess.run(["make", "-s", "print-variable", "VAR=" + variable], cwd=TEST_DIR, capture_output=True, text=True, check=True).stdout
	tests = []
	for path in output.split():
		if path in tests:
			continue
		name = os.path.basename(path)
		if name != "test_" + driver and not name.startswith("test_%s_" % driver):
			continue
		if name.rsplit("_", 1)[-1] in SANITIZER_KINDS:
			continue
		tests.append(path)
	return tests


def make_commands(target, make_arguments):
	"""Return what make would run for the target, or None when it cannot."""
	result = subprocess.run(["make", "-n", target] + make_arguments, cwd=TEST_DIR, capture_output=True, text=True)
	return result.stdout if result.returncode == 0 else None


def hot_plug_targets(driver, make_arguments):
	"""Return the targets and make arguments of a hot-plug run of the driver.

	The plain hardware target is preferred when it adds the hot-plug case for
	HW_HOTPLUG=1, a dedicated hot-plug target is used otherwise.
	"""
	name = driver.replace("_", "-")
	arguments = make_arguments + ([HOT_PLUG] if HOT_PLUG not in make_arguments else [])
	commands = make_commands("test-%s-hw" % name, arguments)
	if commands is not None and "--hotplug" in commands:
		return ["test-%s-hw" % name], arguments
	if make_commands("test-%s-hotplug-hw" % name, make_arguments) is not None:
		return ["test-%s-hotplug-hw" % name], make_arguments
	fail("%s has no hot-plug case: test-%s-hw ignores %s and there is no test-%s-hotplug-hw" % (driver, name, HOT_PLUG, name))


def hardware_free_type(tests):
	kinds = {os.path.basename(test).rsplit("_", 1)[-1] for test in tests}
	return "fake SDK" if kinds & set(FAKE_SDK_KINDS) else "simulator"


def read_results(path):
	"""Return (planned, passed, filtered, devices, tags, passes) from an INDIGO_TEST_RESULTS file.

	tags maps (suite, case) to (driver, device name) for the cases a suite tagged with
	'case_device'; passes is the set of (suite, case) that passed.
	"""
	planned = passed = 0
	filtered = False
	devices = {}
	tags = {}
	passes = set()
	try:
		with open(path, "r", encoding="utf-8", errors="replace") as file:
			lines = file.read().splitlines()
	except OSError:
		lines = []
	for line in lines:
		fields = line.split("\t")
		if fields[0] == "plan" and len(fields) >= 2:
			planned += int(fields[1])
		elif fields[0] == "pass":
			passed += 1
			if len(fields) >= 3:
				passes.add((fields[1], fields[2]))
		elif fields[0] == "case_device" and len(fields) >= 5:
			tags[(fields[1], fields[2])] = (fields[3], fields[4])
		elif fields[0] == "filter":
			filtered = True
		elif fields[0] == "device" and len(fields) >= 5:
			# The last record of a device wins.
			devices[fields[3]] = { "driver": fields[1], "interface": int(fields[2] or 0), "model": fields[4] }
	return planned, passed, filtered, devices, tags, passes


def hardware_type(driver, devices):
	"""Return the name of the hardware the suite connected to, or None."""
	own = [device for device in devices.values() if device["driver"] == "indigo_" + driver]
	interface = CLASS_INTERFACES.get(driver.split("_", 1)[0])
	if interface is not None and any(device["interface"] & interface for device in own):
		own = [device for device in own if device["interface"] & interface]
	# Sorted, so the name does not depend on which device happened to connect
	# first, and the next run of the same hardware replaces this one.
	models = sorted({device["model"] for device in own}, key=str.lower)
	return " and ".join(models) if models else None


def device_results(driver, devices, tags, passes):
	"""Return {model: (planned, passed)} of the cases tagged with devices of the driver, empty without tags."""
	results = {}
	for key, (tag_driver, name) in tags.items():
		if tag_driver != "indigo_" + driver:
			continue
		device = devices.get(name)
		model = (device["model"] if device is not None else name).strip()
		planned, passed = results.get(model, (0, 0))
		results[model] = (planned + 1, passed + (1 if key in passes else 0))
	return results


def update_readme(readme, record):
	"""Put the record into the '## Testing' section of the README.md.

	The line of the same operating system, architecture and type is replaced,
	otherwise the record is added, and the section is kept ordered by timestamp.
	"""
	new = make_test_summary.RECORD.match(record)
	key = (new.group("os"), new.group("architecture"), new.group("type"))
	with open(readme, "r", encoding="utf-8") as file:
		lines = file.read().splitlines()
	start = next((i for i, line in enumerate(lines) if make_test_summary.TESTING_HEADING.match(line)), None)
	if start is None:
		while lines and not lines[-1].strip():
			lines.pop()
		lines += ["", "## Testing", "", record]
	else:
		end = next((i for i in range(start + 1, len(lines)) if make_test_summary.NEXT_HEADING.match(lines[i])), len(lines))
		records = []
		for line in lines[start + 1:end]:
			match = make_test_summary.RECORD.match(line.strip())
			if match is None:
				if line.strip():
					print("%s: dropping malformed record '%s'" % (readme, line), file=sys.stderr)
				continue
			if (match.group("os"), match.group("architecture"), match.group("type")) != key:
				records.append(line.strip())
		records.append(record)
		records.sort(key=lambda line: make_test_summary.RECORD.match(line).group("timestamp"))
		section = ["## Testing", ""] + records
		if end < len(lines):
			section.append("")
		lines[start:end] = section
	with open(readme, "w", encoding="utf-8") as file:
		file.write("\n".join(lines) + "\n")


def main():
	arguments = sys.argv[1:]
	make_arguments = []
	if "--" in arguments:
		index = arguments.index("--")
		arguments, make_arguments = arguments[:index], arguments[index + 1:]
	parser = argparse.ArgumentParser(
		description="Run the automated tests of one driver and record the run in its README.md and TEST_SUMMARY.md.",
		usage="%(prog)s <driver> [options] [-- <make arguments>]",
		formatter_class=argparse.RawDescriptionHelpFormatter,
		epilog=EXAMPLES)
	parser.add_argument("driver", help="driver directory name, e.g. focuser_dsd")
	parser.add_argument("--hw", action="store_true", help="run the hardware suite instead of the hardware-free tests")
	parser.add_argument("--hot-plug", action="store_true", help="run the hardware suite with its unplug and replug case, recorded as '<model> (hot-plug)'")
	parser.add_argument("--target", action="append", help="make target to run for a hardware run, instead of test-<driver>-hw (repeatable)")
	parser.add_argument("--port", help="port or URL of the device for a hardware run (sets INDIGO_TEST_PORT)")
	parser.add_argument("--device", help="name of the device for a hardware run when several are attached (sets INDIGO_TEST_DEVICE)")
	parser.add_argument("--type", help="type to record instead of the detected one")
	parser.add_argument("--no-build", action="store_true", help="do not rebuild the driver first")
	parser.add_argument("--no-record", action="store_true", help="run the tests but leave README.md and TEST_SUMMARY.md alone")
	parser.add_argument("--dry-run", action="store_true", help="print the record that would be written instead of writing it")
	if not arguments:
		parser.print_help()
		return 0
	args = parser.parse_args(arguments)

	driver = args.driver.strip("/")
	driver_dir = find_driver(driver)
	hot_plug = args.hot_plug or HOT_PLUG in make_arguments
	hardware = args.hw or hot_plug
	readme = os.path.join(driver_dir, "README.md")
	if not os.path.isfile(readme):
		fail("%s has no README.md" % driver_dir)
	filters = sorted(name for name in os.environ if FILTER_VARIABLES.match(name))
	record = not args.no_record
	if record and filters:
		print("run_driver_test: %s set, the run will not be recorded" % ", ".join(filters), file=sys.stderr)
		record = False

	if not args.no_build:
		build_driver(driver_dir)
	timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
	handle, results = tempfile.mkstemp(prefix="indigo-test-results.", suffix=".txt")
	os.close(handle)
	env = dict(os.environ, INDIGO_TEST_RESULTS=results)
	# Every hardware suite falls back to these when its own variable, such as MOUNT_PMC8_HW_PORT, is not set.
	if args.port:
		env["INDIGO_TEST_PORT"] = args.port
	if args.device:
		env["INDIGO_TEST_DEVICE"] = args.device
	failures = []
	try:
		if hardware:
			if args.target:
				targets = args.target
				if args.hot_plug and HOT_PLUG not in make_arguments:
					make_arguments = make_arguments + [HOT_PLUG]
			elif args.hot_plug:
				targets, make_arguments = hot_plug_targets(driver, make_arguments)
			else:
				targets = ["test-%s-hw" % driver.replace("_", "-")]
			for target in targets:
				if run(["make", target] + make_arguments, TEST_DIR, env) != 0:
					failures.append(target)
			test_type = args.type
		else:
			if args.target:
				fail("--target applies to a hardware run only")
			tests = driver_tests(driver)
			if not tests:
				fail("no tests named test_%s or test_%s_<kind> in INTEGRATION_TESTS" % (driver, driver))
			if run(["make", "check-lib"] + tests + make_arguments, TEST_DIR) != 0:
				fail("building the tests failed")
			for test in tests:
				if run(["./" + test], TEST_DIR, env) != 0:
					failures.append(os.path.basename(test))
			test_type = args.type or hardware_free_type(tests)
		planned, passed, filtered, devices, tags, passes = read_results(results)
	finally:
		os.unlink(results)

	# a hardware run whose cases are tagged with their devices is recorded per device, see the top of this file
	per_device = device_results(driver, devices, tags, passes) if hardware else {}
	if hardware and test_type is None and not per_device:
		test_type = hardware_type(driver, devices)
	if hot_plug and test_type is not None and not test_type.endswith(HOT_PLUG_SUFFIX):
		test_type += HOT_PLUG_SUFFIX
	ok = not failures and planned > 0 and passed == planned
	print()
	for failure in failures:
		print("FAILED: %s" % failure)
	print("%s: %d/%d %s" % (driver, planned, passed, "OK" if ok else "Failed"))

	if filtered and record:
		print("run_driver_test: only some cases were selected, the run will not be recorded", file=sys.stderr)
		record = False
	if hardware and planned == 0:
		print("run_driver_test: the hardware suite ran no case. It usually has to be told which device or port to use:"
			" pass --port <port> or --device <name>, or the variables its message above asks for after '--'", file=sys.stderr)
	if per_device:
		lines = []
		for model in sorted(per_device, key=str.lower):
			device_planned, device_passed = per_device[model]
			device_type = args.type or model
			if hot_plug and not device_type.endswith(HOT_PLUG_SUFFIX):
				device_type += HOT_PLUG_SUFFIX
			device_ok = device_planned > 0 and device_passed == device_planned
			print("%s: %s %d/%d %s" % (driver, device_type, device_planned, device_passed, "OK" if device_ok else "Failed"))
			lines.append("%s %s %s %s %s %d/%d %s" % (timestamp, driver_version(driver_dir, driver), host_os(), host_architecture(), device_type, device_planned, device_passed, "OK" if device_ok else "Failed"))
		for line in lines:
			if make_test_summary.RECORD.match(line) is None:
				fail("cannot record '%s', the type does not fit the record format" % line)
		if args.dry_run:
			print("would record in %s:\n%s" % (os.path.relpath(readme, ROOT), "\n".join(lines)))
		elif record:
			for line in lines:
				update_readme(readme, line)
			print("recorded in %s:\n%s" % (os.path.relpath(readme, ROOT), "\n".join(lines)))
			subprocess.run([sys.executable, os.path.join(ROOT, "tools", "make_test_summary.py")], check=True)
	elif test_type is None:
		if record or args.dry_run:
			print("run_driver_test: no device of %s was connected, the run cannot be recorded without --type" % driver, file=sys.stderr)
		record = False
	elif record or args.dry_run:
		line = "%s %s %s %s %s %d/%d %s" % (timestamp, driver_version(driver_dir, driver), host_os(), host_architecture(), test_type, planned, passed, "OK" if ok else "Failed")
		if make_test_summary.RECORD.match(line) is None:
			fail("cannot record '%s', the type does not fit the record format" % line)
		if args.dry_run:
			print("would record in %s:\n%s" % (os.path.relpath(readme, ROOT), line))
		elif record:
			update_readme(readme, line)
			print("recorded in %s:\n%s" % (os.path.relpath(readme, ROOT), line))
			subprocess.run([sys.executable, os.path.join(ROOT, "tools", "make_test_summary.py")], check=True)
	return 0 if ok else 1


if __name__ == "__main__":
	sys.exit(main())
