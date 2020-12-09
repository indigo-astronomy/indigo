indigo_agent_scripting.c: boot.js
		hexdump -v -e '1/1 "0x%02x, "' boot.js >boot.js.dat
