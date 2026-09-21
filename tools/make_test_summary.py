#!/usr/bin/env python3

# Collect driver test run records into a single TEST_SUMMARY.md.
#
# The script walks every driver directory under indigo_drivers,
# indigo_mac_drivers, indigo_linux_drivers and indigo_optional_drivers, reads
# the '## Testing' section of the driver's README.md and copies the recorded
# runs into TEST_SUMMARY.md in the project root.
#
# A test run record has the form
# '<timestamp> <version> <os> <architecture> <type> <result>' as described in
# indigo_test/AGENTS.md, e.g.
#
#   2026-09-20 14:32 3.0.0.8 mac arm64 simulator 12/12 OK
#   2026-09-20 15:04 3.0.0.7 linux x64 Optec FocusLynx 12/11 Failed
#
# Every recorded run becomes one row of a single markdown table with the
# columns driver, timestamp, version, platform, type, tests and result, where
# the driver is the driver directory name, the platform joins the operating
# system and the architecture, the tests column holds the total and passed
# counts and the result column holds either '✅ OK' or '❌ Failed'.
#
# Usage: make_test_summary.py [-o <output>]
#   -o <output>  file to write instead of <project root>/TEST_SUMMARY.md,
#                '-' writes to standard output

import argparse
import os
import re
import sys

DRIVER_ROOTS = [
	"indigo_drivers",
	"indigo_mac_drivers",
	"indigo_linux_drivers",
	"indigo_optional_drivers",
]

TESTING_HEADING = re.compile(r"^##\s+Testing\s*$")
NEXT_HEADING = re.compile(r"^#{1,6}\s")
RECORD = re.compile(
	r"^(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2})"
	r"\s+(?P<version>\S+)"
	r"\s+(?P<os>mac|linux|windows)"
	r"\s+(?P<architecture>\S+)"
	r"\s+(?P<type>\S.*?)"
	r"\s+(?P<total>\d+)/(?P<passed>\d+)"
	r"\s+(?P<result>\S+)$"
)
COLUMNS = ["Driver", "Timestamp", "Version", "Platform", "Type", "Tests", "Result"]
PASSED = "✅ OK"
FAILED = "❌ Failed"


def read_testing_section(readme):
	"""Return the lines of the '## Testing' section of the given README.md."""
	lines = []
	with open(readme, "r", encoding="utf-8") as file:
		in_section = False
		for line in file:
			line = line.rstrip("\n")
			if in_section:
				if NEXT_HEADING.match(line):
					break
				lines.append(line)
			elif TESTING_HEADING.match(line):
				in_section = True
	return lines


def read_records(readme):
	"""Return the test run records of the given README.md as match objects."""
	records = []
	for line in read_testing_section(readme):
		line = line.strip()
		if not line:
			continue
		match = RECORD.match(line)
		if match is None:
			print("%s: ignoring malformed record '%s'" % (readme, line), file=sys.stderr)
			continue
		records.append(match)
	return records


def collect(root):
	"""Return the (name, records) pairs of all tested drivers."""
	drivers = []
	for driver_root in DRIVER_ROOTS:
		path = os.path.join(root, driver_root)
		if not os.path.isdir(path):
			continue
		for name in sorted(os.listdir(path)):
			readme = os.path.join(path, name, "README.md")
			if not os.path.isfile(readme):
				continue
			records = read_records(readme)
			if not records:
				continue
			drivers.append((name, records))
	return drivers


def format_row(cells):
	"""Return one markdown table row for the given cells."""
	return "| " + " | ".join(cells) + " |"


def format_summary(drivers):
	"""Return the content of TEST_SUMMARY.md for the given drivers."""
	lines = [format_row(COLUMNS), format_row(["---"] * len(COLUMNS))]
	for name, records in drivers:
		for record in records:
			lines.append(format_row([
				name,
				record.group("timestamp"),
				record.group("version"),
				"%s %s" % (record.group("os"), record.group("architecture")),
				record.group("type"),
				"%s / %s" % (record.group("total"), record.group("passed")),
				PASSED if record.group("result") == "OK" else FAILED
			]))
	return "\n".join(lines) + "\n"


def main():
	parser = argparse.ArgumentParser(description="Collect driver test run records into TEST_SUMMARY.md.")
	parser.add_argument("-o", "--output", help="file to write instead of TEST_SUMMARY.md, '-' for standard output")
	args = parser.parse_args()
	root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
	drivers = collect(root)
	summary = format_summary(drivers)
	if args.output == "-":
		sys.stdout.write(summary)
	else:
		output = args.output or os.path.join(root, "TEST_SUMMARY.md")
		with open(output, "w", encoding="utf-8") as file:
			file.write(summary)
		print("%d driver(s) written to %s" % (len(drivers), output))


if __name__ == "__main__":
	main()
