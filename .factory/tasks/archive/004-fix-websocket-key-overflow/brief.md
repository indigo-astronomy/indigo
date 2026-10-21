# Repair brief

Task ID: 004-fix-websocket-key-overflow. Owner: Peter Polakovic. Date: 21 Oct 2026, Europe/Bratislava.
Repair FND-000045, found by audit task 003. Owner request: "Start a scoped repair task for FND-000045: bound the WebSocket key copy in indigo_server_tcp.c, add a server regression test, and reverify with the reproducer."
Scope: indigo_libs/indigo_server_tcp.c, a new server integration test in indigo_test/, its Makefile rules, the Xcode project references for the new files, and this task's evidence plus the findings register. No public API change, no other library behavior, no README, no push or merge.
Base: task/003-initial-repository-audit at f1a28a893, because master does not yet contain the unmerged Factory adoption (001 -> 002 -> 003 -> 004 integration order).
