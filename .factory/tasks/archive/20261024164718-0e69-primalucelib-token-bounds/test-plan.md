# Test and regression plan

Status: final

`lookups_stay_within_a_full_token_array` builds `{"res":{"get":{` with 508 `"xx":0` members and a final `"Z":[1]}}}`, which
is exactly 1024 tokens, and looks up `res.get.MISSING` (skips the whole last subtree), `res.get.Z.x` (below an array) and
`res.get.ab` (present). The overflow is only observable under AddressSanitizer, so the unit test is also built with
`-fsanitize=address,undefined` from the library sources. The simulator suites of the three drivers run through
`run_driver_test.py`.
