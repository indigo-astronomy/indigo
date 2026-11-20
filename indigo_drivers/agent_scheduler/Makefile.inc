LDFLAGS += -lindigocat

# The object includes the generated file; a dependency of the .c file would not rebuild it, the .c file itself
# has no recipe and keeps its timestamp.
indigo_agent_scheduler.o: Sequencer.js.dat

# Sequencer.js.dat is not checked in; it is generated here whenever it is missing or older than Sequencer.js.
# The tool is an order-only prerequisite, so a rebuilt tool does not by itself regenerate the file.
Sequencer.js.dat: ../agent_scripting/library/Sequencer.js | $(BUILD_BIN)/indigo_hexdump
		$(BUILD_BIN)/indigo_hexdump ../agent_scripting/library/Sequencer.js Sequencer.js.dat

$(BUILD_BIN)/indigo_hexdump: ../../indigo_tools/indigo_hexdump.c
		$(MAKE) -C ../../indigo_tools $(BUILD_BIN)/indigo_hexdump
