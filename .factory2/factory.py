#!/usr/bin/env python3
"""JSON command line for tickets, tasks, settings and the ticket flow."""

import sys
sys.dont_write_bytecode = True

import argparse
import json
from pathlib import Path
from uuid import uuid4

from config import read_json
from errors import FactoryError
import factory_platform as platform_ops
import flow
import tickets
from tickets import TASK_TYPES, TICKET_PRIORITIES, TICKET_SEVERITIES, TICKET_TYPES


def parser():
    cli = argparse.ArgumentParser(description="AI Factory 2 — tickets, tasks and the ticket flow")
    cli.add_argument("--project", default=".", help="Project directory (default: current directory)")
    cli.add_argument("--actor", help="Who records the operation (default: human/NAME for the person logged in "
                                      "on this host: Git user.name, then the OS account, else human/owner)")
    cli.add_argument("--request-id", help="Echoed in the response so a caller can match its acknowledgment")
    groups = cli.add_subparsers(dest="group", required=True)
    for name in ("init", "status"):
        groups.add_parser(name)
    ticket = groups.add_parser("ticket").add_subparsers(dest="command", required=True)
    create = ticket.add_parser("create")
    create.add_argument("--title", required=True)
    create.add_argument("--description", default="")
    create.add_argument("--type", choices=["bug", "change", "check"], default="change")
    create.add_argument("--severity", choices=["unknown", "low", "medium", "high", "critical"])
    create.add_argument("--priority", choices=TICKET_PRIORITIES)
    create.add_argument("--discovered-at")
    create.add_argument("--related-ticket-id")
    create.add_argument("--difficulty", choices=["low", "medium", "high", "unknown"],
                        help="Expected difficulty of the work; the recommended model tier depends on it (default unknown)")
    create.add_argument("--overrides", type=Path, help="JSON file containing ticket config overrides")
    create.add_argument("--profile", help="Proposed flow profile: simple, full, full-ui or one from config.json")
    confirm = ticket.add_parser("confirm", help="Record the owner's confirmation or correction of the proposed "
                                                "priority, severity and flow profile")
    confirm.add_argument("ticket_id")
    confirm.add_argument("--expected-revision", type=int)
    confirm.add_argument("--priority", choices=TICKET_PRIORITIES)
    confirm.add_argument("--severity", choices=TICKET_SEVERITIES)
    confirm.add_argument("--profile", help="simple, full, full-ui or a profile from config.json")
    listing = ticket.add_parser("list")
    listing.add_argument("--archive", action="store_true", help="Include closed tickets")
    for field, values in (("priority", TICKET_PRIORITIES), ("severity", ["low", "medium", "high", "critical"])):
        command = ticket.add_parser(field, help="Set or clear an open ticket's " + field)
        command.add_argument("ticket_id")
        command.add_argument("--expected-revision", type=int, required=True)
        change = command.add_mutually_exclusive_group(required=True)
        change.add_argument("--" + field, choices=values)
        change.add_argument("--clear", action="store_true", default=None)
    type_command = ticket.add_parser(
        "type", help="Reclassify an open ticket; a prepared ticket can become a bug only "
                     "when its plan has a regression check with a failure marker")
    type_command.add_argument("ticket_id")
    type_command.add_argument("--expected-revision", type=int, required=True)
    type_command.add_argument("--type", choices=["bug", "change", "check"], required=True)
    edit = ticket.add_parser("edit", help="Change an open ticket's title, description or difficulty")
    edit.add_argument("ticket_id")
    edit.add_argument("--expected-revision", type=int, required=True)
    edit.add_argument("--title")
    edit.add_argument("--description")
    edit.add_argument("--difficulty", choices=["low", "medium", "high", "unknown"])
    journal = ticket.add_parser("journal", help="Read retained ticket and task execution records")
    journal.add_argument("ticket_id")
    journal.add_argument("--limit", type=int, default=50)
    journal.add_argument("--after", default="")
    task_journal = ticket.add_parser(
        "task-journal", help="Read all retained records for one task in sequence order",
        description="Read a task's complete retained journal in one verified ticket-journal pass. "
                    "The task must belong to the active or archived ticket. Returns every record "
                    "whose task_id matches, in ascending sequence order, without pagination or "
                    "ticket-only/sibling-task records. Returns items=[] for a task with no entries; "
                    "legacy tickets without recorded history report recording=not_recorded. "
                    "Missing tickets or tasks are TICKET_NOT_FOUND or TASK_NOT_FOUND errors. "
                    "Missing, changed or untracked journals fail integrity checks; unreadable or "
                    "malformed records remain errors rather than empty results.")
    task_journal.add_argument("ticket_id", help="UUID of the task's active or archived ticket")
    task_journal.add_argument("task_id", help="UUID of a task belonging to that ticket")
    for name in ("show", "start", "pause", "close"):
        command = ticket.add_parser(name)
        command.add_argument("ticket_id")
        if name != "show":
            command.add_argument("--expected-revision", type=int)
        if name in ("pause", "close"):
            command.add_argument("--reason", required=True)
        if name == "close":
            command.add_argument("--resolution", choices=["unresolved", "resolved"], default="unresolved",
                                 help="Owner-declared resolution; does not claim automated verification")
    task = groups.add_parser("task").add_subparsers(dest="command", required=True)
    for name in ("create", "start", "finish", "retry", "cancel"):
        command = task.add_parser(name)
        command.add_argument("ticket_id")
        command.add_argument("--expected-revision", type=int)
        if name == "create":
            command.add_argument("--title", required=True)
            command.add_argument("--type", choices=sorted(TASK_TYPES))
            command.add_argument("--depends-on", action="append", default=[])
            command.add_argument("--phase", choices=["analysis", "design", "mockup", "development", "testing", "documentation"])
            command.add_argument("--action", choices=["plan", "implement", "review"])
            command.add_argument("--difficulty", choices=["low", "medium", "high", "unknown"])
            command.add_argument("--description")
            command.add_argument("--path", dest="paths", action="append", help="Assigned project-relative file; repeat for each file")
        else:
            command.add_argument("task_id")
        if name == "finish":
            command.add_argument("--status", required=True, choices=["succeeded", "failed", "interrupted"])
            command.add_argument("--summary", required=True)
        if name in ("retry", "cancel"):
            command.add_argument("--reason", required=True)
    config = groups.add_parser("config").add_subparsers(dest="command", required=True)
    show = config.add_parser("show")
    show.add_argument("--ticket", dest="ticket_id")
    show.add_argument("--path", dest="paths", action="append", help="Show applicable rules per file; repeat for each file")
    settings = groups.add_parser("settings").add_subparsers(dest="command", required=True)
    for name in ("show", "update"):
        command = settings.add_parser(name)
        command.add_argument("--scope", default=".")
        if name == "update":
            command.add_argument("--resource", required=True, choices=["config", "rules"])
            command.add_argument("--layer", required=True, choices=["project", "local"])
            command.add_argument("--field", required=True)
            choice = command.add_mutually_exclusive_group(required=True)
            choice.add_argument("--value", type=json.loads)
            choice.add_argument("--remove", action="store_true", default=None)
    policies = groups.add_parser("policy").add_subparsers(dest="command", required=True)
    show = policies.add_parser("show")
    show.add_argument("provider", choices=["openai", "anthropic", "deepseek"])
    select = policies.add_parser("select")
    select.add_argument("ticket_id")
    select.add_argument("task_id")
    select.add_argument('--provider', required=True, choices=['openai', 'anthropic', 'deepseek'])
    flow.add_arguments(groups)
    return cli


