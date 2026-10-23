# Audit checks

Safe baseline: make -C indigo_test test-unit (framework unit suites), make -C indigo_test test-buildsystem, Factory check/test/security, syntax/JSON/XML/Xcode checks and local compiler static analysis where compilation inputs are available. Record actual counts, not command exit alone.
No driver/hardware test mode is inferred. Any diagnostic harness uses temporary synthetic local files/loopback state and never real user configuration/devices. Test cleanup must not remove pre-existing build resources or other processes. Sanitizer reproductions, when needed, are outside the repository with exact retained source/commands in audit evidence. Zero tests or unavailable SDK/platform is not coverage.
