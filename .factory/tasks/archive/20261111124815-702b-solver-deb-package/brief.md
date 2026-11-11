# Task brief

- Task ID: 20261111124815-702b-solver-deb-package
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-11

## Problem and desired outcome
The INDIGO plate solver command line tool (`indigo_solver`) is only shipped inside the full `indigo3` Debian package.
Users who want only the solver need a second, standalone Debian package `indigo3-solver` built by `make`.

## Users and use cases
Users and reviewers who want to install and evaluate the INDIGO solver without the whole framework and its services.

## Scope and non-goals
In scope: a `package-solver` make target, the package relationships with `indigo3`, the Docker deb build script and
`clean-all`. Not in scope: `indigo_solver_indexer` (links libindigo/libindigocat, not standalone), apt repository
publishing, macOS/Windows packaging.

## Constraints and references
Owner requirements (conversation 2026-11-11): package name `indigo3-solver`; same version as `indigo3`; it must not
install while `indigo3` is installed; `indigo3` replaces it. Existing packaging: root `Makefile` `package` target and
`tools/build_debs.sh`.

## Initial questions
Whether to include `indigo_solver_indexer`: proposed to exclude, owner approved the plan without adding it.
