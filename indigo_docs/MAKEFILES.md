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

### Embedded files

Files compiled into C arrays with `#include` are converted by `$(BUILD_BIN)/indigo_hexdump` (source `indigo_tools/indigo_hexdump.c`), not by the system `hexdump`, which Windows lacks. `indigo_hexdump [INPUT [OUTPUT]]` writes every byte as `0x%02x, ` on one line, without a header or a trailing newline; omitted arguments mean standard input and output, so `gzip <file | indigo_hexdump` works too. The tool depends on the C library only, is built by `make -C indigo_tools all` and is not installed. The embedded JavaScript sources `boot.js` and `library/Sequencer.js` in `indigo_drivers/agent_scripting` carry `eol=lf` in `.gitattributes`, so a Windows checkout with `core.autocrlf=true` embeds the same bytes as macOS and Linux.

- `indigo_drivers/agent_scheduler/Makefile.inc` generates `Sequencer.js.dat` from `indigo_drivers/agent_scripting/library/Sequencer.js` whenever it is missing or older; the file is not checked in.
- `indigo_drivers/agent_scripting/Makefile.inc` regenerates the checked-in `boot.js.dat` when `boot.js` changes.
- `indigo_server/Makefile` gzips the control panel resources and converts them into `resource/*.data`.

The object that includes a generated file depends on it (`indigo_agent_scheduler.o`, `indigo_agent_scripting.o`, `indigo_server.o`), so a regenerated file recompiles the object and rebuilds the archive and shared library; a prerequisite on the `.c` file would not, because the source itself is never remade. `indigo_test/buildsystem/test_embedded_file_dependencies.py` checks these dependencies.

As with the generator, the tool is an order-only prerequisite with its own rule: these makefiles build it through `indigo_tools` when it is missing or older than its source, so standalone driver and server builds work once the root `Makefile.inc` and build directories exist. The Xcode project builds the `indigo_hexdump` target before the `indigo` and `indigo_m1` libraries. In `indigo_windows.sln` the `indigo_hexdump` project is built first, always for x64 so that it also runs on an x64 host building ARM64, and the pre-build steps of `indigo_agent_scheduler` and `indigo_agent_scripting` run it.

The MCP help bundle `indigo_drivers/agent_mcp/indigo_agent_mcp_help.h` is generated differently: `indigo_drivers/agent_mcp/generate_help.py` (Python 3) builds it from `indigo_docs/mcp/*.md`, `indigo_docs/mcp/agents/*.md` and `indigo_drivers/agent_scripting/library/Sequencer.md`, and the result is checked in. The driver `Makefile` and the `test-agent-mcp` targets regenerate it when a source is newer; the Xcode project and `indigo_windows.sln` compile the checked-in copy and have no step that regenerates it. Regenerate it with `python3 indigo_drivers/agent_mcp/generate_help.py` and commit it together with every change of its sources; `indigo_test/buildsystem/test_agent_mcp_help.py`, run by `make -C indigo_test test-buildsystem`, runs `generate_help.py --check` and fails while the checked-in copy is stale.
