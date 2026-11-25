"""Ticket flow: Python performs mechanical operations only.

Nothing here stores a worker's claim. A check or review belongs to the Git
tree it was recorded for; a new tree makes it stale and it is simply repeated.
Every command can be re-run: each step looks at the actual Git and file state
first and continues from there.
"""

import copy
from datetime import datetime, timezone
import fnmatch
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from uuid import uuid4

from errors import FactoryError
import factory_platform as platform_ops
import tickets as core


def fail(code, message):
    raise FactoryError(code, message)


def git(cwd, *args, check=True, text=True):
    result = core.run_git(cwd, *args, text=text)
    if check and result.returncode:
        output = result.stderr or result.stdout
        fail("GIT_FAILED", "git " + " ".join(args) + " failed in " + str(cwd) + ":\n"
             + (output if text else os.fsdecode(output)).strip())
    return result if not check else (result.stdout.strip() if text else result.stdout)


def python():
    """The running interpreter as one word of a shell command on this platform."""
    return subprocess.list2cmdline([sys.executable]) if os.name == "nt" else shlex.quote(sys.executable)


def run_check(command, cwd, limit):
    """Run a check command through the platform's shell; (output, exit code, timed out).

    {python} in the command stands for the running interpreter. The command runs supervised: when
    it ends or its time is up, every process it started in its process group (POSIX) or Job
    (Windows) is stopped before this returns.
    """
    command = command.replace("{python}", python())
    if os.name == "nt":
        # The command line that subprocess builds for shell=True.
        argv = (os.environ.get("ComSpec") or "cmd.exe") + ' /c "' + command + '"'
    else:
        argv = ["/bin/sh", "-c", command]
    with tempfile.TemporaryFile("w+", errors="replace") as out, tempfile.TemporaryFile("w+", errors="replace") as err, \
            open(os.devnull) as nothing:
        try:
            code, timed_out = platform_ops.run_supervised(argv, cwd=str(cwd), timeout=limit, stdin=nothing,
                                                          stdout=out, stderr=err)
        except platform_ops.CleanupUnconfirmed:
            fail("CHECK_PROCESSES_LEFT", "The check command's processes could not be confirmed stopped; nothing was "
                                         "recorded. Inspect and stop leftover processes, then run 'flow check' again.")
        out.seek(0)
        err.seek(0)
        return out.read() + err.read(), code, timed_out


def test_counts(output):
    """Best-effort unittest/pytest counts; None when the output has neither."""
    ran = re.findall(r"^Ran (\d+) tests? in ", output, re.M)
    if ran:
        executed = sum(map(int, ran))
        failed = sum(int(n) for part in re.findall(r"^FAILED \((.*)\)", output, re.M)
                     for n in re.findall(r"(?:failures|errors|unexpected successes)=(\d+)", part))
        skipped = sum(int(n) for part in re.findall(r"^(?:OK|FAILED) \((.*)\)", output, re.M)
                      for n in re.findall(r"skipped=(\d+)", part))
        # Fixture and subtest skips do not increment testsRun. Verbose output
        # identifies them; older/terse output can be ambiguous.
        nested_skips = len(re.findall(
            r"^(?:(?:setUpClass|setUpModule) \(.*\) \.\.\. |  .* \.\.\. )?skipped ", output, re.M))
        return {"executed": max(0, executed - skipped + nested_skips), "failed": failed, "skipped": skipped}
    summary = re.findall(r"^=+ (.*) in [\d.]+s.*=+$", output, re.M)
    if summary:
        count = lambda word: sum(int(n) for n in re.findall(r"(\d+) " + word, summary[-1]))
        failed, skipped = count("failed") + count("error"), count("skipped")
        return {"executed": count("passed") + failed, "failed": failed, "skipped": skipped}
    return None


def check_outcome(name, code, counts, passed, timed_out=None):
    """One short journal line for a finished check, with its test counts when known."""
    if timed_out:
        return "Check " + name + " timed out after " + str(timed_out) + " s"
    if passed:
        return "Check " + name + " passed" + (": " + str(counts["executed"]) + " tests" if counts else "")
    if counts and counts["executed"] == 0:
        return "Check " + name + " failed: no tests executed"
    if counts and counts["failed"]:
        return ("Check " + name + " failed: " + str(counts["failed"]) + " of " + str(counts["executed"])
                + " tests failed")
    return "Check " + name + " failed: exit code " + str(code)


# What 'close' requires per profile; config.json may add or replace profiles under "flows".
DEFAULT_FLOWS = {"simple": {"approvals": [], "reviews": ["code"]},
                 "full": {"approvals": ["design"], "reviews": ["code"]},
                 "full-ui": {"approvals": ["design", "mockup"], "reviews": ["code"]}}
STAGES = ("analysis", "design", "mockup")


def review_title(kind):
    return "Review" if kind == "code" else "Review (" + kind + ")"


def flow_state(document):
    """The ticket's flow records with every list present; missing ones read as empty."""
    return {"profile": None, "base": None, "checks": [], "reviews": [], "approvals": [], "notes": [],
            "questions": [], "presented": [], **(document.get("flow") or {})}


