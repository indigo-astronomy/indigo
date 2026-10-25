# Task quality review
Verdict: pass
Reviewer and independence: quality reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5). Not independent; a separate pass over the final diff.
Reviewed revision and working-tree state: `task/20261025004416-0995-xml-client-named-request-protocol` at `5aa5660f3` plus the uncommitted diff.

## Scope and method
Read the client request code and the server's `get_properties_handler`, the new unit case and PROTOCOLS.md.

## Quality findings
None. The attributes come before `device` and `name`, which the server needs, because it translates the name with the
client's version as it parses it; the name is then sent untranslated. The plain request already carries the same
attributes, so servers see nothing new. Before the switch arrives a client may still get replies on the legacy protocol
from requests sent earlier on the same connection; that race existed before and is not widened.

## Fix reinspection
V1/V2 in verification.md.

## Quality verdict
Pass.
