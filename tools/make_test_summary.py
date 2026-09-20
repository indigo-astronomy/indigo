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
# Each driver becomes one chapter titled '# <driver name> (<driver label>)',
# where the name is the driver directory name and the label is taken from the
# 'label' attribute of the driver block in the driver's .driver file.
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
	r"\s+(?P<result>\d+/\d+\s+\S+)$"
)
DRIVER_BLOCK = re.compile(r"^driver\s+(\S+)\s*\{")
LABEL = re.compile(r"^label\s*=\s*\"(.*)\"\s*;")


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
	"""Return the test run records of the given README.md as (line, match) pairs."""
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


def read_label(directory):
	"""Return the label of the driver defined in the .driver file of the directory."""
	names = sorted(name for name in os.listdir(directory) if name.endswith(".driver"))
	for name in names:
		depth = 0
		with open(os.path.join(directory, name), "r", encoding="utf-8") as file:
			for line in file:
				line = line.strip()
				if depth == 0:
					if DRIVER_BLOCK.match(line):
						depth = 1
					continue
				if depth == 1:
					match = LABEL.match(line)
					if match:
						return match.group(1)
				depth += line.count("{") - line.count("}")
				if depth <= 0:
					break
	return None


def collect(root):
	"""Return the (name, label, records) triplets of all tested drivers."""
	drivers = []
	for driver_root in DRIVER_ROOTS:
		path = os.path.join(root, driver_root)
		if not os.path.isdir(path):
			continue
		for name in sorted(os.listdir(path)):
			directory = os.path.join(path, name)
			readme = os.path.join(directory, "README.md")
			if not os.path.isfile(readme):
				continue
			records = read_records(readme)
			if not records:
				continue
			drivers.append((name, read_label(directory), records))
	return drivers


def format_summary(drivers):
	"""Return the content of TEST_SUMMARY.md for the given drivers."""
	lines = []
	for name, label, records in drivers:
		if lines:
			lines.append("")
		lines.append("# %s (%s)" % (name, label) if label else "# %s" % name)
		lines.append("")
		for record in records:
			lines.append("%s %s %s %s %s %s" % (
				record.group("timestamp"),
				record.group("version"),
				record.group("os"),
				record.group("architecture"),
				record.group("type"),
				" ".join(record.group("result").split())
			))
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