class Flow:
    def __init__(self, project, tickets=None):
        if tickets is not None:
            # Reader for 'ticket list' and 'ticket show': files only, Git is never called.
            self.primary, self.tickets, self.root = tickets.project, tickets, tickets.root
            self.unmoved, self.moved, self.verified = {}, [], set()
            return
        project = Path(project).resolve()
        top = git(project, "rev-parse", "--show-toplevel", check=False)
        if top.returncode:
            fail("GIT_REQUIRED", "The ticket flow needs a Git repository.")
        # The first listed worktree is the primary checkout; ticket state lives there.
        listed = git(project, "worktree", "list", "--porcelain")
        self.primary = Path(listed.splitlines()[0].split(" ", 1)[1])
        self.tickets = core.Tickets(self.primary)
        self.root = self.tickets.root
        # Worktrees left beside the project because Git could not move them: {path: reason}.
        self.unmoved, self.moved, self.verified = {}, [], set()
        if not self.root.is_dir():
            fail("NOT_INITIALIZED", "No .factory2 directory in " + str(self.primary) + ".")

    def load(self, ticket_id):
        _, document = self.tickets.load(ticket_id)
        document["flow"] = flow_state(document)
        return document

    def profile(self, document):
        """The ticket's profile name and what it requires before closing."""
        profiles = {**DEFAULT_FLOWS, **self.config(document).get("flows", {})}
        name = document["flow"]["profile"] or "simple"
        if name not in profiles:
            fail("UNKNOWN_FLOW", "Unknown flow profile '" + name + "'. Available: " + ", ".join(sorted(profiles)) + ".")
        return name, {"approvals": list(profiles[name].get("approvals", [])),
                      "reviews": list(profiles[name].get("reviews", []))}

    def parent(self, document):
        """The started parent ticket whose branch collects this sub-ticket, if any."""
        related = document.get("related_ticket_id")
        if not related:
            return None
        try:
            parent = self.tickets.load(related)[1]
        except FactoryError:
            return None
        if parent["status"] == "in_progress" and parent.get("branch") and self.branch_exists(parent["branch"]):
            return parent
        return None

    def children(self, document):
        return [d for d in self.tickets.documents()
                if d.get("related_ticket_id") == document["ticket_id"] and d.get("status") != "closed"]

    def save(self, document, name, actor, details=None):
        """One history event per command; tickets.py derives the journal lines."""
        with self.tickets.lock():
            before = self.current(document)
            core.event(document, name, actor, **(details or {}))
            self.tickets.save(before, document, actor, "flow/" + name.split("/")[-1])

    def current(self, document):
        """Under the lock: the stored ticket, after the document adopted progress lines written meanwhile."""
        before = self.tickets.load(document["ticket_id"])[1]
        if before["revision"] != document["revision"]:
            # Progress lines change nothing else: adopt them and continue. Anything else is a conflict.
            known = {entry["event_id"] for entry in document["history"]}
            added = [entry for entry in before["history"] if entry["event_id"] not in known]
            at = sum(entry["event_id"] in known for entry in before["history"])
            if (not added or any(entry["event"] != "task/progress" for entry in added)
                    or before["history"][:at] != document["history"][:at]):
                fail("REVISION_CONFLICT", "The ticket changed meanwhile; repeat the command.")
            document["history"][at:at] = added
            # A progress line that declared a model also set it on its attempt; keep that too.
            for entry in (entry for entry in added if entry.get("worker")):
                for attempt in (a for task in document["tasks"] for a in task["attempts"]):
                    if attempt["attempt_id"] == entry["attempt_id"]:
                        attempt["worker"] = entry["worker"]
            document.update(revision=before["revision"], journal=before["journal"])
        return before

    def amend(self, ticket_id, change, actor, operation):
        """Apply change(document) to the ticket as it is now and save it; returns what change returned.

        The ticket is read under the lock, so nothing read earlier is written back and a
        command running at the same moment cannot make this one fail or lose its addition.
        """
        for attempt in range(3):
            try:
                with self.tickets.lock():
                    before = self.tickets.load(ticket_id)[1]
                    document = copy.deepcopy(before)
                    document["flow"] = flow_state(document)
                    result = change(document)
                    self.tickets.save(before, document, actor, operation)
                    return result
            except FactoryError as error:
                if error.code != "BUSY" or attempt == 2:
                    raise

    @staticmethod
    def running_task(document):
        """The running task whose attempt started last, or None."""
        running = [task for task in document["tasks"] if task["status"] == "running" and task["attempts"]]
        return max(running, key=lambda task: task["attempts"][-1]["started_at"], default=None)

    @staticmethod
    def progress(document, text, task, actor, worker=None):
        """Add one progress line for a task (or the ticket) to the document; the caller saves it.

        With worker ({"provider", "model", "reasoning"}) the line also records, on the task's
        latest attempt, the model that runs it, replacing an earlier declaration.
        """
        line = core.journal_output({"text": " ".join(text.split())})["text"]
        details = {}
        if worker:
            attempt = task["attempts"][-1]
            attempt["worker"] = worker
            details = {"attempt_id": attempt["attempt_id"], "worker": worker}
        core.event(document, "task/progress", actor, task_id=task["task_id"] if task else None, text=line, **details)

    def log(self, ticket_id, text=None, task=None, actor="human/owner", model=None, provider=None, reasoning=None):
        """Record one progress line in a task's journal and, with model, the model on its latest attempt."""
        values = {"provider": provider, "model": model, "reasoning": reasoning}
        for name, value in values.items():
            if value is not None and (not value.strip() or len(value) > 100 or len(value.split()) > 1):
                fail("INVALID_ARGUMENT", "--" + name + " must be one word of at most 100 characters.")
        if model is None and (provider or reasoning):
            fail("INVALID_ARGUMENT", "--provider and --reasoning need --model.")
        worker = {name: value and value.strip() for name, value in values.items()} if model else None
        if not (text or "").strip():
            if not worker:
                fail("INVALID_ARGUMENT", "The progress line needs --text.")
            text = ("Running on " + worker["model"] + (" via " + worker["provider"] if worker["provider"] else "")
                    + (", reasoning " + worker["reasoning"] if worker["reasoning"] else ""))

        def add(document):
            if document["status"] == "closed":
                fail("CLOSED", "The ticket is closed.")
            target = self.running_task(document)
            if task:
                target = next((t for t in document["tasks"] if task.lower() in (t["task_id"], t["title"].lower())),
                              None)
                if target is None:
                    fail("INVALID_ARGUMENT", "No task '" + task + "' in this ticket. Tasks: "
                         + (", ".join(t["title"] for t in document["tasks"]) or "none") + ".")
            if worker and not (target and target["attempts"]):
                fail("INVALID_ARGUMENT", "The model is recorded on a task's attempt: "
                     + ("task '" + target["title"] + "' has not started." if target
                        else "no task is running; name one with --task."))
            self.progress(document, text, target, actor, worker)
            return dict({"ticket_id": ticket_id, "task_id": target["task_id"] if target else None,
                         "task": target["title"] if target else None, "text": document["history"][-1]["text"]},
                        **({"worker": worker} if worker else {}))
        return self.amend(ticket_id, add, actor, "flow/log")

    def config(self, document=None):
        """Effective configuration: project config.json, Local layer, ticket overrides."""
        import config
        layers = [config.read_json(path) if path.is_file() else {}
                  for path in (self.root / "config.json", self.root / "local" / "config.json")]
        layers.append((document or {}).get("config_overrides", {}))
        effective = config.resolve(*layers)
        effective["local_rules"] = layers[1].get("rules", {})
        return effective

    def rules(self, document):
        """Effective rule text for the files this ticket changes, composed by rules.py."""
        import rules
        started = self.worktree(document).is_dir()
        project = self.worktree(document) if started else self.primary
        if not (project / ".factory2/rules.json").is_file():
            return []
        files = [name for name in (self.changed(document) if started else [])
                 if not any(c in name for c in "\\*?[]\x00\n\r") and not (project / name).is_dir()]
        effective = self.config(document)
        projection = rules.compose(project, files, document.get("config_overrides", {}).get("rules", {}),
                                   local_project=self.primary, local_rules=effective["local_rules"])
        lines = ["", "Rules (effective for this ticket):"]
        lines += ["- " + name + ": " + text for name, text in sorted(projection["rules"].items())]
        seen = set()
        for name, scoped in projection["rules_by_path"].items():
            directory = name.rsplit("/", 1)[0] if "/" in name else "."
            different = {key: text for key, text in scoped.items() if projection["rules"].get(key) != text}
            if different and directory not in seen:
                seen.add(directory)
                lines += ["", "Rules that differ for files in " + directory + "/:"]
                lines += ["- " + key + ": " + text for key, text in sorted(different.items())]
        return lines

    # ---- git ---------------------------------------------------------------

    def worktree(self, document):
        """The ticket's checkout: .factory2/local/worktrees/ID in the primary checkout, ignored by Git.

        A worktree that an earlier version created beside the project in PROJECT.worktrees/ID is
        moved here first; when Git cannot move it, it is used where it is and the result says so.
        """
        tree, old = core.worktree_locations(self.primary, document["ticket_id"])
        if not tree.is_dir() and old.is_dir() and str(old) not in self.unmoved:
            tree.parent.mkdir(parents=True, exist_ok=True)
            moved = git(self.primary, "worktree", "move", str(old), str(tree), check=False)
            if not moved.returncode:
                self.moved.append({"from": str(old), "to": str(tree)})
            if tree.is_dir():
                # Moved now, or by a command running at the same moment.
                self.drop_old_worktrees()
            else:
                self.unmoved[str(old)] = (moved.stderr or moved.stdout).strip()
        if not tree.is_dir() and old.is_dir():
            tree = old
        if tree.is_dir() and tree not in self.verified:
            self.own_worktree(tree, "ticket/" + document["ticket_id"])
            self.verified.add(tree)
        return tree

    def own_worktree(self, tree, branch):
        """Refuse a directory that is not the Git worktree of the ticket branch.

        The earlier flow kept its checkouts, on branch factory/ID, in the same place; the upgrade keeps them.
        """
        found = git(tree, "rev-parse", "--show-toplevel", "--abbrev-ref", "HEAD", check=False).stdout.splitlines()
        if len(found) == 2 and Path(found[0]).resolve() == tree.resolve():
            if found[1] == branch:
                return
            actual = "a Git worktree of " + ("branch " + found[1] if found[1] != "HEAD" else "a detached commit")
        else:
            actual = "not a Git worktree"
        fail("WORKTREE_CONFLICT", "The directory " + str(tree) + " is " + actual + ", not the worktree of branch "
             + branch + ". It may be a checkout kept from an earlier Factory version. The directory was left "
             "untouched and this command recorded nothing. Finish its work with Git (commit and merge what is needed), remove it with 'git worktree remove' "
             "or, when it is a plain directory, delete or rename it, then repeat the command.")

    def own_tree(self, document):
        """The ticket's worktree when it exists; None when there is none or the directory there is something else."""
        try:
            tree = self.worktree(document)
        except FactoryError as error:
            if error.code != "WORKTREE_CONFLICT":
                raise
            return None
        return tree if tree.is_dir() else None

    def drop_old_worktrees(self):
        """Remove PROJECT.worktrees once the last worktree left it; a directory still in use stays."""
        try:
            (self.primary.parent / (self.primary.name + ".worktrees")).rmdir()
        except OSError:
            pass

    def branch_exists(self, branch):
        return not git(self.primary, "show-ref", "--verify", "--quiet", "refs/heads/" + branch, check=False).returncode

    def snapshot(self, document, check_whitespace=False):
        """Commit pending work on the ticket branch and return (commit, tree)."""
        tree = self.worktree(document)
        if not tree.is_dir():
            fail("NOT_STARTED", "Run 'flow start " + document["ticket_id"] + "' first.")
        if check_whitespace:
            # Check both views before snapshot stages everything: a working-tree fix must not hide an index error.
            whitespace = []
            for options in ((), ("--cached",)):
                result = git(tree, "diff", "--check", *options, check=False)
                if result.returncode not in (0, 2):
                    fail("GIT_FAILED", "git diff --check failed in " + str(tree) + ":\n"
                         + (result.stderr or result.stdout).strip())
                if result.returncode:
                    whitespace.append(("Staged" if options else "Unstaged") + ":\n"
                                      + (result.stderr or result.stdout).strip())
            if whitespace:
                fail("WHITESPACE_ERROR", "\n".join(whitespace) + "\nFix both diffs and run 'flow check' again.")
        if git(tree, "status", "--porcelain"):
            git(tree, "add", "-A")
            git(tree, "commit", "-q", "--no-verify", "-m", "wip: " + document["title"])
        return git(tree, "rev-parse", "HEAD"), git(tree, "rev-parse", "HEAD^{tree}")

    def changed(self, document):
        tree = self.worktree(document)
        base = git(tree, "merge-base", document["flow"]["base"], "HEAD")
        # Binary NUL-delimited output preserves Git paths without display quoting,
        # whitespace stripping or universal-newline conversion.
        names = git(tree, "diff", "--name-only", "-z", base, "HEAD", text=False)
        return [os.fsdecode(name) for name in names.split(b"\0") if name]

    # ---- commands ----------------------------------------------------------

    def new(self, title, description="", kind="change", priority=None, severity=None, actor="human/owner",
            overrides=None, profile=None):
        arguments = {"title": title, "description": description, "type": kind, "config_overrides": overrides or {}}
        arguments.update({k: v for k, v in (("priority", priority), ("severity", severity), ("profile", profile)) if v})
        created = self.tickets.write("ticket/create", arguments, actor)
        return {**self.summary(self.load(created["ticket_id"])),
                **{key: created[key] for key in ("commit_error",) if key in created}}

    def start(self, ticket_id, actor="human/owner", provider=None):
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed; create a new one for further work.")
        branch = "ticket/" + ticket_id
        tree = self.worktree(document)
        parent = self.parent(document)
        base = (document["flow"]["base"] or (parent["branch"] if parent else None)
                or self.config(document)["git"].get("target_branch")
                or git(self.primary, "rev-parse", "--abbrev-ref", "HEAD"))
        if not tree.is_dir():
            tree.parent.mkdir(parents=True, exist_ok=True)
            git(self.primary, "worktree", "prune")
            if self.branch_exists(branch):
                git(self.primary, "worktree", "add", str(tree), branch)
            else:
                git(self.primary, "worktree", "add", "-b", branch, str(tree), base)
        if document["status"] != "in_progress" or document.get("branch") != branch:
            document.update(status="in_progress", branch=branch)
            document["flow"]["base"] = base
            implement = self.worker_task(document, "Implement", "implementation")
            if implement["status"] != "running":
                core.start_task(implement, actor, "Ticket branch " + branch)
                self.progress(document, "Implementation started on branch " + branch, implement, actor)
            self.save(document, "ticket/started", actor, {"branch": branch, "base": base})
        brief = self.brief(ticket_id, "implement", provider=provider)
        return {**self.summary(document), "brief": brief["brief"], "recommendation": brief["recommendation"]}

    def selected_checks(self, document, commands, names=None):
        if commands and names:
            fail("INVALID_ARGUMENT", "--check and --command cannot be combined: name configured checks or give "
                                     "shell commands, not both.")
        if names:
            configured = {c["name"]: c for c in self.config(document).get("checks", [])}
            unknown = [name for name in names if name not in configured]
            if unknown:
                fail("UNKNOWN_CHECK", "No configured check named " + ", ".join(unknown) + ". Configured checks: "
                     + (", ".join(configured) or "none") + ".")
            return [configured[name] for name in dict.fromkeys(names)]
        if commands:
            return [{"name": "command-" + str(index + 1), "command": command}
                    for index, command in enumerate(commands)]
        files = self.changed(document)
        return [check for check in self.config(document).get("checks", [])
                if not check.get("paths") or any(fnmatch.fnmatch(name, pattern)
                                                 for name in files for pattern in check["paths"])]

    def check(self, ticket_id, commands=None, timeout=None, actor="human/owner", names=None):
        document = self.load(ticket_id)
        if names:
            self.selected_checks(document, commands, names)  # A refusal happens before anything is committed.
        commit, tree_hash = self.snapshot(document, check_whitespace=True)
        checks = self.selected_checks(document, commands, names)
        # The checks a plain run would choose for this tree; stored so that close needs no Git to judge them.
        selected = None if commands else [c["name"] for c in self.selected_checks(document, None)]
        if not checks:
            fail("NO_CHECKS", "No check in the effective configuration matches the changed files. Pass the "
                              "verification command with --command, or add a check to .factory2/config.json.")
        # Logs are temporary files; .factory2/local holds Local settings, Factory state and ticket worktrees.
        logs = Path(tempfile.gettempdir()) / "factory-flow" / ticket_id
        logs.mkdir(parents=True, exist_ok=True)
        results = []
        for check in checks:
            started = time.monotonic()
            limit = timeout or check.get("timeout_seconds") or 900
            # Written before the check runs, so that a long check is visible while it runs.
            self.log(ticket_id, "Check " + check["name"] + " started"
                     + (": " + check["command"][:150] if commands else ""), actor=actor)
            timed_out = None
            output, code, expired = run_check(check["command"], self.worktree(document), limit)
            if expired:
                output += "\nTimed out after " + str(limit) + " s."
                code, timed_out = 124, limit
            log = logs / (tree_hash[:12] + "-" + re.sub(r"[^A-Za-z0-9_.-]", "_", check["name"]) + ".log")
            log.write_text(output, encoding="utf-8")
            counts = test_counts(output)
            passed = code == 0 and (counts is None or (counts["executed"] > 0 and counts["failed"] == 0))
            results.append({"name": check["name"], "command": check["command"], "commit": commit, "tree": tree_hash,
                            "exit_code": code, "counts": counts, "passed": passed, "at": core.now(),
                            "duration_s": round(time.monotonic() - started, 1), "log": str(log),
                            **({"selected": selected} if selected is not None else {})})
            self.log(ticket_id, check_outcome(check["name"], code, counts, passed, timed_out), actor=actor)
        names = {result["name"] for result in results}

        def record(current):
            # The ticket may have changed while the checks ran: the results are added to its current state.
            # A repeated check replaces the earlier result of the same name for the same tree.
            current["flow"]["checks"] = [c for c in current["flow"]["checks"]
                                         if not (c["tree"] == tree_hash and c["name"] in names)] + results
            core.event(current, "check/passed" if all(r["passed"] for r in results) else "check/failed", actor,
                       commit=commit, checks=", ".join(r["name"] + (" passed" if r["passed"] else " FAILED")
                                                       for r in results))
        self.amend(ticket_id, record, actor, "flow/" + ("passed" if all(r["passed"] for r in results) else "failed"))
        response = {"ticket_id": ticket_id, "commit": commit, "tree": tree_hash,
                    "passed": all(r["passed"] for r in results), "results": results}
        for result in results:
            if not result["passed"]:
                tail = Path(result["log"]).read_text(encoding="utf-8")[-3000:]
                result["output_tail"] = tail
                if result["counts"] and result["counts"]["executed"] == 0:
                    result["hint"] = "Zero tests executed is not a pass; fix the test selection."
        if not response["passed"]:
            response["next"] = "Fix the failures in the worktree and run 'flow check' again."
        return response

    def review(self, ticket_id, verdict, notes="", reviewer=None, actor="human/owner", kind="code"):
        document = self.load(ticket_id)
        if verdict not in ("pass", "changes"):
            fail("INVALID_ARGUMENT", "Verdict must be pass or changes.")
        commit, tree_hash = self.snapshot(document)
        document["flow"]["reviews"].append({"kind": kind, "verdict": verdict, "notes": notes,
                                            "reviewer": reviewer or actor, "commit": commit, "tree": tree_hash,
                                            "at": core.now()})
        # Without a recorded handoff the review attempt starts now and has no duration.
        task = self.handoff(document, reviewer or actor, kind)
        core.stop_task(task, "succeeded" if verdict == "pass" else "failed", notes[:2000] or verdict, reviewer or actor)
        task["result"] = {"summary": notes[:2000] or verdict, "source": "review", "recorded_by": reviewer or actor}
        self.progress(document, "Review finished: " + ("passed" if verdict == "pass" else "changes requested"),
                      task, reviewer or actor)
        if verdict == "changes":
            # The implementer's next attempt runs from the verdict until the next handoff.
            implement = self.worker_task(document, "Implement", "implementation")
            if implement["status"] != "running":
                core.start_task(implement, actor, "Changes requested by review")
                self.progress(document, "Implementation resumed: the review requested changes", implement, actor)
        self.save(document, "review/" + ("passed" if verdict == "pass" else "changes_requested"), actor,
                  {"task_id": task["task_id"], "kind": kind, "verdict": verdict, "commit": commit})
        response = {"ticket_id": ticket_id, "kind": kind, "verdict": verdict, "commit": commit, "tree": tree_hash}
        if verdict == "changes":
            rounds = sum(r["verdict"] == "changes" and r.get("kind", "code") == kind
                         for r in document["flow"]["reviews"])
            response["next"] = ("Two review rounds have asked for changes; consult the owner before continuing."
                                if rounds >= 2 else
                                "Return the notes to the implementer, then run 'flow check' and review again.")
        return response

    @staticmethod
    def worker_task(document, title, kind):
        """The ticket's single Implement or Review task; each round is one attempt."""
        task = next((t for t in document["tasks"] if t["title"] == title and t["type"] == kind), None)
        if task is None:
            task = core.new_task(title, kind)
            document["tasks"].append(task)
        task["difficulty"] = document.get("difficulty", "unknown")
        return task

    def handoff(self, document, actor, kind="code"):
        """End the implementer's running attempt and start the reviewer's; returns the Review task."""
        implement = self.worker_task(document, "Implement", "implementation")
        if implement["status"] == "running":
            core.stop_task(implement, "succeeded", "Handed to review", actor)
        review = self.worker_task(document, review_title(kind), "security_review" if kind == "security" else "review")
        if review["status"] != "running":
            core.start_task(review, actor, "Review started")
        return review

    def gates(self, document, tree_hash):
        """Recomputed from recorded facts for one tree; nothing is cached."""
        ticket_id = document["ticket_id"]
        _, required = self.profile(document)
        missing = []
        directory = self.tickets.load(ticket_id)[0]
        for stage in required["approvals"]:
            records = [a for a in document["flow"]["approvals"] if a["stage"] == stage]
            if not records:
                missing.append("The owner has not approved the " + stage + ": prepare it with 'flow stage " + ticket_id
                               + " --stage " + stage + "' and record 'flow approve'.")
            elif not records[-1]["approved"]:
                missing.append("The owner returned the " + stage + " with comments: revise it and ask again.")
            else:
                path = directory / records[-1]["file"]
                if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != records[-1]["sha256"]:
                    missing.append("The " + stage + " file changed after approval: the owner approves it again "
                                   "with 'flow approve'.")
        if tree_hash is None:
            missing.append("No branch yet: run 'flow start " + ticket_id + "'.")
            return missing
        checks = [c for c in document["flow"]["checks"] if c["tree"] == tree_hash]
        if not checks:
            missing.append("No check ran for the current tree: run 'flow check " + ticket_id + "'.")
        elif not all(c["passed"] for c in checks):
            missing.append("A check failed for the current tree: fix it and run 'flow check' again.")
        else:
            # A configured-check result stores the checks that plain 'flow check' chose for its tree, so a
            # named run cannot stand in for the rest. Results without it (--command, older ones) need nothing more.
            passed = {c["name"] for c in checks}
            absent = sorted({name for c in checks for name in c.get("selected", [])} - passed)
            if absent:
                missing.append(("Checks " + ", ".join(absent) + " have" if len(absent) > 1
                                else "Check " + absent[0] + " has") + " not passed for the current tree: run "
                               "'flow check " + ticket_id + " --check NAME' for each, or plain 'flow check'.")
        for kind in required["reviews"]:
            reviews = [r for r in document["flow"]["reviews"]
                       if r["tree"] == tree_hash and r.get("kind", "code") == kind]
            option = "" if kind == "code" else " --kind " + kind
            if not reviews:
                missing.append("No " + kind + " review exists for the current tree: have an independent reviewer "
                               "inspect it and record 'flow review " + ticket_id + option + " --verdict pass|changes'.")
            elif reviews[-1]["verdict"] != "pass":
                missing.append("The latest " + kind + " review asked for changes: address them, then check and "
                               "review again.")
        for child in self.children(document):
            missing.append("Sub-ticket " + child["ticket_id"] + " (" + child["title"] + ") is not closed.")
        return missing

    def pending(self, document):
        """What the owner is asked to do now, as [{"kind", "text"}].

        Recomputed on every read from the ticket's recorded facts and the effective
        configuration, without Git. Nothing is stored and no command is refused because
        of it: an item disappears when the owner's action is recorded or the facts change.
        """
        status, ticket_id = document.get("status"), document.get("ticket_id")
        actions = []
        if status == "closed":
            return actions
        document = dict(document, flow=flow_state(document))
        state = document["flow"]

        def ask(kind, text):
            actions.append({"kind": kind, "text": text})

        proposal = document.get("proposal")
        if proposal is not None and proposal.get("confirmed_at") is None:
            # A ticket without a proposal record predates proposals and counts as confirmed.
            metadata = core.effective_metadata(document)
            ask("ticket/confirm", "Confirm or correct the proposed priority " + metadata["priority"] + ", severity "
                                  + metadata["severity"] + " and flow profile " + (state["profile"] or "simple")
                                  + " (recorded with 'ticket confirm " + ticket_id
                                  + "', adding --priority, --severity or --profile for corrections).")
        for question in state["questions"]:
            if question.get("answer") is None:
                ask("flow/answer", "Answer the question: " + question["text"] + " (recorded with 'flow answer "
                                   + ticket_id + " --text ...').")
        for stage in STAGES:
            shown = [p for p in state["presented"] if p["stage"] == stage]
            decided = [a for a in state["approvals"] if a["stage"] == stage]
            if shown and not (decided and decided[-1]["at"] > shown[-1]["at"]):
                ask("flow/approve", "Approve or return the " + stage + " in " + shown[-1]["file"]
                                    + " (recorded with 'flow approve " + ticket_id + " --stage " + stage + " --file "
                                    + shown[-1]["file"] + "', or with '--reject --text ...').")
        # Work that continued, or the owner's recorded answer, ends a consultation about review rounds.
        later = [c["at"] for c in state["checks"]] + [q["answered_at"] for q in state["questions"]
                                                      if q.get("answered_at")]
        for kind in dict.fromkeys(r.get("kind", "code") for r in state["reviews"]):
            rounds = [r for r in state["reviews"] if r.get("kind", "code") == kind]
            if (rounds[-1]["verdict"] == "changes" and sum(r["verdict"] == "changes" for r in rounds) >= 2
                    and not any(at > rounds[-1]["at"] for at in later)):
                ask("flow/consult", "Two " + kind + " review rounds asked for changes: decide how to continue "
                                    "(recorded with 'flow answer " + ticket_id + " --text ...').")
        if status == "in_progress" and state["checks"]:
            try:
                options = self.consent(document)
                if options:
                    # The tree of the latest record; edits not yet recorded by a check are not seen here.
                    latest = max(state["checks"] + state["reviews"], key=lambda r: r["at"])
                    # A sub-ticket merged since then changed the tree: check and review come first.
                    if (state.get("sub_ticket_merged_at") or "") < latest["at"] and not self.gates(
                            document, latest["tree"]):
                        # One action and one command, also when both consents are needed.
                        commit, merge = " --commit" in options, " --merge" in options
                        ask("flow/commit" if commit else "flow/merge",
                            "Checks and reviews pass and " + ("committing" if commit else "merging")
                            + " is not automatic: decide whether to " + " and ".join(
                                ["commit"] * commit + ["merge"] * merge)
                            + "; on your instruction the coordinator runs 'flow close " + ticket_id + options + "'.")
            except FactoryError:
                pass  # An unreadable configuration or profile is reported by the flow commands themselves.
        return actions

    def consent(self, document):
        """The 'flow close' options for the owner's consent that the effective configuration does not give."""
        settings = self.config(document)["git"]
        return (("" if settings["auto_commit"] else " --commit")
                + ("" if settings["auto_merge_and_cleanup"] else " --merge"))

    def ask(self, ticket_id, text, actor="human/owner"):
        """Record a question the coordinator put to the owner about this ticket."""
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed.")
        if not text.strip():
            fail("INVALID_ARGUMENT", "The question needs --text.")
        document["flow"]["questions"].append({"text": text, "at": core.now(), "actor": actor,
                                              "answer": None, "answered_at": None})
        self.save(document, "question/asked", actor, {"text": text[:2000]})
        return self.summary(document)

    def answer(self, ticket_id, text, actor="human/owner"):
        """Record the owner's answer; it settles every open question of the ticket."""
        if not actor.startswith("human/"):
            fail("INVALID_ACTOR", "Only the owner answers; record it with a human/ actor on their instruction.")
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed.")
        if not text.strip():
            fail("INVALID_ARGUMENT", "The answer needs --text.")
        stamp = core.now()
        waiting = [q for q in document["flow"]["questions"] if q.get("answer") is None]
        if not waiting:
            # A decision the owner gave without a recorded question, for example after two review rounds.
            waiting = [{"text": None, "at": stamp, "actor": actor}]
            document["flow"]["questions"] += waiting
        for question in waiting:
            question.update(answer=text, answered_at=stamp, answered_by=actor)
        self.save(document, "question/answered", actor, {"text": text[:2000]})
        return self.summary(document)

    def ticket_file(self, ticket_id, file):
        """A file inside the ticket directory as (path, name relative to that directory)."""
        directory = self.tickets.load(ticket_id)[0]
        path = (directory / file).resolve()
        if directory.resolve() not in path.parents or not path.is_file():
            fail("INVALID_ARGUMENT", "The file must exist inside the ticket directory " + str(directory) + ".")
        return path, path.relative_to(directory.resolve()).as_posix()

    def present(self, ticket_id, stage, file, actor="human/owner"):
        """Record that a stage file was shown to the owner and waits for 'approve'."""
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed.")
        relative = self.ticket_file(ticket_id, file)[1]
        document["flow"]["presented"].append({"stage": stage, "file": relative, "actor": actor, "at": core.now()})
        self.save(document, "stage/presented", actor, {"stage": stage, "file": relative})
        return {"ticket_id": ticket_id, "stage": stage, "file": relative}

    def set_profile(self, ticket_id, name=None, actor="human/owner", expected_revision=None, mutation_id=None):
        # Read, compare and save under one lock, including the retry receipt.
        with self.tickets.lock():
            document = self.load(ticket_id)
            before = copy.deepcopy(document)
            fingerprint = {"name": name, "expected_revision": expected_revision, "actor": actor}
            if mutation_id is not None:
                core.identifier(mutation_id)
                if not name or type(expected_revision) is not int:
                    fail("INVALID_ARGUMENT", "A profile mutation ID requires a profile and expected revision.")
                receipt = document["flow"].get("profile_mutations", {}).get(mutation_id)
                if receipt is not None:
                    if receipt["arguments"] != fingerprint:
                        fail("IDEMPOTENCY_CONFLICT", "The profile mutation ID belongs to a different edit.")
                    return copy.deepcopy(receipt["result"])
            if name:
                if document["status"] == "closed":
                    fail("CLOSED", "A closed ticket cannot change profile.")
                if expected_revision is not None and (type(expected_revision) is not int
                                                       or expected_revision != document["revision"]):
                    fail("REVISION_CONFLICT", "The ticket changed meanwhile; refresh and choose again.")
            changed = bool(name and name != document["flow"]["profile"])
            if changed:
                previous = document["flow"]["profile"]
                document["flow"]["profile"] = name
                self.profile(document)
                core.event(document, "flow/profile_changed", actor, previous=previous or "simple", profile=name)
            current, required = self.profile(document)
            profiles = {**DEFAULT_FLOWS, **self.config(document).get("flows", {})}
            result = {"ticket_id": ticket_id, "profile": current, "requires": required,
                      "available": sorted(profiles),
                      "profiles": {key: {"approvals": list(profiles[key].get("approvals", [])),
                                         "reviews": list(profiles[key].get("reviews", []))} for key in sorted(profiles)},
                      "revision": document["revision"] + int(changed or mutation_id is not None)}
            if mutation_id is not None:
                document["flow"].setdefault("profile_mutations", {})[mutation_id] = {
                    "arguments": fingerprint, "result": copy.deepcopy(result)}
            if changed or mutation_id is not None:
                self.tickets.save(before, document, actor, "flow/profile")
            return result

    def stage(self, ticket_id, stage, actor="human/owner", provider=None):
        """Start the analysis, design or mockup task and print its brief."""
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed; create a new one for further work.")
        import policy
        task = next((t for t in document["tasks"] if t.get("phase") == stage and t.get("action") == "implement"), None)
        if task is None:
            task = core.new_task(stage.capitalize(), policy.task_type(stage, "implement"), phase=stage,
                                 action="implement", workflow_step=stage + "/implement")
            document["tasks"].append(task)
        if task["status"] != "running" or document["status"] != "in_progress":
            task["difficulty"] = document.get("difficulty", "unknown")
            document["status"] = "in_progress"
            if task["status"] != "running":
                core.start_task(task, actor, stage.capitalize() + " started")
                self.progress(document, stage.capitalize() + " started", task, actor)
            self.save(document, "stage/started", actor, {"stage": stage, "task_id": task["task_id"]})
        directory = self.tickets.load(ticket_id)[0]
        name = stage + (".png" if stage == "mockup" else ".md")
        cli = python() + ' ".factory2/factory.py" --project "' + str(self.primary) + '" flow'
        returned = [a for a in document["flow"]["approvals"] if a["stage"] == stage and not a["approved"]]
        lines = ["Ticket " + ticket_id + ": " + document["title"], "", document["description"].strip(), "",
                 "Prepare the " + stage + " for this ticket. Do not change project code.",
                 "Write the result as one file in the ticket directory, for example:",
                 "  " + str(directory / name),
                 "State open questions for the owner in that file instead of guessing.",
                 "The owner decides: " + cli + " approve " + ticket_id + " --stage " + stage + " --file " + name
                 + " [--reject --text \"...\"]"]
        lines.append(self.progress_hint(ticket_id))
        recommendation = self.recommendation(document, task, provider)
        lines.append(self.recommendation_line(recommendation))
        if returned:
            lines += ["", "Owner comments to address:", returned[-1]["text"]]
        lines += self.rules(document)
        return {"ticket_id": ticket_id, "stage": stage, "task_id": task["task_id"], "brief": "\n".join(lines),
                "recommendation": recommendation}

    def approve(self, ticket_id, stage, file, reject=False, text="", actor="human/owner"):
        """Record the owner's decision for the exact content of one stage file."""
        if not actor.startswith("human/"):
            fail("INVALID_ACTOR", "Only the owner approves a stage; record it with a human/ actor on their instruction.")
        document = self.load(ticket_id)
        if document["status"] == "closed":
            fail("CLOSED", "The ticket is closed.")
        if reject and not text.strip():
            fail("INVALID_ARGUMENT", "Returning a stage needs the owner's comments in --text.")
        path, relative = self.ticket_file(ticket_id, file)
        document["flow"]["approvals"].append({"stage": stage, "file": relative, "approved": not reject, "text": text,
                                              "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                                              "actor": actor, "at": core.now()})
        import policy
        task = next((t for t in document["tasks"] if t.get("phase") == stage and t.get("action") == "implement"), None)
        if task is None:
            task = core.new_task(stage.capitalize(), policy.task_type(stage, "implement"), phase=stage,
                                 action="implement", workflow_step=stage + "/implement",
                                 difficulty=document.get("difficulty", "unknown"))
            document["tasks"].append(task)
        if task["status"] != "running":
            core.start_task(task, actor, stage.capitalize() + " presented")
        summary = text[:2000] or ("Approved " + relative if not reject else "Returned")
        core.stop_task(task, "failed" if reject else "succeeded", summary, actor)
        task["result"] = {"summary": summary, "source": "owner", "recorded_by": actor}
        self.progress(document, stage.capitalize() + (" returned by the owner" if reject else " approved: " + relative),
                      task, actor)
        self.save(document, "stage/" + ("returned" if reject else "approved"), actor,
                  {"stage": stage, "task_id": task["task_id"], "file": relative})
        response = {"ticket_id": ticket_id, "stage": stage, "approved": not reject, "file": relative}
        if reject:
            response["next"] = "Run 'flow stage " + ticket_id + " --stage " + stage + "' and revise the file."
        return response

    def status(self, ticket_id):
        document = self.load(ticket_id)
        response = self.summary(document)
        name, required = self.profile(document)
        response.update(profile=name, requires=required)
        if document["status"] == "in_progress":
            tree = self.worktree(document)
            tree_hash, dirty = None, False
            if tree.is_dir():
                tree_hash = git(tree, "rev-parse", "HEAD^{tree}")
                dirty = bool(git(tree, "status", "--porcelain"))
                response.update(worktree=str(tree), tree=tree_hash, uncommitted_changes=dirty,
                                base=document["flow"]["base"], changed_files=self.changed(document),
                                commit=("automatic" if self.config(document)["git"]["auto_commit"]
                                        else "needs 'flow close --commit' on the owner's instruction"),
                                merge=("automatic" if self.config(document)["git"]["auto_merge_and_cleanup"]
                                       else "needs 'flow close --merge' on the owner's instruction"))
            response["missing"] = ((["Uncommitted changes: run 'flow check' to record them."] if dirty else [])
                                   + self.gates(document, tree_hash))
        response["sub_tickets"] = [{"ticket_id": d["ticket_id"], "title": d["title"], "status": d["status"]}
                                   for d in self.tickets.documents(archive=True)
                                   if d.get("related_ticket_id") == ticket_id]
        response["checks"] = document["flow"]["checks"][-5:]
        response["reviews"] = document["flow"]["reviews"][-3:]
        response["approvals"] = document["flow"]["approvals"][-3:]
        return response

    def brief(self, ticket_id, role="implement", actor="human/owner", kind="code", provider=None):
        document = self.load(ticket_id)
        tree = self.worktree(document)
        review = next((t for t in document["tasks"] if t["title"] == review_title(kind)), None)
        if role == "review" and document["status"] == "in_progress" and not (review and review["status"] == "running"):
            # Asking for the reviewer brief is the handoff: it stops the implementer's clock and starts the reviewer's.
            self.progress(document, "Review started", self.handoff(document, actor, kind), actor)
            self.save(document, "review/started", actor, {"kind": kind})
        cli = python() + ' ".factory2/factory.py" --project "' + str(self.primary) + '" flow'
        lines = ["Ticket " + ticket_id + ": " + document["title"], "", document["description"].strip(), ""]
        notes = [note for note in document["flow"]["notes"] if (note.get("text") or "").strip()]
        if notes:
            # Additions and corrections made after the ticket was written reach the worker with the brief.
            lines += ["Notes added to the ticket, oldest first; they add to or correct the text above:"]
            lines += ["- " + note["at"] + " " + note["actor"] + ": " + note["text"].strip() for note in notes] + [""]
        lines.append("Workspace (ticket branch " + str(document["branch"]) + "): " + str(tree))
        configured = [c["name"] + ": " + c["command"] for c in self.config(document).get("checks", [])]
        if role == "review":
            base = document["flow"]["base"]
            focus = ("Judge the diff against the ticket: correctness, tests for the changed behavior, "
                     "documentation, needless complexity." if kind == "code" else
                     "This is the " + kind + " review: judge the diff from that point of view only.")
            lines += ["", "You are the independent reviewer; you did not write this change.",
                      "Inspect: git -C \"" + str(tree) + "\" diff " + str(base) + "...HEAD",
                      focus + " Report concrete defects only; do not ask for work beyond the ticket.",
                      "Record: " + cli + " review " + ticket_id + ("" if kind == "code" else " --kind " + kind)
                      + " --verdict pass|changes --text \"...\"", self.progress_hint(ticket_id)]
            recent = document["flow"]["checks"][-5:]
            if recent:
                lines += ["", "Recorded checks:"] + [
                    "- " + c["name"] + " " + ("passed" if c["passed"] else "FAILED") + " at " + c["commit"][:10]
                    + (" " + json.dumps(c["counts"]) if c["counts"] else "") for c in recent]
            lines += ["", "To rerun one configured check: " + cli + " check " + ticket_id + " --check NAME"]
        else:
            lines += ["", "Implement the change in the workspace, including tests and documentation for the "
                          "changed behavior. Keep the change minimal.",
                      "Verify: " + cli + " check " + ticket_id + " [--check NAME ... | --command \"...\"]",
                      "Plain check runs the configured checks for the changed files; --check NAME runs only "
                      "that configured check (names are listed below). Close needs a passed result of every check "
                      "that plain check chooses, unless only --command ran.",
                      "A failed check returns its output; fix and repeat. Do not close the ticket yourself.",
                      self.progress_hint(ticket_id)]
            previous = [r for r in document["flow"]["reviews"] if r["verdict"] == "changes"]
            if previous:
                lines += ["", "Review notes to address:", previous[-1]["notes"]]
        # The task this worker runs; before 'flow start' or the handoff it does not exist yet.
        title, task_type = (("Implement", "implementation") if role != "review" else
                            (review_title(kind), "security_review" if kind == "security" else "review"))
        task = next((t for t in document["tasks"] if t["title"] == title and t["type"] == task_type),
                    {"title": title, "type": task_type, "attempts": []})
        recommendation = self.recommendation(document, task, provider)
        lines.append(self.recommendation_line(recommendation))
        directory = self.tickets.load(ticket_id)[0]
        approved = {a["stage"]: a for a in document["flow"]["approvals"]}
        if any(a["approved"] for a in approved.values()):
            lines += ["", "Approved by the owner; the change must follow it:"] + [
                "- " + stage + ": " + str(directory / a["file"]) for stage, a in approved.items() if a["approved"]]
        if configured:
            lines += ["", "Configured checks:"] + ["- " + item for item in configured]
        lines += self.rules(document)
        return {"ticket_id": ticket_id, "role": role, "brief": "\n".join(lines), "recommendation": recommendation}

    def recommendation(self, document, task, provider=None):
        """What policy select recommends for the worker's task, for each provider with a policy; no Git call.

        A recorded task is passed to policy select as stored, so both give the same result; the stand-in
        for a task that does not exist yet takes the ticket's difficulty.
        """
        import policy
        settings = self.config(document)
        if "task_id" not in task:
            task = dict(task, difficulty=document.get("difficulty", "unknown"))
        names = [provider] if provider else sorted(policy.PROVIDERS)
        selections = [policy.select(settings, task, name) for name in names if name in settings["models"]]
        return {"task": task["title"], "task_id": task.get("task_id"), "assignment_type": task["type"],
                "difficulty": task.get("difficulty", "unknown"),
                "providers": [{"provider": s["requested"]["provider"], "tier": s["tier"],
                               "model": s["requested"]["model"], "reasoning": s["requested"]["reasoning"],
                               "failed_attempts": s["failed_attempts"], "reasons": s["reasons"]} for s in selections]}

    @staticmethod
    def recommendation_line(recommendation):
        """Brief text: the recommended model in one line."""
        choices = "; ".join(p["provider"] + " " + p["model"] + ", reasoning " + p["reasoning"] + " (" + p["tier"] + ")"
                            for p in recommendation["providers"]) or "none (no provider policy is configured)"
        return ("Recommended model for the " + recommendation["task"] + " task (" + recommendation["assignment_type"]
                + ", difficulty " + recommendation["difficulty"] + "): " + choices
                + ". Record the model you actually run on with your first flow log line: add --model MODEL "
                "[--provider PROVIDER] [--reasoning LEVEL] to it.")

    def progress_hint(self, ticket_id):
        """Brief text: when a worker records a progress line and what a good one looks like."""
        return ("Progress: when you start a distinct piece of work, and before you run tests of your own outside "
                "'flow check', record one line: " + python() + " \".factory2/factory.py\" --project \"" + str(self.primary)
                + "\" flow log " + ticket_id + " --text \"...\". A good line is one sentence under about 100 "
                "characters that says what you are doing now, for example \"Writing the parser tests\" or "
                "\"Running console tests: test_console_waiting.py\"; never put secrets in it. "
                "Flow commands record their own start and result.")

    def note(self, ticket_id, text, tokens=None, actor="human/owner"):
        document = self.load(ticket_id)
        document["flow"]["notes"].append({"at": core.now(), "actor": actor, "text": text, "tokens": tokens})
        self.save(document, "ticket/note", actor, {"text": text[:2000], "tokens": tokens})
        return self.summary(document)

    def close(self, ticket_id, unresolved=False, reason="", merge=False, actor="human/owner", commit=False):
        document = self.load(ticket_id)
        # Committing the archived ticket needs git/auto_commit or the owner's consent, given with --commit
        # or, for a landed change, when that change was committed.
        consented = commit or self.config(document)["git"]["auto_commit"]
        if document["status"] == "closed":
            # Closing already happened; finish any commit, cleanup or push that was interrupted.
            landed = document["closure"].get("change_commit")
            if consented or landed:
                self.commit_closed(document)
            # Only a ticket whose change is on the base branch loses its worktree and branch here: one
            # closed without a change commit, for example by 'ticket close', may hold work that never landed.
            return {**self.summary(document), "already_closed": True,
                    **(self.cleanup(document) if landed else {}), **self.push(document)}
        if unresolved:
            if not reason.strip():
                fail("INVALID_ARGUMENT", "An unresolved closure needs --reason.")
            return self.archive(document, actor, "unresolved", reason, landed=None, commit=consented)
        if document["status"] != "in_progress":
            fail("NOT_STARTED", "Run 'flow start " + ticket_id + "' first.")
        tip, tree_hash = self.snapshot(document)
        base, tree = document["flow"]["base"], self.worktree(document)
        # A sub-ticket lands on its parent's branch, in the parent's worktree.
        target = self.primary
        if base.startswith("ticket/"):
            target = self.worktree({"ticket_id": base[len("ticket/"):]})
        if not target.is_dir() or git(target, "rev-parse", "--abbrev-ref", "HEAD") != base:
            fail("WRONG_BRANCH", "The checkout " + str(target) + " must be on " + base + " to close this ticket.")
        head = git(target, "rev-parse", base)
        contained = lambda: not git(target, "merge-base", "--is-ancestor", landed, head, check=False).returncode
        # A repeated close: the ticket branch is exactly the change commit that an earlier run recorded on
        # the ticket after the gates and the merge permission were satisfied. It is on the base branch
        # already, or is its next commit; nothing is landed twice. Any other commit, whatever its message,
        # passes the gates and the permission below.
        record = document["flow"].get("landing") or {}
        landed = tip if record.get("commit") == tip and record.get("base") == base else None
        if landed and not (contained() or git(tree, "rev-parse", landed + "^") == head):
            landed = None  # The base moved before the commit reached it: land the work again.
        if landed and not contained():
            # Not on the base yet: it still must not land against the records. The requirements hold for the
            # tree that was checked and reviewed, and nothing negative is recorded for the tree of the commit.
            # The merge permission that authorised this landing covers finishing it.
            missing = self.gates(document, record["tree"])
            if tree_hash != record["tree"]:
                state = document["flow"]
                if any(c["tree"] == tree_hash and not c["passed"] for c in state["checks"]):
                    missing.append("A check failed for the current tree: fix it and run 'flow check' again.")
                for kind in self.profile(document)[1]["reviews"]:
                    reviews = [r for r in state["reviews"] if r["tree"] == tree_hash and r.get("kind", "code") == kind]
                    if reviews and reviews[-1]["verdict"] != "pass":
                        missing.append("The latest " + kind + " review asked for changes: address them, then "
                                       "check and review again.")
            if missing:
                fail("GATE", " ".join(missing))
        if not landed:
            missing = self.gates(document, tree_hash)
            if missing:
                fail("GATE", " ".join(missing))
            options = self.consent(document)
            for option, given, code, text in (
                    (" --commit", consented, "COMMIT_NOT_PERMITTED", "Committing is not automatic (git/auto_commit"),
                    (" --merge", merge, "MERGE_NOT_PERMITTED", "Merging is not automatic (git/auto_merge_and_cleanup")):
                if option in options and not given:
                    # The owner's Git permission, not a workflow state: nothing is recorded.
                    fail(code, "Checks and review pass for the current tree. " + text + " is false): on the "
                         "owner's instruction run 'flow close " + ticket_id + options + "'.")
            result = tree_hash
            if git(tree, "merge-base", base, "HEAD") != head:
                # The base moved: the merged tree is computed, and a conflict reported, before anything changes.
                trial = git(target, "merge-tree", "--write-tree", base, tip, check=False)
                if trial.returncode:
                    fail("CONFLICT", "The ticket branch conflicts with " + base + ". In the workspace run "
                                     "'git merge " + base + "', resolve the conflicts, then check and review again.")
                result = trial.stdout.split()[0]
            # One change commit on top of the base branch: the ticket's whole change, without a merge commit.
            signed = git(target, "config", "--type=bool", "--get", "commit.gpgsign", check=False).stdout.strip()
            landed = git(target, "commit-tree", result, "-p", head, *(["-S"] if signed == "true" else []),
                         "-m", document["title"], "-m", "Ticket: " + ticket_id)
            # Recorded on the ticket as it is now, under the lock, before any branch moves: only this commit
            # is later taken for a change commit. Interrupted before this, the commit object is unreferenced
            # and a repeat starts over; a later landing replaces the record.
            self.amend(ticket_id, lambda current: current["flow"].update(
                landing={"commit": landed, "tree": tree_hash, "base": base, "at": core.now()}), actor, "flow/close")
            document = self.load(ticket_id)
            # The ticket branch becomes that commit first, so that after any interruption a repeated close
            # finds it and later work in the worktree, removals included, is a difference from it.
            git(tree, "reset", "-q", "--hard", landed)
        if not contained():
            forward = git(target, "merge", "--ff-only", "-q", landed, check=False)
            if forward.returncode:
                fail("MERGE_FAILED", "Moving " + base + " to the change commit failed and " + base
                     + " was not changed:\n" + (forward.stderr or forward.stdout).strip()
                     + "\nResolve the cause and run 'flow close' again.")
        if base.startswith("ticket/"):
            # The parent's tree changed; recorded on the parent so that reads need neither Git nor the archive.
            # A closed parent never waits and a missing one has nothing to record; neither stops this close.
            try:
                parent = self.load(base[len("ticket/"):])
            except FactoryError:
                parent = None
            if parent and parent["status"] != "closed" and not any(
                    entry["event"] == "sub_ticket/merged" and entry.get("commit") == landed
                    for entry in parent["history"]):
                parent["flow"]["sub_ticket_merged_at"] = core.now()
                self.save(parent, "sub_ticket/merged", actor, {"sub_ticket_id": ticket_id, "commit": landed})
        return self.archive(document, actor, "resolved", reason or "Checks and review passed.", landed=landed)

    def archive(self, document, actor, resolution, reason, landed, commit=True):
        with self.tickets.lock():
            before = self.current(document)
            self.tickets.finish(document, actor, resolution, reason, mode="flow",
                                verification="checks_and_review" if resolution == "resolved" else "not_claimed",
                                **({"change_commit": landed} if landed else {}))
            self.tickets.save(before, document, actor, "flow/close")
        if commit:
            self.commit_closed(document)
        response = {**self.summary(document), **(self.cleanup(document) if resolution == "resolved" else {})}
        response.update(self.push(document) if resolution == "resolved" else {"pushed": False})
        if resolution != "resolved":
            # The closure is recorded: a directory that is not this ticket's worktree is left alone and named.
            tree, place = self.own_tree(document), core.worktree_locations(self.primary, document["ticket_id"])[0]
            if tree:
                response["kept_worktree"] = str(tree)
            elif place.exists():
                response["directory_left_untouched"] = str(place)
        return response

    def commit_closed(self, document):
        """Commit the archived ticket on its own so that the archive follows the repository; repeatable."""
        error = self.tickets.commit(document["ticket_id"], "close")
        if error:
            fail("GIT_FAILED", "The closed ticket could not be committed:\n" + error
                 + "\nResolve the cause and run 'flow close' again; the ticket is already closed.")

    def push(self, document):
        """Push the base branch when the effective configuration permits it; repeatable."""
        if (document["closure"]["resolution"] != "resolved" or not self.config(document)["git"]["auto_push"]
                or (document["flow"]["base"] or "").startswith("ticket/")):
            # A sub-ticket lands on its parent's branch; the parent's closure publishes it.
            return {"pushed": False}
        result = git(self.primary, "push", check=False)
        if result.returncode:
            return {"pushed": False, "push_error": (result.stderr or result.stdout).strip(),
                    "next": "Resolve the push problem and run 'flow close' again; the ticket is already closed."}
        return {"pushed": True}

    def cleanup(self, document):
        """Remove the worktree and the landed branch; safe to repeat.

        A worktree that cannot be removed completely, for example while a program holds one of its
        files open on Windows, does not fail the closure: the directory is returned as leftover. What
        remains of it is usually no Git worktree any more, so a later run names the directory at the
        worktree path as left untouched and never removes it.
        """
        if document["closure"]["resolution"] != "resolved":
            return {}
        # Only the ticket's own worktree is removed; any other directory at that path is never touched.
        tree, branch = self.own_tree(document), document.get("branch")
        landed = document["closure"].get("change_commit")
        base = document["flow"].get("base")
        # Close resets the ticket branch to its squash commit before landing. Progress commits
        # are not ancestors of that commit; compare the current branch with the landed base instead.
        safe = branch == "ticket/" + document["ticket_id"] and landed and base
        if safe:
            safe = not git(self.primary, "merge-base", "--is-ancestor", landed, base, check=False).returncode
        if safe and self.branch_exists(branch):
            safe = not git(self.primary, "merge-base", "--is-ancestor", branch, base, check=False).returncode
        if tree:
            # Recheck ownership even if this Flow instance verified the path earlier.
            self.own_worktree(tree, "ticket/" + document["ticket_id"])
            common = git(tree, "rev-parse", "--path-format=absolute", "--git-common-dir", check=False)
            primary = git(self.primary, "rev-parse", "--path-format=absolute", "--git-common-dir")
            status = git(tree, "status", "--porcelain", "--untracked-files=all", check=False)
            safe = (safe and not common.returncode and common.stdout.strip() == primary
                    and not status.returncode and not status.stdout.strip())
        if not safe:
            return {**({"kept_worktree": str(tree)} if tree else {}), "kept_branch": branch,
                    "cleanup_hint": "The ticket is closed, but cleanup could not prove its work is safe to "
                                    "remove. Preserve or integrate later changes and commits with Git, then "
                                    "run 'flow close " + document["ticket_id"] + "' again."}
        if tree:
            git(self.primary, "worktree", "remove", "--force", str(tree), check=False)
            shutil.rmtree(tree, ignore_errors=True)
        git(self.primary, "worktree", "prune", check=False)
        self.drop_old_worktrees()
        if branch and self.branch_exists(branch):
            git(self.primary, "branch", "-D", branch, check=False)
        place = core.worktree_locations(self.primary, document["ticket_id"])[0]
        if tree and tree.exists():
            return {"leftover_worktree": str(tree),
                    "leftover_hint": "The ticket is closed and its change has landed. Part of the worktree could "
                                     "not be removed; delete the directory yourself once no program uses it. "
                                     "Factory does not remove it later."}
        if place.exists():
            return {"directory_left_untouched": str(place)}
        return {}

    @staticmethod
    def summary(document):
        return {"ticket_id": document["ticket_id"], "title": document["title"], "status": document["status"],
                "type": document["type"], "priority": document.get("priority", "low"),
                "severity": document.get("severity", "low"), "branch": document.get("branch"),
                "created_at": document["created_at"], "closure": document.get("closure")}


def add_arguments(groups):
    flow = groups.add_parser("flow", help="Ticket flow: start, stage, approve, check, review, close").add_subparsers(
        dest="command", required=True)
    new = flow.add_parser("new", help="Create a ticket")
    new.add_argument("--title", required=True)
    new.add_argument("--description", default="")
    new.add_argument("--type", choices=["bug", "change", "check"], default="change")
    new.add_argument("--priority", choices=["high", "medium", "low"])
    new.add_argument("--severity", choices=["low", "medium", "high", "critical"])
    new.add_argument("--overrides", type=Path, help="JSON file containing ticket config overrides")
    new.add_argument("--profile", help="Proposed flow profile: simple, full, full-ui or one from config.json")
    for name, text in (("start", "Create the branch and worktree and print the implementer brief"),
                       ("profile", "Show or change what closing this ticket requires"),
                       ("stage", "Start analysis, design or mockup and print its brief"),
                       ("present", "Record that a stage file was shown to the owner for approval"),
                       ("approve", "Record the owner's approval or return of a stage file"),
                       ("ask", "Record a question put to the owner about the ticket"),
                       ("answer", "Record the owner's answer to the open questions"),
                       ("status", "Show what is still missing before closing"),
                       ("brief", "Print the assignment text for a worker"),
                       ("check", "Commit pending work and run the checks for the current tree"),
                       ("review", "Record a review verdict for the current tree"),
                       ("note", "Record a free-text note or token usage"),
                       ("log", "Record one short progress line in the journal of the running task"),
                       ("close", "Verify checks and review, land the change as one commit and archive")):
        command = flow.add_parser(name, help=text)
        command.add_argument("ticket_id")
        if name in ("start", "brief", "stage"):
            command.add_argument("--provider", choices=["openai", "anthropic", "deepseek"],
                                 help="Recommend a model for this provider only; default: every provider with a policy")
        if name == "brief":
            command.add_argument("--role", choices=["implement", "review"], default="implement")
            command.add_argument("--kind", default="code", help="Review kind for --role review, for example security")
        if name == "profile":
            command.add_argument("name", nargs="?", help="simple, full, full-ui or a profile from config.json")
            command.add_argument("--expected-revision", type=int)
            command.add_argument("--mutation-id", help="UUID identifying this profile edit for exact retry")
        if name in ("stage", "present", "approve"):
            command.add_argument("--stage", choices=STAGES, required=True)
        if name in ("present", "approve"):
            command.add_argument("--file", required=True, help="File inside the ticket directory")
        if name in ("ask", "answer"):
            command.add_argument("--text", required=True)
        if name == "approve":
            command.add_argument("--reject", action="store_true", help="The owner returns it; requires --text")
            command.add_argument("--text", default="", help="The owner's words")
        if name == "check":
            command.add_argument("--command", dest="commands", action="append",
                                 help="Shell command run in the worktree; repeatable. Default: configured "
                                      "checks matching the changed files. {python} stands for the running interpreter")
            command.add_argument("--check", dest="names", action="append",
                                 help="Name of a configured check to run; repeatable. Not combinable with --command")
            command.add_argument("--timeout", type=int)
        if name == "review":
            command.add_argument("--verdict", choices=["pass", "changes"], required=True)
            text_source = command.add_mutually_exclusive_group()
            text_source.add_argument("--text", default="")
            text_source.add_argument("--notes", type=Path, help="File containing the review notes")
            command.add_argument("--reviewer")
            command.add_argument("--kind", default="code", help="Review kind; a profile lists the required kinds")
        if name == "note":
            command.add_argument("--text", required=True)
            command.add_argument("--tokens", type=int)
        if name == "log":
            command.add_argument("--text", help="One short sentence saying what is being done now; "
                                                "optional with --model")
            command.add_argument("--task", help="implement, review, a stage name, another task title or a task ID; "
                                                "default: the running task")
            command.add_argument("--model", help="Model identifier running the task, recorded on its latest attempt")
            command.add_argument("--provider", help="Provider of --model")
            command.add_argument("--reasoning", help="Reasoning level of --model")
        if name == "close":
            command.add_argument("--unresolved", action="store_true",
                                 help="Owner closes without merging; requires --reason")
            command.add_argument("--reason", default="")
            command.add_argument("--merge", action="store_true",
                                 help="Owner authorizes the merge when git/auto_merge_and_cleanup is false")
            command.add_argument("--commit", action="store_true",
                                 help="Owner authorizes the commits when git/auto_commit is false")


def run(project, command, arguments, actor):
    flow = Flow(project)
    result = dispatch(flow, command, arguments, actor)
    # Every result says whether the ticket now waits for the owner, and for what.
    result.update(flow.tickets.waiting(flow.tickets.load(result["ticket_id"])[1]))
    if flow.moved:
        # A worker that used the old absolute path continues under the new one.
        result["worktrees_moved"] = flow.moved
    if flow.unmoved:
        # The command worked in the old location; the next command tries the move again.
        result["worktrees_not_moved"] = [{"worktree": path, "reason": reason}
                                         for path, reason in sorted(flow.unmoved.items())]
    return result


def dispatch(flow, command, arguments, actor):
    ticket = arguments.get("ticket_id")
    if command == "new":
        overrides = json.loads(arguments["overrides"].read_text(encoding="utf-8")) if arguments.get("overrides") else {}
        return flow.new(arguments["title"], arguments.get("description", ""), arguments.get("type", "change"),
                        arguments.get("priority"), arguments.get("severity"), actor, overrides,
                        arguments.get("profile"))
    if command == "start":
        return flow.start(ticket, actor, arguments.get("provider"))
    if command == "status":
        return flow.status(ticket)
    if command == "brief":
        return flow.brief(ticket, arguments.get("role", "implement"), actor, arguments.get("kind", "code"),
                          arguments.get("provider"))
    if command == "profile":
        return flow.set_profile(ticket, arguments.get("name"), actor,
                                arguments.get("expected_revision"), arguments.get("mutation_id"))
    if command == "stage":
        return flow.stage(ticket, arguments["stage"], actor, arguments.get("provider"))
    if command == "present":
        return flow.present(ticket, arguments["stage"], arguments["file"], actor)
    if command in ("ask", "answer"):
        return getattr(flow, command)(ticket, arguments["text"], actor)
    if command == "approve":
        return flow.approve(ticket, arguments["stage"], arguments["file"], bool(arguments.get("reject")),
                            arguments.get("text", ""), actor)
    if command == "check":
        return flow.check(ticket, arguments.get("commands"), arguments.get("timeout"), actor,
                          arguments.get("names"))
    if command == "review":
        notes = arguments["notes"].read_text(encoding="utf-8") if arguments.get("notes") else arguments.get("text", "")
        return flow.review(ticket, arguments["verdict"], notes, arguments.get("reviewer"), actor,
                           arguments.get("kind", "code"))
    if command == "note":
        return flow.note(ticket, arguments["text"], arguments.get("tokens"), actor)
    if command == "log":
        return flow.log(ticket, arguments.get("text"), arguments.get("task"), actor,
                        arguments.get("model"), arguments.get("provider"), arguments.get("reasoning"))
    if command == "close":
        return flow.close(ticket, bool(arguments.get("unresolved")), arguments.get("reason", ""),
                          bool(arguments.get("merge")), actor, bool(arguments.get("commit")))
    fail("INVALID_ARGUMENT", "Unknown flow command.")
