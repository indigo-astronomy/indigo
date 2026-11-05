include ../../Makefile.drv

MCP_HELP_SOURCES = $(wildcard ../../indigo_docs/mcp/*.md ../../indigo_docs/mcp/agents/*.md) ../agent_scripting/library/Sequencer.md

indigo_agent_mcp.o: indigo_agent_mcp_help.h

indigo_agent_mcp_help.h: generate_help.py $(MCP_HELP_SOURCES)
	python3 generate_help.py
