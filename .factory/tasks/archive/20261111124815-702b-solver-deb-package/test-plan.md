# Test plan

Build both packages, inspect control fields and contents, then exercise install, refusal and takeover in a scratch
`dpkg --root` with the main package repacked without maintainer scripts. Dry-run a Docker target. Results are in
verification.md.
