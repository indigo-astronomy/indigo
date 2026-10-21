#!/usr/bin/env python3
# Copyright (c) 2026 INDIGO initiative
# All rights reserved.
#
# You may use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Check INDIGO's local Factory boundaries without reading private inventories."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def unique(pairs):
	result = {}
	for key, value in pairs:
		if key in result:
			raise ValueError('duplicate JSON key')
		result[key] = value
	return result


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument('--security', action='store_true')
	args = parser.parse_args()
	try:
		lock = json.loads((ROOT / '.factory/template-lock.json').read_text(), object_pairs_hook=unique)
		for relative in lock['files'].values():
			path = ROOT
			for part in Path(relative).parts:
				if part in ('..', '/'):
					raise ValueError('unsafe mapped path')
				path /= part
				if path.is_symlink():
					raise ValueError('mapped support must not be a symlink')
			if not path.is_file():
				raise ValueError('mapped support file is missing')
		policy = json.loads((ROOT / '.factory/review-policy.json').read_text(), object_pairs_hook=unique)
		if policy['allowed_reviewers'] or policy['required_roles'] or policy['share_paths']:
			raise ValueError('external review requires a separately reviewed owner authorization')
		for relative in ('.factory/adopters.local.json', '.factory/review.local.json', '.factory/telemetry/example.local.json', '.factory/telemetry/example.local.jsonl'):
			tracked = subprocess.check_output(['git', 'ls-files', '--', relative], cwd=ROOT)
			ignored = subprocess.run(['git', 'check-ignore', '-q', '--', relative], cwd=ROOT).returncode == 0
			if tracked or not ignored:
				raise ValueError('private Factory metadata must remain untracked and ignored')
		if not args.security:
			for relative in ('indigo_test/Makefile', 'indigo_drivers/AGENTS.md', 'indigo_test/AGENTS.md', 'indigo_libs/AGENTS.md', 'indigo_docs/AGENTS.md'):
				if not (ROOT / relative).is_file():
					raise ValueError('required project path is missing')
			if (ROOT / 'indigo_drivers/AGENTS.override.md').exists():
				raise ValueError('obsolete driver instruction entry remains')
			files = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT).decode().split('\0')
			if any(Path(name).name == 'REVIEW.md' and (ROOT / name).exists() for name in files if name):
				raise ValueError('legacy REVIEW.md tracking remains')
	except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as exc:
		print('Factory boundary check failed: ' + str(exc), file=sys.stderr)
		return 1
	print('Factory local security boundaries valid.' if args.security else 'Factory project boundaries valid.')
	return 0


if __name__ == '__main__':
	sys.exit(main())
