# Plan

1. Add standalone public-API unit suite for accepted formats, rejection boundaries, metadata/header growth and retained output. Register in existing unit targets; reproduce baseline failure with sanitizers.
2. Validate/copy RAW header, use bounded size arithmetic; assemble bounded cards in record-aligned temporary header, parse extension within payload/length, then realloc via temporary pointer and encode pixels without aligned input assumptions.
3. Run focused ordinary/sanitizer tests and Factory gates. Document actual behavior in public header; separate same-agent verification, quality and security passes.
4. Wait for post-006 verified master, synchronize/reconcile predecessor metadata/tests/Xcode, rerun final gates; archive evidence and commit. Coordinator alone integrates. No remote operations.
