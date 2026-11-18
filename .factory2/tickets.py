"""Tickets, tasks, journal and settings on plain files.

One ticket is one directory with ticket.json and journal.jsonl. A mutation
takes the project lock, checks the optional expected revision, writes both
files and returns. There are no receipts, replays or recovery states: a
command that did not finish is simply run again.

In a Git project with git/auto_commit enabled, creating a ticket, changing its
assignment and closing it also commit that ticket's files; reads and every other
mutation never call Git. Without git/auto_commit these commands commit nothing.
"""

from contextlib import contextmanager
import copy
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from uuid import UUID, uuid4

import config
import factory_platform as platform_ops
from errors import FactoryError, require


TICKET_PRIORITIES = ("high", "medium", "low")
TICKET_SEVERITIES = ("low", "medium", "high", "critical")
TICKET_TYPES = ("bug", "change", "check")
TASK_TYPES = {"analysis", "planning", "implementation", "test_design", "verification",
              "review", "documentation", "security_review", "repository_review"}
FINAL = {"succeeded", "failed", "interrupted", "cancelled"}
# Operations that commit the ticket's files in a Git project when git/auto_commit is enabled, and the word
# in the commit subject.
COMMITS = {"ticket/create": "open", "ticket/edit": "update", "ticket/priority": "update",
           "ticket/severity": "update", "ticket/type": "update", "ticket/confirm": "update", "ticket/close": "close"}

SECRET_KEY = re.compile(r"(?i)(password|credential|secret|api[_-]?key|access[_-]?token|capability[_-]?token|authorization)")
SECRET_TEXT = re.compile(r"(?i)(\b(?:password|secret|token|api[_-]?key|capability[_-]?token)\s*[=:]\s*)([^\s,;]+)")
BEARER = re.compile(r"(?i)(\bBearer\s+)[^\s,;]+")


def now():
    return datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")


def identifier(value):
    try:
        require(isinstance(value, str) and str(UUID(value)) == value, "INVALID_ID", "IDs must be canonical UUIDs.")
    except (ValueError, AttributeError) as error:
        raise FactoryError("INVALID_ID", "IDs must be canonical UUIDs.") from error
    return value


def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8")


def digest(value):
    return hashlib.sha256(canonical(value)).hexdigest()


def atomic_write(path, value):
    data = (json.dumps(value, ensure_ascii=False, allow_nan=False, indent=2) + "\n").encode("utf-8")
    platform_ops.atomic_bytes(path, data)


def text(value, label):
    require(isinstance(value, str) and bool(value.strip()), "INVALID_ARGUMENT", label + " is required.")
    return value


def effective_metadata(document):
    """Unset priority and severity read as low without changing the stored ticket."""
    return {key: "low" if document.get(key) is None else document[key] for key in ("priority", "severity")}


def safe_output(value):
    """Defense in depth for journal text, not a general secret detector."""
    if isinstance(value, dict):
        return {key: "[REDACTED]" if SECRET_KEY.search(key) else safe_output(item) for key, item in value.items()}
    if isinstance(value, list):
        return [safe_output(item) for item in value]
    if isinstance(value, str):
        return BEARER.sub(r"\1[REDACTED]", SECRET_TEXT.sub(r"\1[REDACTED]", value))
    return value


def journal_output(details):
    """Journal records carry short plain values only; the ticket holds the rest."""
    return safe_output({key: value if not isinstance(value, str) or len(value) <= 300 else value[:298] + " …"
                        for key, value in details.items()
                        if value is None or isinstance(value, (bool, int, float, str))})


def aggregate_metrics(attempts):
    """Sum attempt wall times; a partial token total never looks complete."""
    measured = [a["tokens"] for a in attempts if isinstance(a.get("tokens"), dict)]
    counts = {}
    for key in ("input", "output", "total"):
        values = [tokens[key] for tokens in measured if tokens.get(key) is not None]
        complete = all(a["status"] != "running" for a in attempts)
        counts[key] = sum(values) if attempts and len(values) == len(attempts) and complete else None
        counts["known_" + key] = sum(values) if values else None
    counts.update(complete=bool(attempts) and counts["total"] is not None,
                  measured_attempts=sum(t.get("total") is not None for t in measured), total_attempts=len(attempts))
    return {"duration_ms": sum(round((a.get("duration_ms") or 0) * 1000) for a in attempts) / 1000,
            "duration_basis": "sum_of_completed_attempt_wall_times",
            "running_attempts": sum(a["status"] == "running" for a in attempts),
            "unknown_duration_attempts": sum(a["status"] != "running" and a.get("duration_ms") is None for a in attempts),
            "tokens": counts}


