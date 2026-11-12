LDFLAGS += -lindigocat

indigo_agent_scheduler.c: Sequencer.js.dat

Sequencer.js.dat: ../agent_scripting/library/Sequencer.js
		hexdump -v -e '1/1 "0x%02x, "' ../agent_scripting/library/Sequencer.js >Sequencer.js.dat