def main(argv=None):
    args = vars(parser().parse_args(argv))
    project = args.pop("project")
    actor = args.pop("actor")
    request_id = args.pop("request_id") or str(uuid4())
    group = args.pop("group")
    command = args.pop("command", None)
    arguments = {key: value for key, value in args.items() if value is not None}
    if actor is None and (group == "flow" or (group + "/" + command if command else group) not in tickets.READS):
        actor = platform_ops.owner_actor(project)
    try:
        if group == "flow":
            result = flow.run(project, command, arguments, actor)
        else:
            if "overrides" in arguments:
                arguments["config_overrides"] = read_json(arguments.pop("overrides"))
            operation = group + "/" + command if command else group
            result = tickets.execute(project, operation, arguments, actor, request_id)
        print(json.dumps({"ok": True, "request_id": request_id, "result": result},
                         indent=2, ensure_ascii=False, allow_nan=False))
        return 0
    except FactoryError as error:
        output = {"ok": False, "request_id": request_id, "error": {"code": error.code, "message": error.message}}
        print(json.dumps(output, indent=2, ensure_ascii=False), file=sys.stderr)
        return 2
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(json.dumps({"ok": False, "request_id": request_id,
                          "error": {"code": "IO_OR_STATE_ERROR",
                                    "message": "Operation failed (" + type(error).__name__ + "); repeat the command."}}),
              file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
