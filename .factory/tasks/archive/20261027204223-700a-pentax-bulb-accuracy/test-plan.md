# Test plan and scope

Preparation: published source identity, driver/archive freshness, harness build, shell syntax, EXIF analyzer positive/negative checks against retained JPEG. Hardware: two timing samples with50ms diagnostic threshold, then owner-requested canonical full and distinct bulb/RAW profiles. Preserve full failure and timing failure, validate image transfers/LibRaw unpack, verify settings restoration. Images outside Git.

Documentation: compare README setup/range/model/platform/format/feature statements with actual production source and hardware evidence. Keep timing finding open, no implied calibration or full-class certification. Run configured Factory check/test/security and independent quality/security passes after current master synchronization. No driver regression rerun needed because production and harness source are unchanged.