def update_metrics(document):
    attempts = []
    for task in document["tasks"]:
        for attempt in task["attempts"]:
            if attempt.get("ended_at"):
                start = datetime.fromisoformat(attempt["started_at"].replace("Z", "+00:00"))
                end = datetime.fromisoformat(attempt["ended_at"].replace("Z", "+00:00"))
                elapsed = (end - start).total_seconds() * 1000
                attempt["duration_ms"] = round(elapsed, 3) if elapsed >= 0 else None
            else:
                attempt["duration_ms"] = None
            attempt.setdefault("tokens", None)
            attempts.append(attempt)
        task["metrics"] = aggregate_metrics(task["attempts"])
    document["metrics"] = aggregate_metrics(attempts)


def without_workers(task):
    """The task apart from the models recorded on its attempts."""
    return task and dict(task, attempts=[dict(attempt, worker=None) for attempt in task["attempts"]])


def new_task(title, kind, **extra):
    return {"task_id": str(uuid4()), "title": title, "type": kind, "status": "pending", "depends_on": [],
            "attempts": [], "history": [], "result": None, "paths": [], **extra}


def start_task(task, actor="human/owner", reason=None):
    stamp = now()
    task["result"] = None
    task["status"] = "running"
    task["attempts"].append({"attempt_id": str(uuid4()), "number": len(task["attempts"]) + 1,
                             "execution_mode": "manual", "worker": None, "status": "running",
                             "started_at": stamp, "last_activity_at": stamp, "ended_at": None, "outcome": None})
    task["history"].append({"at": stamp, "status": "running", "actor": actor, "reason": reason or "Started."})


def stop_task(task, status, reason, actor):
    stamp = now()
    if task["status"] == "running":
        task["attempts"][-1].update(status=status if status in FINAL else "interrupted", ended_at=stamp,
                                    last_activity_at=stamp,
                                    outcome={"summary": reason, "recorded_by": actor, "source": "human_report"})
    task["status"] = status
    task["history"].append({"at": stamp, "status": status, "actor": actor, "reason": reason})


def event(document, name, actor, **details):
    document["history"].append({"event_id": str(uuid4()), "event": name, "at": now(), "actor": actor, **details})


def primary_checkout(project):
    """The primary checkout of a linked Git worktree; otherwise the project itself."""
    marker = project / ".git"
    if marker.is_file():
        try:
            line = marker.read_text(encoding="utf-8").strip()
            common = Path(line[len("gitdir:"):].strip())
            if line.startswith("gitdir:") and common.parent.name == "worktrees" and common.parent.parent.name == ".git":
                return common.parent.parent.parent
        except (OSError, UnicodeError):
            pass
    return project


def run_git(cwd, *arguments, text=True):
    """Run Git in a checkout through the platform layer and return the completed process.

    git_invocation finds the Git executable and, on Windows, applies the checkout trust set up
    there. Its fixed locale and its prompt switch are not used: commits, merges and pushes run
    the user's hooks, signing program and credential helpers with the environment they had.
    """
    argv, environment = platform_ops.git_invocation(cwd)
    for name in ("LC_ALL", "GIT_TERMINAL_PROMPT"):
        environment.pop(name)
        if name in os.environ:
            environment[name] = os.environ[name]
    return subprocess.run([*argv, *arguments], env=environment, capture_output=True, text=text,
                          stdin=subprocess.DEVNULL)


def worktree_locations(project, ticket_id):
    """Where the ticket flow creates a ticket's worktree, and where earlier versions created it."""
    return (project / ".factory2" / "local" / "worktrees" / ticket_id,
            project.parent / (project.name + ".worktrees") / ticket_id)


