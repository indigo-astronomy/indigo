# Specification

AC-001: Native queued define/redefine/update events own immutable deep snapshots, including long text, until dispatch. Later cache mutation or allocation changes cannot alter or invalidate queued data. Release copies after synchronous publication and reclaim undispatched copies after queue shutdown.

AC-002: Each Sequencer save replaces the prior saved batch with an independent copy of all own numeric item values. Public focus success and ignored failure restore original COUNT, EXPOSURE and additional batch settings after the live boot cache changes.

AC-003: Preserve property schemas, permissions, driver portability, existing scripting behavior and test coverage. Increment the handwritten driver's version. No hardware, generator change, remote service, publication or main integration is in scope.

Complexity is high because lifetime ownership crosses the scripting mutex, asynchronous queue and shutdown. Risk is medium: native memory safety and functional restoration. Existing host/model retained; no budget or additional provider use authorized.
