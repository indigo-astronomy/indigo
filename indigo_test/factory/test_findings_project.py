"""Finding transactions and validation stay independent of Xcode project files."""
from contextlib import redirect_stderr, redirect_stdout
from copy import deepcopy
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / '.factory/scripts'))
import findings
import task_store

TASK = '20261025181246-9a8f-fixture'


class FindingsProjectTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.project = self.root / 'fixture.xcodeproj/project.pbxproj'
        self.project.parent.mkdir()
        self.project.write_bytes(b'Unrelated, unsupported Xcode project\xff\n')
        self.active = self.root / findings.ACTIVE_DIR
        self.archive = self.root / findings.ARCHIVE
        self.active.mkdir(parents=True)
        self.archive.mkdir()
        (self.root / findings.REGISTRY).write_text('{"version": 2, "storage": "per-record"}')
        (self.root / '.factory/state.json').write_text('{"version": 2, "task_state": "per-task"}')
        report = self.root / '.factory/tasks' / TASK / 'verification.md'
        report.parent.mkdir(parents=True)
        report.write_text('Synthetic evidence\n')
        (report.parent / 'state.json').write_text(json.dumps({
            'version': 2, 'active_task': TASK, 'phase': 'verification', 'status': 'active',
            'assigned_role': 'verifier', 'next_action': 'Inspect fixture.', 'blocker': None,
        }))
        self.ident = 'FND-000001'
        self.record = dict(id=self.ident, title='Synthetic', category='quality', severity='low',
                           status='open', owner='fixture', blocking=False,
                           tasks=[dict(id=TASK, report='verification.md', revision=None)],
                           summary='Synthetic finding', evidence='Fixture report', resolution=None)
        self.source = self.active / (self.ident + '.json')
        self.source.write_text(json.dumps(self.record))
        self.closed = deepcopy(self.record)
        self.closed.update(status='closed', resolution=dict(kind='fixed', evidence='Synthetic verified fix',
                                                            approved_by=None, duplicate_of=None))
        self.input = self.root / 'closed.json'
        self.input.write_text(json.dumps(self.closed))
        self.new_input = self.root / 'new.json'
        self.new_input.write_text(json.dumps({k: v for k, v in self.record.items() if k != 'id'}))
        self.git('init', '-q')
        self.git('symbolic-ref', 'HEAD', 'refs/heads/task/' + TASK)
        self.addCleanup(patch.stopall)
        patch.object(findings, 'ROOT', self.root).start()

    def git(self, *args):
        result = subprocess.run(['git', *args], cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def forbid_project_reads(self):
        original = Path.open

        def guarded(path, *args, **kwargs):
            if any(part.endswith('.xcodeproj') for part in path.parts):
                raise AssertionError('finding operation attempted to read an Xcode project')
            return original(path, *args, **kwargs)

        return patch.object(Path, 'open', guarded)

    def run_cli(self, *args):
        scripts = self.root / '.factory/scripts'
        scripts.mkdir(exist_ok=True)
        for name in ('findings.py', 'task_store.py'):
            shutil.copy2(ROOT / '.factory/scripts' / name, scripts / name)
        return subprocess.run([sys.executable, '-B', str(scripts / 'findings.py'), *args],
                              cwd=self.root, capture_output=True, text=True)

    def test_create_does_not_read_or_modify_xcode_project(self):
        before = self.project.read_bytes()
        with self.forbid_project_reads():
            ident = findings.create(TASK, self.new_input)
        self.assertEqual(ident, 'FND-' + TASK + '-001')
        self.assertTrue((self.active / (ident + '.json')).is_file())
        self.assertEqual(self.project.read_bytes(), before)

    def test_archive_does_not_read_or_modify_xcode_project(self):
        before = self.project.read_bytes()
        with self.forbid_project_reads():
            findings.archive(self.ident, self.input)
        self.assertFalse(self.source.exists())
        self.assertEqual(json.loads((self.archive / self.source.name).read_text()), self.closed)
        self.assertEqual(self.project.read_bytes(), before)

    def test_check_does_not_read_or_modify_xcode_project(self):
        before = self.project.read_bytes()
        with self.forbid_project_reads(), patch.object(sys, 'argv', ['findings', 'check']), \
                redirect_stdout(io.StringIO()) as output, redirect_stderr(io.StringIO()) as errors:
            self.assertEqual(findings.main(), 0, errors.getvalue())
        self.assertIn('Findings valid: 1 active, 0 archived', output.getvalue())
        self.assertEqual(self.project.read_bytes(), before)

    def test_multiple_projects_do_not_block_finding_archive(self):
        (self.root / 'second.xcodeproj').mkdir()
        before = self.project.read_bytes()
        findings.archive(self.ident, self.input)
        self.assertFalse(self.source.exists())
        self.assertEqual(self.project.read_bytes(), before)

    def test_symlink_project_is_not_followed(self):
        self.project.unlink()
        self.project.symlink_to(self.input)
        original = self.input.read_bytes()
        with self.forbid_project_reads():
            findings.archive(self.ident, self.input)
        self.assertTrue(self.project.is_symlink())
        self.assertEqual(self.input.read_bytes(), original)

    def test_no_xcode_repository_can_archive(self):
        self.project.unlink()
        self.project.parent.rmdir()
        findings.archive(self.ident, self.input)
        self.assertFalse(self.source.exists())
        self.assertTrue((self.archive / self.source.name).exists())

    def test_archive_rejects_identity_change_and_collision(self):
        self.input.write_text(json.dumps(dict(self.closed, id='FND-000002')))
        with self.assertRaisesRegex(ValueError, 'identity'):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())
        self.input.write_text(json.dumps(self.closed))
        (self.archive / self.source.name).write_text(json.dumps(self.closed))
        with self.assertRaises(ValueError):
            findings.archive(self.ident, self.input)
        self.assertTrue(self.source.exists())

    def test_rejects_risk_acceptance_without_owner_and_unknown_duplicate(self):
        resolutions = (
            dict(kind='accepted_risk', evidence='Synthetic', approved_by=None, duplicate_of=None),
            dict(kind='duplicate', evidence='Synthetic', approved_by=None, duplicate_of='FND-000099'),
        )
        for resolution in resolutions:
            with self.subTest(resolution=resolution['kind']):
                self.input.write_text(json.dumps(dict(self.closed, resolution=resolution)))
                with self.assertRaises(ValueError):
                    findings.archive(self.ident, self.input)
                self.assertTrue(self.source.exists())

    def test_create_write_failure_leaves_no_record(self):
        destination = self.active / 'FND-000002.json'
        original = self.source.read_bytes()
        with patch.object(task_store, 'atomic', side_effect=OSError('Synthetic record write failure')):
            with self.assertRaises(OSError):
                findings.write_record(destination, dict(self.record, id='FND-000002'))
        self.assertFalse(destination.exists())
        self.assertEqual(self.source.read_bytes(), original)

    def test_create_failure_after_atomic_replace_removes_new_record(self):
        destination = self.active / 'FND-000002.json'
        atomic = task_store.atomic

        def failed(path, raw):
            atomic(path, raw)
            raise OSError('Synthetic error after replacement')

        with patch.object(task_store, 'atomic', failed):
            with self.assertRaises(OSError):
                findings.write_record(destination, dict(self.record, id='FND-000002'))
        self.assertFalse(destination.exists())

    def test_archive_failure_before_source_unlink_preserves_record(self):
        original = self.source.read_bytes()
        unlink = Path.unlink

        def failed(path, *args, **kwargs):
            if path == self.source:
                raise OSError('Synthetic source unlink failure')
            return unlink(path, *args, **kwargs)

        with patch.object(Path, 'unlink', failed):
            with self.assertRaises(OSError):
                findings.archive(self.ident, self.input)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertFalse((self.archive / self.source.name).exists())

    def test_archive_failure_after_source_unlink_restores_record(self):
        original = self.source.read_bytes()
        unlink = Path.unlink

        def failed(path, *args, **kwargs):
            result = unlink(path, *args, **kwargs)
            if path == self.source:
                raise OSError('Synthetic error after source unlink')
            return result

        with patch.object(Path, 'unlink', failed):
            with self.assertRaises(OSError):
                findings.archive(self.ident, self.input)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertFalse((self.archive / self.source.name).exists())

    def test_create_rejects_destination_collision_without_overwriting(self):
        destination = self.active / 'FND-000002.json'
        destination.write_bytes(b'Existing destination\n')
        with self.assertRaisesRegex(ValueError, 'destination exists'):
            findings.write_record(destination, dict(self.record, id='FND-000002'))
        self.assertEqual(destination.read_bytes(), b'Existing destination\n')

    def test_cli_create_archive_and_check_do_not_need_xcode_helper(self):
        before = self.project.read_bytes()
        created = self.run_cli('create', '--task', TASK, '--record', str(self.new_input))
        self.assertEqual(created.returncode, 0, created.stderr)
        archived = self.run_cli('archive', self.ident, '--record', str(self.input))
        self.assertEqual(archived.returncode, 0, archived.stderr)
        checked = self.run_cli('check')
        self.assertEqual(checked.returncode, 0, checked.stderr)
        self.assertIn('1 active, 1 archived', checked.stdout)
        self.assertFalse((self.root / '.factory/scripts/findings_project.py').exists())
        self.assertEqual(self.project.read_bytes(), before)

    def test_cli_retired_sync_project_is_rejected_without_mutation(self):
        before = self.project.read_bytes()
        result = self.run_cli('sync-project')
        self.assertEqual(result.returncode, 2)
        self.assertIn('invalid choice', result.stderr)
        self.assertEqual(self.project.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