class Tickets:
    def __init__(self, project, lock_timeout=5.0):
        self.project = Path(project).resolve()
        self.root = self.project / ".factory2"
        # A linked Git worktree shares the Local layer and lock of its primary checkout.
        self.local = primary_checkout(self.project) / ".factory2" / "local"
        self.lock_timeout = lock_timeout
        # config.py and installation.py address the project through these names.
        self.store, self.git = self, None

    # ---- files -------------------------------------------------------------

    def safe(self, path):
        path = Path(path)
        base = next((b for b in (self.root, self.local) if path == b or b in path.parents), None)
        require(base is not None, "UNSAFE_PATH", "Path is outside the Factory directory.")
        for part in (path, *path.parents):
            require(not platform_ops.is_link_or_reparse(part), "UNSAFE_PATH", "Managed paths must not be symlinks.")
            if part == base:
                break
        return path

    @contextmanager
    def lock(self):
        require(self.project.is_dir(), "PROJECT_NOT_FOUND", "Project directory does not exist.")
        self.local.mkdir(parents=True, exist_ok=True)
        try:
            with platform_ops.file_lock(self.local / "lock", self.lock_timeout):
                yield
        except platform_ops.LockBusy as error:
            raise FactoryError("BUSY", "Another local operation holds the Factory lock; repeat the command.") from error

    def directory(self, ticket_id, area):
        return self.safe(self.root / area / identifier(ticket_id))

    def load(self, ticket_id):
        for area in ("tickets", "archive"):
            directory = self.directory(ticket_id, area)
            if (directory / "ticket.json").is_file():
                document = config.read_json(directory / "ticket.json")
                require(isinstance(document, dict) and document.get("ticket_id") == ticket_id,
                        "CORRUPT_STATE", "Ticket identity mismatch.")
                return directory, document
        raise FactoryError("TICKET_NOT_FOUND", "Ticket does not exist.")

    def documents(self, archive=False):
        for area in ("tickets", "archive") if archive else ("tickets",):
            directory = self.root / area
            for entry in sorted(directory.iterdir()) if directory.is_dir() else []:
                if (entry / "ticket.json").is_file():
                    document = config.read_json(entry / "ticket.json")
                    if isinstance(document, dict) and document.get("ticket_id") == entry.name:
                        yield document

    def journal(self, directory, document):
        """Journal bytes, verified against the marker written with the ticket."""
        path = directory / "journal.jsonl"
        data = path.read_bytes() if path.is_file() else b""
        marker = document.get("journal")
        if marker is not None:
            require(marker.get("sha256") == hashlib.sha256(data).hexdigest()
                    and marker.get("entries") == len(data.splitlines()),
                    "JOURNAL_CONFLICT", "The ticket journal is missing or changed outside Factory.")
        else:
            require(not data, "JOURNAL_CONFLICT", "An untracked ticket journal already exists.")
        return data

    def save(self, before, document, actor="human/owner", operation=None, request_id=None):
        """Write the ticket and the journal lines derived from what changed."""
        ticket_id = document["ticket_id"]
        source = next((d for d in (self.directory(ticket_id, a) for a in ("tickets", "archive"))
                       if (d / "ticket.json").is_file()), None)
        target = self.directory(ticket_id, "archive" if document["status"] == "closed" else "tickets")
        update_metrics(document)
        document["revision"] = document.get("revision", 0) + 1
        path = (source or target) / "journal.jsonl"
        data = path.read_bytes() if path.is_file() else b""
        sequence, entries = len(data.splitlines()), []
        stamp = now()

        def emit(name, message, output, task=None, history=None):
            attempt = task["attempts"][-1] if task and task["attempts"] else {}
            entries.append({"schema_version": 1, "sequence": sequence + len(entries) + 1,
                            "event_id": history["event_id"] if history else str(uuid4()),
                            "at": history["at"] if history else stamp, "ticket_id": ticket_id,
                            "revision": document["revision"], "task_id": task["task_id"] if task else None,
                            "attempt_id": attempt.get("attempt_id"), "assignment_id": None,
                            "workflow_step": task.get("workflow_step") if task else None,
                            "event": name, "message": message, "actor": history["actor"] if history else actor,
                            "request_id": request_id, "operation": operation, "output": journal_output(output)})

        for history in document["history"][len(before["history"]) if before else 0:]:
            details = {k: v for k, v in history.items() if k not in {"event", "event_id", "at", "actor"}}
            details["ticket_status"] = document["status"]
            if history["event"] == "ticket/created":
                details.update(title=document["title"], type=document["type"], **effective_metadata(document))
            task = next((t for t in document["tasks"] if t["task_id"] == history.get("task_id")), None)
            # A progress line is its own message; every other event is named after itself.
            message = (journal_output({"text": str(history.get("text") or "")})["text"]
                       if history["event"] == "task/progress"
                       else history["event"].replace("/", " ").replace("_", " ").capitalize() + ".")
            emit(history["event"], message, details, task=task, history=history)
        old_tasks = {task["task_id"]: task for task in before["tasks"]} if before else {}
        for task in document["tasks"]:
            # A model recorded on an attempt is reported by its progress line, not as a task update.
            if without_workers(old_tasks.get(task["task_id"])) == without_workers(task):
                continue
            attempt = task["attempts"][-1] if task["attempts"] else {}
            emit("task/updated", "Task " + task["status"] + ".",
                 {"title": task["title"], "type": task["type"], "status": task["status"],
                  "summary": (task.get("result") or {}).get("summary"), "attempt": attempt.get("number"),
                  "duration_ms": attempt.get("duration_ms")}, task=task)
        data += b"".join(canonical(entry) + b"\n" for entry in entries)
        document["journal"] = {"entries": sequence + len(entries), "sha256": hashlib.sha256(data).hexdigest()}
        if source and source != target:
            target.parent.mkdir(parents=True, exist_ok=True)
            os.replace(source, target)
        target.mkdir(parents=True, exist_ok=True)
        platform_ops.atomic_bytes(target / "journal.jsonl", data)
        atomic_write(target / "ticket.json", document)
        return target

    def commit(self, ticket_id, word):
        """In a Git project, commit this ticket's files, and nothing else, as 'factory: WORD ID'.

        Returns Git's message when the commit could not be made and None otherwise. Git is
        not called outside a Git repository, and nothing is committed when the ticket's
        files hold no change. Other staged or modified files of the checkout stay as they are.
        """
        if not (self.project / ".git").exists():
            return None
        areas = [".factory2/" + area + "/" + ticket_id for area in ("tickets", "archive")]

        def run(*arguments):
            """(standard output, None) or (None, why Git failed)."""
            try:
                done = run_git(self.project, *arguments)
            except OSError as error:
                return None, "Git could not be run (" + type(error).__name__ + ")."
            if done.returncode:
                return None, (done.stderr or done.stdout).strip() or "git " + arguments[0] + " failed."
            return done.stdout, None
        def changed(*arguments):
            """The ticket directories named in Git's listing, and why Git failed if it did."""
            listed, error = run(*arguments, "--", *areas)
            return [area for area in areas if area + "/" in (listed or "")], error
        # Only directories that hold a change are named: Git refuses a path it cannot resolve.
        paths, error = changed("status", "--porcelain", "--untracked-files=all")
        if error or not paths:
            return error
        error = run("add", "-A", "--", *paths)[1]
        if not error:
            # What is staged now decides what 'commit' is given; a path staged and then removed is gone.
            staged, error = changed("diff", "--cached", "--name-only", "--no-renames")
            if error or not staged:
                return error
            error = run("commit", "-q", "--no-verify", "-m", "factory: " + word + " " + ticket_id, "--", *staged)[1]
        if error:
            # A refused commit leaves nothing of the ticket staged: the files stay in the working tree for
            # the ticket's next commit and are not swept into a commit or an aborted merge of the owner.
            run("reset", "-q", "--", *areas)
        return error

    # ---- configuration -----------------------------------------------------

    def _arguments(self, arguments, allowed):
        require(set(arguments) <= set(allowed), "INVALID_ARGUMENT", "Unexpected argument.")

    def _effective_config(self, ticket=None, paths=None):
        project = config.read_json(self.safe(self.root / "config.json"))
        require("rules" not in project, "RULES_IN_CONFIG",
                "Move project rules to project-rules.json; config.json stores project settings.")
        local_path = self.safe(self.local / "config.json")
        local = config.read_json(local_path) if local_path.exists() else {}
        overrides = ticket.get("config_overrides", {}) if ticket else {}
        result = config.resolve(project, local, overrides)
        import rules
        assigned = rules.paths(self.project, paths if paths is not None else [])
        result.update(rules.compose(self.project, assigned, overrides.get("rules", {}),
                                    local_project=self.local.parent.parent, local_rules=local.get("rules", {})))
        return result

    def _git_settings(self, ticket):
        """The effective git settings for a ticket: project config.json, Local layer, ticket overrides."""
        layers = [config.read_json(path) if path.is_file() else {}
                  for path in (self.safe(self.root / "config.json"), self.safe(self.local / "config.json"))]
        return config.resolve(*layers, ticket.get("config_overrides", {}))["git"]

    def commit_settings(self, target, before, after, request_id, receipt):
        target.parent.mkdir(parents=True, exist_ok=True)
        atomic_write(target, after)

    def initialize(self):
        for area in ("tickets", "archive"):
            (self.root / area).mkdir(parents=True, exist_ok=True)
        path = self.root / "config.json"
        created = not path.exists()
        if created:
            atomic_write(path, config.defaults())
        else:
            config.validate(config.read_json(path))
        import rules
        source = Path(__file__).parent
        for name, initial in (("rules.json", config.read_json(source / "rules.json")),
                              ("project-rules.json", {"schema_version": 1, "rules": {}})):
            target = self.root / name
            if not target.exists():
                atomic_write(target, initial)
            rules.validate(config.read_json(target))
        from installation import install_missing
        install_missing(self)
        ignore = self.safe(self.root / ".gitignore")
        existing = ignore.read_text(encoding="utf-8") if ignore.exists() else ""
        missing = [line for line in ("/local/", "__pycache__/", "*.py[cod]") if line not in existing.splitlines()]
        if missing:
            with ignore.open("a", encoding="utf-8") as stream:
                stream.write(("\n" if existing and not existing.endswith("\n") else "") + "\n".join(missing) + "\n")
        return {"initialized": True, "created_config": created, "directory": ".factory2"}

    # ---- reads -------------------------------------------------------------

    def waiting(self, document):
        """Whether the ticket waits for the owner and for what; computed by flow.py without Git."""
        import flow
        actions = flow.Flow(self.project, tickets=self).pending(document)
        return {"waiting_for_human": bool(actions), "pending_human_actions": actions}

    def profile(self, document):
        """The ticket's flow profile name; an unknown one is refused with UNKNOWN_FLOW."""
        import flow
        return flow.Flow(self.project, tickets=self).profile(dict(document, flow=flow.flow_state(document)))[0]

    def projection(self, document):
        # The flow profile: the recorded one, else the default for a flow record, else the proposed one;
        # null for a ticket with neither record.
        flow, proposal = document.get("flow"), document.get("proposal")
        profile = ((flow.get("profile") or "simple") if isinstance(flow, dict)
                   else proposal.get("profile") if isinstance(proposal, dict) else None)
        return dict(effective_metadata(document), **self.waiting(document), profile=profile)

    def read(self, operation, arguments):
        if operation == "status":
            active = sorted(d["ticket_id"] for d in self.documents() if d.get("status") == "in_progress")
            return {"state": "working" if active else "idle", "in_progress": active}
        if operation == "ticket/list":
            archive = bool(arguments.get("archive"))
            return [dict({key: doc.get(key) for key in ("ticket_id", "title", "description", "type", "status", "revision", "created_at")},
                         **self.projection(doc),
                         closed_at=(doc.get("closure") or {}).get("at"),
                         duration_ms=(doc.get("metrics", {}).get("duration_ms")
                                      if any(a.get("status") != "running" and a.get("duration_ms") is not None
                                             for t in doc.get("tasks", []) for a in t.get("attempts", [])) else None),
                         # The Console counts these still-running attempts up to the present.
                         running_since=sorted(a["started_at"] for t in doc.get("tasks", [])
                                              for a in t.get("attempts", [])
                                              if a.get("status") == "running" and a.get("started_at")))
                    for doc in self.documents(archive) if archive or doc.get("status") != "closed"]
        if operation == "ticket/show":
            _, document = self.load(arguments.get("ticket_id"))
            # Each task names the model of its latest attempt that has one recorded (flow log --model).
            tasks = [dict(task, worker=next((a["worker"] for a in reversed(task["attempts"])
                                             if (a.get("worker") or {}).get("model")), None))
                     for task in document["tasks"]]
            return dict(document, **self.projection(document), tasks=tasks,
                        priorityOverride=document.get("priority"), severityOverride=document.get("severity"))
        if operation in ("ticket/journal", "ticket/task-journal"):
            directory, document = self.load(arguments.get("ticket_id"))
            entries = []
            for sequence, line in enumerate(self.journal(directory, document).splitlines(), 1):
                entry = json.loads(line)
                require(isinstance(entry, dict) and entry.get("sequence") == sequence
                        and entry.get("ticket_id") == document["ticket_id"],
                        "JOURNAL_CONFLICT", "The ticket journal contains an invalid record.")
                entries.append(entry)
            recording = "enabled" if "journal" in document else "not_recorded"
            if operation == "ticket/task-journal":
                task_id = identifier(arguments.get("task_id"))
                require(any(t["task_id"] == task_id for t in document["tasks"]),
                        "TASK_NOT_FOUND", "Task does not belong to this ticket.")
                return {"ticket_id": document["ticket_id"], "task_id": task_id, "recording": recording,
                        "items": [e for e in entries if e.get("task_id") == task_id]}
            limit, after = arguments.get("limit", 50), arguments.get("after", "")
            prefix = document["ticket_id"] + "/"
            require(type(limit) is int and 1 <= limit <= 200, "INVALID_ARGUMENT", "limit must be between 1 and 200.")
            require(isinstance(after, str) and (not after or after.startswith(prefix) and after[len(prefix):].isdigit()),
                    "INVALID_ARGUMENT", "Journal cursor must belong to this ticket.")
            start = int(after[len(prefix):]) if after else 0
            require(start <= len(entries), "INVALID_ARGUMENT", "Journal cursor is beyond the last entry.")
            items = entries[start:start + limit]
            return {"ticket_id": document["ticket_id"], "items": items, "recording": recording,
                    "next_cursor": prefix + str(items[-1]["sequence"]) if items else after}
        if operation == "config/show":
            ticket = self.load(arguments["ticket_id"])[1] if "ticket_id" in arguments else None
            return self._effective_config(ticket, arguments.get("paths"))
        if operation == "settings/show":
            return config.settings_snapshot(self, arguments.get("scope", "."))
        if operation in ("policy/show", "policy/select"):
            import policy
            if operation == "policy/show":
                return policy.load(self._effective_config(), arguments.get("provider"))
            _, document = self.load(arguments.get("ticket_id"))
            task = self.task(document, arguments.get("task_id"))
            return policy.select(self._effective_config(document), task, arguments.get("provider"))
        raise FactoryError("UNKNOWN_OPERATION", "Unsupported operation.")

    @staticmethod
    def task(document, task_id):
        identifier(task_id)
        task = next((t for t in document["tasks"] if t["task_id"] == task_id), None)
        require(task is not None, "TASK_NOT_FOUND", "Task does not belong to this ticket.")
        return task

    # ---- mutations ---------------------------------------------------------

    def write(self, operation, arguments, actor="human/owner", request_id=None):
        with self.lock():
            if operation == "settings/update":
                return config.settings_update(self, arguments, request_id, None)
            if operation == "ticket/create":
                before, document, result = None, *self.create(arguments, actor)
                changed = True
            else:
                _, before = self.load(arguments.get("ticket_id"))
                document = copy.deepcopy(before)
                expected = arguments.get("expected_revision")
                field = operation.split("/")[1]
                # Repeating a metadata change that already holds is a success, whatever the revision.
                settled = (field in ("priority", "severity", "type")
                           and (field in arguments or arguments.get("clear") is True)
                           and document.get(field) == arguments.get(field)
                           and (arguments.get(field) is not None or field not in document))
                given = [key for key in ("title", "description", "difficulty") if key in arguments]
                settled = settled or (operation == "ticket/edit" and bool(given)
                                      and all(document.get(key) == arguments[key] for key in given))
                if expected is not None and not settled:
                    require(type(expected) is int and expected == document["revision"],
                            "REVISION_CONFLICT", "The ticket revision has changed; read it again and repeat.")
                handler = self.ticket_command if operation.startswith("ticket/") else self.task_command
                result, changed = handler(operation, document, arguments, actor)
            if changed:
                self.save(before, document, actor, operation, request_id)
                result["revision"] = document["revision"]
            if operation == "ticket/confirm" and not {"priority", "severity"} & set(result["corrected"]):
                # The owner's confirmation alone changes no assignment: it is committed with the next commit.
                changed = False
            # Repeating a closure also commits what an interrupted one left behind.
            if operation in COMMITS and (changed or operation == "ticket/close"):
                try:
                    # Without git/auto_commit the ticket's files stay uncommitted local files.
                    permitted = self._git_settings(document)["auto_commit"]
                    error = self.commit(document["ticket_id"], COMMITS[operation]) if permitted else None
                except FactoryError as problem:
                    error = problem.message
                if error:
                    # The ticket is saved; its files are committed with its next commit, at the latest at closing.
                    result["commit_error"] = error
            return result

    def create(self, arguments, actor):
        import policy
        require(arguments.get("type", "change") in TICKET_TYPES, "INVALID_ARGUMENT", "Ticket type must be bug, change or check.")
        require(arguments.get("severity") in (None, "unknown", *TICKET_SEVERITIES), "INVALID_ARGUMENT", "Invalid severity.")
        require(arguments.get("priority") in (None, *TICKET_PRIORITIES), "INVALID_ARGUMENT", "Invalid priority.")
        require(arguments.get("difficulty", "unknown") in policy.DIFFICULTIES, "INVALID_ARGUMENT", "Invalid difficulty.")
        overrides = arguments.get("config_overrides", {})
        config.validate(overrides)
        related = arguments.get("related_ticket_id")
        if related is not None:
            self.load(related)
        discovered = arguments.get("discovered_at")
        if discovered is not None:
            try:
                stamp = datetime.fromisoformat(discovered.replace("Z", "+00:00"))
                require(stamp.tzinfo is not None, "INVALID_ARGUMENT", "Discovery time needs a timezone.")
                discovered = stamp.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")
            except (ValueError, TypeError, AttributeError) as error:
                raise FactoryError("INVALID_ARGUMENT", "Discovery time must be an ISO timestamp.") from error
        document = {"schema_version": 1, "ticket_id": str(uuid4()), "revision": 0,
                    "title": text(arguments.get("title"), "Title"), "description": arguments.get("description", ""),
                    "type": arguments.get("type", "change"), "discovered_at": discovered, "created_at": now(),
                    "status": "open", "branch": None, "related_ticket_id": related, "config_overrides": overrides,
                    "tasks": [], "history": [], "closure": None}
        require(isinstance(document["description"], str), "INVALID_ARGUMENT", "Description must be text.")
        # An omitted difficulty is not stored and reads as unknown.
        document.update({key: arguments[key] for key in ("priority", "severity", "difficulty") if key in arguments})
        if arguments.get("profile") is not None:
            require(isinstance(arguments["profile"], str), "INVALID_ARGUMENT", "Profile must be a name.")
            document["flow"] = {"profile": arguments["profile"]}
        # Priority, severity and profile are proposals until the owner confirms them with ticket/confirm.
        document["proposal"] = dict(effective_metadata(document), profile=self.profile(document),
                                    at=document["created_at"], actor=actor, confirmed_at=None, confirmed_by=None)
        event(document, "ticket/created", actor, **effective_metadata(document), profile=document["proposal"]["profile"])
        return document, {"ticket_id": document["ticket_id"], "status": "open"}

    def ticket_command(self, operation, document, arguments, actor):
        result = {"ticket_id": document["ticket_id"], "status": document["status"], "revision": document["revision"]}
        if operation in ("ticket/priority", "ticket/severity", "ticket/type"):
            field = operation.split("/")[1]
            allowed = {"type": TICKET_TYPES, "priority": TICKET_PRIORITIES, "severity": TICKET_SEVERITIES}[field]
            require(arguments.get("expected_revision") is not None, "INVALID_ARGUMENT", "A ticket revision is required.")
            require((field in arguments and arguments[field] in allowed and "clear" not in arguments)
                    or (field != "type" and field not in arguments and arguments.get("clear") is True),
                    "INVALID_ARGUMENT", "Choose a supported " + field + ("." if field == "type" else " or clear it."))
            require(document["status"] != "closed", "INVALID_TRANSITION", "A closed ticket cannot change " + field + ".")
            previous, value = document.get(field), arguments.get(field)
            result.update({field: value if value is not None else "low", "field": field, "value": value, "saved": True})
            if previous == value and (value is not None or field not in document):
                return result, False
            if value is None:
                document.pop(field, None)
            else:
                document[field] = value
            details = {"field": field, "previousOverride": previous, "newOverride": value}
            if field == "priority":
                details.update(previous_priority=previous, priority=value)
            if field == "type":
                details = {"previous_type": previous, "type": value}
            event(document, "ticket/" + field + "_changed", actor, **details)
            return result, True
        if operation == "ticket/confirm":
            require(actor.startswith("human/"), "INVALID_ACTOR",
                    "Only the owner confirms; record it with a human/ actor on their instruction.")
            require(arguments.get("priority") in (None, *TICKET_PRIORITIES), "INVALID_ARGUMENT", "Invalid priority.")
            require(arguments.get("severity") in (None, *TICKET_SEVERITIES), "INVALID_ARGUMENT", "Invalid severity.")
            require(isinstance(arguments.get("profile", ""), str), "INVALID_ARGUMENT", "Profile must be a name.")
            require(document["status"] != "closed", "INVALID_TRANSITION", "A closed ticket cannot be confirmed.")
            previous = dict(effective_metadata(document), profile=(document.get("flow") or {}).get("profile") or "simple")
            for field in ("priority", "severity"):
                if arguments.get(field) is not None:
                    document[field] = arguments[field]
            if arguments.get("profile") is not None:
                document["flow"] = dict(document.get("flow") or {}, profile=arguments["profile"])
            confirmed = dict(effective_metadata(document), profile=self.profile(document))
            corrected = [key for key in confirmed if confirmed[key] != previous[key]]
            proposal = document.get("proposal")
            # A ticket without a proposal record was created before proposals existed and counts as confirmed.
            waiting = proposal is not None and proposal.get("confirmed_at") is None
            result.update(confirmed, confirmed=True, corrected=corrected)
            if not waiting and not corrected:
                return result, False
            if waiting:
                proposal.update(confirmed_at=now(), confirmed_by=actor)
            event(document, "ticket/confirmed", actor, **confirmed, corrected=", ".join(corrected) or None,
                  **{"previous_" + key: previous[key] for key in corrected})
            return result, True
        if operation == "ticket/edit":
            import policy
            require(arguments.get("expected_revision") is not None, "INVALID_ARGUMENT", "A ticket revision is required.")
            given = [key for key in ("title", "description", "difficulty") if key in arguments]
            require(bool(given), "INVALID_ARGUMENT", "Give a new title, description or difficulty.")
            if "title" in arguments:
                text(arguments["title"], "Title")
            require(isinstance(arguments.get("description", ""), str), "INVALID_ARGUMENT", "Description must be text.")
            require(arguments.get("difficulty", "unknown") in policy.DIFFICULTIES, "INVALID_ARGUMENT", "Invalid difficulty.")
            require(document["status"] != "closed", "INVALID_TRANSITION", "A closed ticket cannot be edited.")
            changed = [key for key in given if document.get(key) != arguments[key]]
            if "difficulty" in changed:
                # Unfinished tasks that carry the ticket's difficulty follow it; a task given its own keeps it.
                previous = document.get("difficulty", "unknown")
                for task in document["tasks"]:
                    if task["status"] in ("pending", "running") and task.get("difficulty", "unknown") == previous:
                        task["difficulty"] = arguments["difficulty"]
            for key in changed:
                event(document, "ticket/" + key + "_changed", actor,
                      **{"previous_" + key: document.get(key), key: arguments[key]})
                document[key] = arguments[key]
            result.update(title=document["title"], description=document["description"],
                          difficulty=document.get("difficulty", "unknown"), saved=True)
            return result, bool(changed)
        if operation == "ticket/start":
            require(document["status"] != "closed", "INVALID_TRANSITION",
                    "A closed ticket cannot be restarted; create a related ticket.")
            if document["status"] == "in_progress":
                return result, False
            document["status"] = "in_progress"
            event(document, "ticket/started", actor)
        elif operation == "ticket/pause":
            reason = text(arguments.get("reason"), "Pause reason")
            require(document["status"] in ("in_progress", "paused"), "INVALID_TRANSITION", "Only an active ticket can be paused.")
            if document["status"] == "paused":
                return result, False
            for task in document["tasks"]:
                if task["status"] == "running":
                    stop_task(task, "interrupted", reason, actor)
            document["status"] = "paused"
            event(document, "ticket/paused", actor, reason=reason)
        elif operation == "ticket/close":
            reason = text(arguments.get("reason"), "Closure reason")
            resolution = arguments.get("resolution", "unresolved")
            require(resolution in ("resolved", "unresolved"), "INVALID_ARGUMENT", "Invalid closure resolution.")
            if document["status"] == "closed":
                result["closure"] = document["closure"]
                return result, False
            self.finish(document, actor, resolution, reason, mode="manual", verification="not_claimed")
            result["closure"] = document["closure"]
        result["status"] = document["status"]
        return result, True

    @staticmethod
    def finish(document, actor, resolution, reason, **closure):
        """Close in place: unfinished tasks end, the closure is recorded."""
        for task in document["tasks"]:
            if task["status"] in ("pending", "running"):
                stop_task(task, "succeeded" if resolution == "resolved" and task["status"] == "running" else "cancelled",
                          reason, actor)
        document["status"] = "closed"
        document["closure"] = {"actor": actor, "at": now(), "reason": reason, "resolution": resolution, **closure}
        event(document, "ticket/closed", actor, reason=reason, resolution=resolution)

    def task_command(self, operation, document, arguments, actor):
        require(document["status"] == "in_progress", "INVALID_TRANSITION", "Task work requires an in_progress ticket.")
        if operation == "task/create":
            import policy
            import rules
            phase, action = arguments.get("phase"), arguments.get("action")
            require((phase is None) == (action is None), "INVALID_ARGUMENT", "Give both phase and action, or neither.")
            kind = arguments.get("type", policy.task_type(phase, action) if phase else "implementation")
            require(kind in TASK_TYPES, "INVALID_ARGUMENT", "Unknown task type.")
            dependencies = arguments.get("depends_on", [])
            require(isinstance(dependencies, list) and len(set(dependencies)) == len(dependencies),
                    "INVALID_ARGUMENT", "Invalid dependencies.")
            require(set(dependencies) <= {task["task_id"] for task in document["tasks"]},
                    "INVALID_DEPENDENCY", "Dependencies must be existing tasks of this ticket.")
            task = new_task(text(arguments.get("title"), "Task title"), kind, depends_on=dependencies,
                            paths=rules.paths(self.project, arguments.get("paths", [])))
            if phase:
                task.update(phase=phase, action=action, workflow_step=phase + "/" + action)
            for key in ("difficulty", "description"):
                if key in arguments:
                    task[key] = arguments[key]
            document["tasks"].append(task)
        else:
            task = self.task(document, arguments.get("task_id"))
            if operation == "task/start":
                require(task["status"] == "pending", "INVALID_TRANSITION", "Only a pending task can start.")
                done = {item["task_id"] for item in document["tasks"] if item["status"] == "succeeded"}
                require(set(task["depends_on"]) <= done, "DEPENDENCIES_PENDING", "Task dependencies have not succeeded.")
                start_task(task, actor)
            elif operation == "task/finish":
                require(task["status"] == "running", "INVALID_TRANSITION", "Only a running task can finish.")
                status = arguments.get("status")
                require(status in ("succeeded", "failed", "interrupted"), "INVALID_ARGUMENT", "Invalid attempt outcome.")
                summary = text(arguments.get("summary"), "Outcome summary")
                stop_task(task, status, summary, actor)
                task["result"] = {"summary": summary, "source": "human_report", "recorded_by": actor}
            elif operation == "task/retry":
                require(task["status"] in ("failed", "interrupted"), "INVALID_TRANSITION",
                        "Only failed or interrupted tasks can retry.")
                stop_task(task, "pending", text(arguments.get("reason"), "Retry reason"), actor)
                task["result"] = None
            elif operation == "task/cancel":
                require(task["status"] in ("pending", "running"), "INVALID_TRANSITION",
                        "Only pending or running work can be cancelled.")
                stop_task(task, "cancelled", text(arguments.get("reason"), "Cancellation reason"), actor)
        name = {"task/create": "task/created", "task/start": "task/started", "task/finish": "task/finished",
                "task/retry": "task/retry_requested", "task/cancel": "task/cancelled"}[operation]
        event(document, name, actor, task_id=task["task_id"], status=task["status"])
        result = {"ticket_id": document["ticket_id"], "task_id": task["task_id"], "status": task["status"]}
        if task["attempts"]:
            result["attempt_id"] = task["attempts"][-1]["attempt_id"]
        return result, True


READS = {"status", "ticket/list", "ticket/show", "ticket/journal", "ticket/task-journal", "config/show",
         "settings/show", "policy/show", "policy/select"}
WRITES = {"ticket/create", "ticket/start", "ticket/pause", "ticket/close", "ticket/priority", "ticket/severity",
          "ticket/type", "ticket/edit", "ticket/confirm", "task/create", "task/start", "task/finish", "task/retry", "task/cancel", "settings/update"}


def execute(project, operation, arguments, actor="human/owner", request_id=None):
    tickets = Tickets(project)
    if operation == "init":
        return tickets.initialize()
    require(tickets.root.is_dir(), "NOT_INITIALIZED", "Run init in this project first.")
    if operation in READS:
        return tickets.read(operation, arguments)
    require(operation in WRITES, "UNKNOWN_OPERATION", "Unsupported operation.")
    parts = actor.split("/") if isinstance(actor, str) else []
    require(len(parts) >= 2 and parts[0] in ("human", "host") and all(p and p == p.strip() for p in parts)
            and len(actor) <= 200 and ":" not in actor, "INVALID_ACTOR", "Use an actor such as human/owner.")
    return tickets.write(operation, arguments, actor, request_id)
