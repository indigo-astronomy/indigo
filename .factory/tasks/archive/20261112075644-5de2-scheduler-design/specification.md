# Specification

Status: approved by owner (plan approval, 2026-11-12)

## Problem and outcome
Provide a reviewed design document for an INDIGO Scheduler Agent: `indigo_docs/SCHEDULER.md`.

## In scope
- Survey of existing schedulers and adopted features.
- Architecture (C Scheduler Agent running task bodies in the Scripting Agent).
- Task text format (header grammar, validation), task states, agent properties, persistence, selection, urgent preemption, running a task, phases and testing.
- Xcode project reference for the new document.

## Out of scope
Implementation, `indigo_names.h`, `PROPERTIES.md` (no properties exist yet), web UI.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_docs/SCHEDULER.md` exists and covers survey, architecture, task format, states, properties, persistence, selection, urgent preemption, task execution, phases and testing | Inspection |
| AC-002 | The document reflects every owner decision listed in brief.md | Inspection against brief.md |
| AC-003 | Claims about existing code (APIs, properties, functions) match the current source | Independent review against source |
| AC-004 | The document is referenced in the `indigo_docs` group of `indigo.xcodeproj/project.pbxproj` with a unique object ID | `git diff`, ID uniqueness grep |

## Failure behavior and edge cases
N/A for a document; design-level failure behavior is part of the document content.

## Permissions, data, and security requirements
The document specifies validation of untrusted task text; no secrets or private data.

## Compatibility and nonfunctional requirements
English, repository Markdown style, LF line endings.

## Assumptions and open questions
Property names and defaults may change during implementation; the document says so.

## Owner decisions and requirement changes
See brief.md "Constraints and references" (2026-11-11/12, Rumen G. Bogdanovski).
