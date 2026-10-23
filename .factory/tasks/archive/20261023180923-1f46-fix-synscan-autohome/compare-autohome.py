#!/usr/bin/env python3
"""Compare retained original AUTOHOME writes with a fresh simulator event trace."""
import json
from pathlib import Path
import sys


def writes(commands):
	commands = commands[commands.index(":W1050000"):]
	commands = commands[:max(i for i, c in enumerate(commands) if c.startswith(":E2")) + 1]
	result = [c for c in commands if c.startswith(":") and c[1] not in "fjq"]
	for axis in "12":
		indexes = [i for i, c in enumerate(result) if c.startswith(":H" + axis)]
		assert len(indexes) in (3, 4), indexes
		values = [int.from_bytes(bytes.fromhex(result[i][3:]), "little") for i in indexes]
		assert all(value == 12000 for value in values[:-2]), values
		assert values[-1] == 24000, values
		assert values[-2] >= 24000, values
		# Only this distance includes live overshoot after the independently detected sensor.
		result[indexes[-2]] = ":H" + axis + "<live-index-backoff>"
	forward = result.index(":G130")
	dec_start = result.index(":J2", result.index(":G230", forward))
	assert set(result[dec_start + 1:dec_start + 3]) == { ":K1", ":K2" }
	# Independent sensor crossings determine which axis is stopped first.
	result[dec_start + 1:dec_start + 3] = sorted(result[dec_start + 1:dec_start + 3])
	return result


reference = json.loads(Path(__file__).with_name("autohome-reference.json").read_text())
traces = Path(sys.argv[1])
for name in ("synscan_mount_autohome_finds_home_index", "synscan_autohome_negative_start", "synscan_autohome_extra_offset"):
	current = [line.split("\t", 1)[1].strip() for line in (traces / (name + "-0.events")).read_text().splitlines()]
	before, after = writes(reference["commands"][name]), writes(current)
	if before != after:
		import difflib
		print("\n".join(difflib.unified_diff(before, after)), file=sys.stderr)
		raise SystemExit("AUTOHOME write compatibility mismatch: " + name)
	print("PASS", name, len(after), "ordered writes; 5-degree offsets and 10-degree returns exact")
