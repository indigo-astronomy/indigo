# Scheduler agent

Selects and runs imaging tasks by sky constraints and priority, and lets urgent tasks interrupt the running one. Each task is a text with a `// @key value` header followed by a `Sequencer.js` body, which the agent runs in the Scripting Agent. The design is described in [SCHEDULER.md](../../indigo_docs/SCHEDULER.md) and the properties in [PROPERTIES.md](../../indigo_docs/PROPERTIES.md).

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_scripting indigo_agent_scheduler ...

## Status: Under development

## Notes

- The agent needs the Scripting Agent. It loads `Sequencer.js` into it when the library is not loaded yet.
- Task bodies run with the default agents of `Sequence.start()`: "Imager Agent", "Mount Agent" and "Guider Agent". The site coordinates for the sky constraints come from the Mount Agent.
- Hooks, the night plan preview, the run history, duration estimates and safety handling are not implemented yet.

## Testing

- 2026-11-13 00:10 3.0.0.3 linux arm64 simulator 36/36 OK
