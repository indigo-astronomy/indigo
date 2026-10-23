# INDIGO makefiles

## Top level makefiles

There are three top level makefiles:

- `Makefile` builds the platform-specific bundle (framework, tools, drivers and server) from the repository root.
- `Makefile.drvs` builds platform drivers when invoked from `indigo_drivers`, `indigo_mac_drivers` or `indigo_linux_drivers` with `-f ../Makefile.drvs`.
- `Makefile.drv` builds a particular driver when invoked from its source directory with `-f ../../Makefile.drv`, unless that driver supplies its own `Makefile`.

The top level `Makefile` generates the root `Makefile.inc`, which component makefiles include for platform detection, compiler/linker flags and build/install paths. Generate it from the repository root with `make Makefile.inc` before invoking component makefiles directly. It contains paths for the current checkout and host; after moving a checkout or changing hosts, force regeneration with `make -B Makefile.inc` even if the existing file appears up to date. Do not edit or copy this generated file between machines. A normal top level build prepares it through the `init` target.

## Second level makefiles

Component makefiles (for example, in `indigo_libs`, `indigo_tools` and `indigo_server`) must run in their own directories, with the root `Makefile.inc` already present. For example, `make -C indigo_tools all` builds the tools after the framework and its dependencies are available.

## Third level makefiles

`Makefile.drv` detects the usual driver components in their standard locations: source files, rules, firmware, and SDKs under `externals` or `bin_externals`. To customize a driver build, provide a driver-local `Makefile` or a `Makefile.inc` included by the shared rules.

### Generated drivers

When a driver has a `.driver` definition, `Makefile.drv` invokes `$(BUILD_BIN)/indigo_generator` if the definition or `indigo_tools/indigo_generator.c` is newer than its generated `.c`/`.cpp` source. One generator invocation writes the source, header and `_main.c` stub; the header and stub depend on the generated source so parallel builds do not run competing generators.

The generator binary is an order-only prerequisite with its own build rule. If it is missing or older than its source, the driver build recursively invokes `indigo_tools` to build it before regeneration. Rebuilding the binary alone does not force every driver to regenerate. The root `Makefile.inc`, build directories and compiler prerequisites must already be prepared.

For a manual generator invocation, first run `make init` from the repository root, then build the generator explicitly:

```sh
make -C indigo_tools "$(pwd)/build/bin/indigo_generator"
```

Edit the `.driver` source and regenerate its outputs; do not hand-edit generated files. See [DRIVER_GENERATOR_MIGRATION.md](DRIVER_GENERATOR_MIGRATION.md) for manual invocation and migration instructions.
