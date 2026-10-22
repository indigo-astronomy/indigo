# Test and regression plan
AC-001: positive/negative current temperature, zero/default precision, explicit 1/2/5 and capped 9 precision.
AC-002: distinct measured/target values in mixed strings; uppercase NA with cooling off, cooler hidden or temperature hidden.
AC-003: current value with cooling off/hidden; NA when temperature hidden.
AC-004: repeated macros, surrounding filename text and suffix; mixed sequence macro.
Task-local regression.py extracts the complete unchanged sanitize/create_file_name functions from production source, compiles against real public headers and links the existing INDIGO library. It uses synthetic property structures and makes no hardware/network calls or output image writes. TemporaryDirectory cleans compilation products. This avoids exposing a private function or changing build/test infrastructure for a small parser extension. The retained script is reproducible evidence, not a registered unit-suite target. Full hardware/platform testing is unnecessary for this formatting change; Windows/Linux compilation remains unverified.
