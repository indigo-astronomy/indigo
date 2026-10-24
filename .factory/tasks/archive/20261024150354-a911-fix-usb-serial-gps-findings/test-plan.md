# Test plan

USB: fake IOKit discovery with real CF values and production translation-unit object; verify capacity one with multi-endpoint fixture only after repair, full-capacity metadata/order, zero/negative capacity, descendant failures/hubs and exactly-once release counters. Do not run old-code memory corruption reproduction. Build only the core library and selected tests; no USB hardware enumeration or drivers.

GPS: isolated fake framework/tool against copied actual script; optional absence succeeds with SKIPs, presence validates items, missing required optional item and mandatory property cause failure, restoration/summary still executed. No network/hardware and no routine indigo_tests sweep.

OmniSim: fake process/readiness/stop or fake killpg/wait; success transfers ownership, readiness exception/interrupt tears down, Popen failure closes output, teardown errors do not silently convert startup failure to success. No real child service/network.

Record exact commands and results in verification.md. Run bash syntax, Python tests, native unit target if relevant, configured check/test/security, findings/decision/Xcode/process validation; clean generated test artifacts. Cross-platform runtime and physical hardware remain unverified unless actually run.
