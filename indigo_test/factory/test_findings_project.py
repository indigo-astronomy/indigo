"""Finding lifecycle updates preserve navigator placement and unrelated content."""
from copy import deepcopy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / '.factory/scripts'))
import findings
import findings_project
import task_store


def group(key, path, children):
    return f'\t\t{key} /* {path} */ = {{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n{children}\t\t\t);\n\t\t\tpath = "{path}";\n\t\t\tsourceTree = "<group>";\n\t\t}};\n'


class FindingsProjectTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.project = self.root / 'fixture.xcodeproj/project.pbxproj'
        self.project.parent.mkdir()
        self.active = self.root / findings.ACTIVE_DIR
        self.archive = self.root / findings.ARCHIVE
        self.active.mkdir(parents=True)
        self.archive.mkdir()
        (self.root / findings.REGISTRY).write_text('{"version": 2, "storage": "per-record"}')
        self.ident = 'FND-000001'
        self.record = dict(id=self.ident, title='Synthetic', category='quality', severity='low', status='open', owner='fixture', blocking=False, tasks=[dict(id='001-demo', report='verification.md', revision=None)], summary='Synthetic finding', evidence='Fixture report', resolution=None)
        self.source = self.active / (self.ident + '.json')
        self.source.write_text(json.dumps(self.record))
        report = self.root / '.factory/tasks/001-demo/verification.md'
        report.parent.mkdir(parents=True)
        report.write_text('Synthetic evidence\n')
        self.closed = deepcopy(self.record)
        self.closed.update(status='closed', resolution=dict(kind='fixed', evidence='Synthetic verified fix', approved_by=None, duplicate_of=None))
        self.input = self.root / 'closed.json'
        self.input.write_text(json.dumps(self.closed))
        a, b, c, d = [str(n) * 24 for n in range(1, 5)]
        self.ids = (a, b, c, d)
        self.untouched = '\t\t' + 'A' * 24 + ' /* sentinel */ = {isa = PBXFileReference; path = untouched.c; sourceTree = "<group>"; };\n'
        text = '// !$*UTF8*$!\n{\nobjects = {\n/* Begin PBXFileReference section */\n' + self.untouched + '/* End PBXFileReference section */\n'
        text += group(a, '.factory', f'\t\t\t\t{b},\n')
        text += group(b, 'findings', f'\t\t\t\t{c},\n\t\t\t\t{d},\n')
        text += group(c, 'active', '') + group(d, 'archive', '') + '};\n}\n'
        self.project.write_text(text)
        self.addCleanup(patch.stopall)
        patch.object(findings, 'ROOT', self.root).start()
        # A disposable lock avoids Git access; locking itself is covered by lifecycle tests.
        from contextlib import nullcontext
        patch.object(task_store, 'lifecycle_lock', lambda root: nullcontext()).start()

    def test_reconcile_is_idempotent_and_keeps_unrelated_objects(self):
        self.assertTrue(findings_project.sync(self.root))
        self.assertIn(self.untouched, self.project.read_text())
        self.assertFalse(findings_project.sync(self.root))
        self.assertFalse(findings_project.sync(self.root, check=True))

    def test_check_reports_missing_and_wrong_group_without_writing(self):
        before = self.project.read_bytes()
        with self.assertRaisesRegex(ValueError, 'stale'):
            findings_project.sync(self.root, check=True)
        self.assertEqual(before, self.project.read_bytes())
        findings_project.sync(self.root)
        self.source.rename(self.archive / self.source.name)
        with self.assertRaisesRegex(ValueError, 'stale'):
            findings_project.sync(self.root, check=True)
        self.assertTrue(findings_project.sync(self.root))
        self.assertFalse(findings_project.sync(self.root))

    def test_archive_moves_record_and_preserves_reference_identity(self):
        findings_project.sync(self.root)
        before = findings_project.plan(self.root)[2].decode()
        ref = next(line for line in before.splitlines() if 'isa = PBXFileReference' in line and self.ident in line)
        findings.archive(self.ident, self.input)
        self.assertFalse(self.source.exists())
        self.assertEqual(json.loads((self.archive / self.source.name).read_text()), self.closed)
        self.assertIn(ref, self.project.read_text())
        self.assertFalse(findings_project.sync(self.root, check=True))

    def test_archive_rejects_identity_change_and_collision(self):
        invalid = dict(self.closed, id='FND-000002')
        self.input.write_text(json.dumps(invalid))
        with self.assertRaisesRegex(ValueError, 'identity'):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())
        self.input.write_text(json.dumps(self.closed))
        (self.archive / self.source.name).write_text(json.dumps(self.closed))
        with self.assertRaises(ValueError):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())

    def test_project_failure_restores_record_and_project(self):
        before = self.project.read_bytes()
        original = self.source.read_bytes()
        with patch.object(findings_project, 'sync', side_effect=OSError('synthetic write failure')):
            with self.assertRaises(OSError):
                findings.archive(self.ident, self.input)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertFalse((self.archive / self.source.name).exists())
        self.assertEqual(self.project.read_bytes(), before)

    def test_unsupported_project_fails_before_record_move(self):
        self.project.write_text('Unsupported project')
        with self.assertRaises(ValueError):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())
        self.assertFalse((self.archive / self.source.name).exists())

    def test_symlink_project_is_rejected(self):
        self.project.unlink()
        self.project.symlink_to(self.input)
        with self.assertRaises(ValueError):
            findings_project.sync(self.root)

    def test_no_xcode_repository_can_archive(self):
        self.project.unlink()
        self.project.parent.rmdir()
        findings.archive(self.ident, self.input)
        self.assertFalse(self.source.exists())
        self.assertTrue((self.archive / self.source.name).exists())

    def test_create_transaction_adds_reference_and_rolls_back_on_error(self):
        record = dict(self.record, id='FND-000002')
        destination = self.active / 'FND-000002.json'
        findings.write_record(destination, record)
        self.assertFalse(findings_project.sync(self.root, check=True))
        destination = self.active / 'FND-000003.json'
        with patch.object(findings_project, 'sync', side_effect=OSError('synthetic failure')):
            with self.assertRaises(OSError):
                findings.write_record(destination, dict(record, id='FND-000003'))
        self.assertFalse(destination.exists())

    def test_rejects_risk_acceptance_without_owner_and_unknown_duplicate(self):
        for resolution in (dict(kind='accepted_risk', evidence='Synthetic', approved_by=None, duplicate_of=None), dict(kind='duplicate', evidence='Synthetic', approved_by=None, duplicate_of='FND-000099')):
            self.input.write_text(json.dumps(dict(self.closed, resolution=resolution)))
            with self.assertRaises(ValueError):
                findings.archive(self.ident, self.input)
            self.assertTrue(self.source.exists())

    def test_build_membership_rejected_without_project_mutation(self):
        findings_project.sync(self.root)
        text = self.project.read_text()
        import re
        ref = re.search(r'([A-F0-9]{24}) /\* FND-000001.json \*/ =', text)[1]
        text += f'fileRef = {ref};\n'
        self.project.write_text(text)
        with self.assertRaisesRegex(ValueError, 'build membership'):
            findings_project.sync(self.root)
        self.assertEqual(text, self.project.read_text())

    def test_multiple_projects_fail_before_record_move(self):
        (self.root / 'second.xcodeproj').mkdir()
        with self.assertRaisesRegex(ValueError, 'multiple'):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())

    def test_archive_cli_updates_record_and_project(self):
        import shutil
        import subprocess
        scripts = self.root / '.factory/scripts'
        scripts.mkdir()
        for name in ('findings.py', 'findings_project.py', 'task_store.py'):
            shutil.copy2(ROOT / '.factory/scripts' / name, scripts / name)
        result = subprocess.run([sys.executable, str(scripts / 'findings.py'), 'archive', self.ident, '--record', str(self.input)], cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.source.exists())
        self.assertFalse(findings_project.sync(self.root, check=True))
