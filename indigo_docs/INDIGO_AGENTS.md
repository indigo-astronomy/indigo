# INDIGO Agents
Revision: 22.12.2023 (early draft)

Authors: **Rumen G. Bogdanovski** & **Peter Polakovic**

e-mail: *rumenastro@gmail.com*

## What are Agents?

Even if INDIGO has always been well prepared for a distributed computing, for many situations the traditional
client/server approach is not the best one. Sometimes part of the client logic is needed on the server.
A good example of this is the implementation of webGUI in INDIGO server. The thin client running in a browser needs much smarter backend than INDIGO server itself, it needs a server side application logic. For this code, acting on the server side and controlling the devices on behalf of the client, we use term INDIGO **agent**.

One can think of **agents** as higher level drivers that operate in the layer between the device drivers and the client or between different device drivers. Agents are independent on the connection to the client. The client can configure it, disconnect and the **agent** will keep running, doing its job. The client can connect later to monitor the status or to get the results. The communication between the driver and the **agent** is also not limited by network bandwidth as far as they may both live in the same INDIGO server and can communicate to each other over the software bus at high speed. However agents can communicate with drivers running on different servers and agents can operate on the client itself. They are quite flexible depending on the use case.

Agent code contains the most of the application logic for the typical operations supported by the applications. This makes the implementation of the client pretty straightforward. The client only needs to implement native GUI for a given operating system. It is faster, easier and possible bugs can be fixed in one place - the agent.

## Basic INDIGO Agents

- **Imager Agent** controls main imaging cameras, filter wheels and focusers. It can capture a given number of images in a single batch or in a more complicated sequence, focus automatically, generate stretched JPEG previews or upload captured images to the client. The agent name is "*indigo_agent_imager*".

- **Mount Agent** controls mounts, domes, GPS units and joysticks. It provides a common source of information about observatory position and source of the local time, creates links between the joysticks and the mounts and also acts as a gateway between INDIGO protocol and LX200 protocol for 3rd party applications like SkySafari. The agent name is "*indigo_agent_mount*".

- **Guider Agent** controls guiding cameras and guiding devices. It can calibrate and guide with a different drift detection algorithms, it generates stats and data for different kinds of graphs. In future it will also control active optic devices. The agent name is "*indigo_agent_guider*". The agent's [README.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_guider/README.md) contains useful information how to setup your guiding.

- **Auxiliary Agent** controls auxiliary devices like power boxes, flat boxes, weather stations, sky quality meters, etc. The agent name is "*indigo_agent_auxiliary*".

- **Scripting Agent** manages and executes INDIGO scripts. The agent name is "*indigo_agent_scripting*". More on INDIGO scripting can be found in the agent's [README.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_scripting/README.md) and in [Basics of INDIGO Scripting](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/SCRIPTING_BASICS.md)

- **Snoop Agent** is a special agent that enables the communication between the device drivers. Device drivers can not communicate between each other natively. To make it possible the **Snoop Agent** is used. For example The Mount can synchronize the time and the geographical coordinates from the GPS using the **Snoop Agent**. The agent name is "*indigo_agent_snoop*".

- **Astrometry Agent** solves images using Astrometry.net, manages indexes and syncs the current position to the mount. It is also responsible for the mount polar alignment. The agent name is "*indigo_agent_astrometry*"

- **ATAP Agent** *(deprocated)* solves images using ASTAP, manages indexes and syncs the current position to the mount. It is also responsible for the mount polar alignment. The agent name is "*indigo_agent_astap*"

- **Configuration Agent** is responsible for managing the system configuration. The agent's [README.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_config/README.md) contains useful information. The agent name is "*indigo_agent_config*".


**Agents** can also talk to each other. E.g. **Imager Agent** can initiate dithering in **Guider Agent** or to sync coordinates in **Mount Agent** to the center of a plate solved image. **Mount Agent** can set FITS metadata in **Imager Agent** or to stop guiding upon slew or parking request. Such agents we refer as related agents.

## Device Names in Agents

 Client applications, which are just a GUI of the agent, show the device names as seen by the agent. So if e.g. AstroTelescope is connected to its embedded **Imager Agent** it shows "*Mount SynScan*" connected directly to your Mac as "*Mount SynScan*", while the same mount connected to INDIGO Sky will be named "*Mount SynScan @ indigosky*". On the other hand, if AstroTelescope is connected to the **Imager Agent** running server side on INDIGO Sky, will not show mount connected to your Mac (because INDIGO Sky is not configured to connect remote devices) and the mount connected to Raspberry Pi will be called just "*Mount SynScan*". Always keep in mind, that names shown by GUI are relative to the selected agent.

The same is true for related agents. **Imager Agent** running in INDIGO Sky as seen from **Mount Agent** running in the same Raspberry Pi is named "*Imager Agent*", while as seen from the **Mount Agent** embedded into AstroTelescope is named "*Imager Agent @ indigosky*". This is what you will see in any control panel or a similar tool.

## Notes About Agents

- Agents are loaded and unloaded as regular drivers. Please see [INDIGO_SERVER_AND_DRIVERS_GUIDE.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/INDIGO_SERVER_AND_DRIVERS_GUIDE.md) and [PROPERTY_MANIPULATION.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROPERTY_MANIPULATION.md)

- Agents can be statically linked in the client, just like the drivers.
