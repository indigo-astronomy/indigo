# Draft embedded MCP operating manuals

Owner asks to design and show the documents before implementation. Produce English, source-grounded, offline-ready draft manuals for every in-tree agent, including its complete user-visible function inventory. Agent-first workflow: discover/load agents through Server.DRIVERS, discover/load hardware drivers, configure serial ports/model-specific prerequisites, select devices in agents, configure and start agent processes, observe results/abort/recovery. Direct device operations are for setup/service tasks, not normal acquisition/control workflows. Labels alone are not semantic documentation.

Scope: new indigo_docs/mcp/ manuals and current task evidence only. No runtime MCP methods/embedding/source/framework/build/Xcode/property changes, no existing driver README edits, no simulator/hardware/client operations, no commit/push/integration. Distinguish draft content from features already delivered. GitHub/runtime documentation presence cannot be assumed; files are designed to be embedded later.

Base: committed MCP323a560ff37d036c34bd7984a5e38abed9036c6a, which contains latest configuredmain67a7e06. Dependent documentation task branches directly from this verified prerequisite to keep originalcheckout/Xcode source stable; no mainintegration or worktree. Maintain prior feature branch.

## Authorized embedding continuation

Owner now requests embedding the reviewed manuals into MCP so they work without installed documentation or Internet; explicitly no commit. Continue this active task/branch/original checkout. Runtime scope expands to MCP agent source/generated help/generator/local build support, its Windows/Xcode header registration if required, targeted test integration/dependencies and manual/README updates. No framework/bus/property changes, unrelated source fixes, Sequencer execution embedding or hardware use. Previous draft review is retained but does not approve the new code.

## Owner-authorized automatic service lifecycle

Owner finds the extra MCP CONNECTION switch redundant: loading the agent must activate the service without a second explicit enable. Remove its exposed connection control; unloading stops/drains it. Continue the same active task/branch, no commit, no changes to the running user server. Update MCP source/version, visible-property reference, embedded manuals and targeted tests. Existing native bus/auth/HTTP admission bounds stay unchanged.

## Authorized delivery

On 2026-11-05 the owner explicitly requested committing current work, merging to the main integration branch and pushing everything pending. The configured integration branch is master. This supersedes earlier no-commit/no-integration/no-push restrictions. The owner explicitly accepted deferring the due incremental repository audit until after this delivery; its checkpoint remains due before unrelated work. No audit completion or security risk acceptance is claimed.
