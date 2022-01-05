# astap_cli agent

Backend implementation of ASTAP plate solver

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_astap indigo_agent_imager indigo_agent_mount indigo_ccd_... indigo_mount_... 

## Status: Stable

## Notes on agent setup

This is how to configure the agent
1. in ASTAP Agent > Index Management select some indexes fitting field size produced by the camera
2. in ASTAP Agent > Plate Solver select indexes to use and hints for subsequent plate solving
3. in ASTAP Agent > Main select related Imager or Guider agent and optionally Mount Agent
4. if Mount Agent is selected, configure also Sync mode
5. Trigger exposure
