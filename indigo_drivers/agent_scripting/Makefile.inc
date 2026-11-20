LDFLAGS += -lindigocat

# The object includes the generated file; a dependency of the .c file would not rebuild it, the .c file itself
# has no recipe and keeps its timestamp.
indigo_agent_scripting.o: boot.js.dat

boot.js.dat: boot.js | $(BUILD_BIN)/indigo_hexdump
		$(BUILD_BIN)/indigo_hexdump boot.js boot.js.dat

$(BUILD_BIN)/indigo_hexdump: ../../indigo_tools/indigo_hexdump.c
		$(MAKE) -C ../../indigo_tools $(BUILD_BIN)/indigo_hexdump
