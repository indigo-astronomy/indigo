# INDIGO Agents
Revision: 04.08.2020 (early draft)

Authors: **Rumen G.Bogdanovski** & **Peter Polakovic**

e-mail: *rumen@skyarchive.org*

## What are Agents?

Even if INDIGO has always been well prepared for a distributed computing, for many situations the traditional
client/server approach is not the best one. Sometimes part of the client logic is needed on the server.
A good example of this is the implementation of webGUI in INDIGO server. The thin client running in a browser needs much smarter backend than INDIGO server itself, it needs a server side application logic. For this code, acting on the server side and controlling the devices on behalf of the client, we use term INDIGO **agent**.

One can think of **agents** as higher level drivers that operate in the layer between the device drivers and the client. Agents are independent on the connection to the client. The client can configure it, disconnect and the **agent** will keep running, doing its job. The client can connect later to monitor the status or to get the results. The communication between the driver and the **agent** is also not limited by network bandwidth as far as they may both live in the same INDIGO server and can communicate to each other over the software bus at high speed. However agents can communicate with drivers running on different servers and agents can operate on the client itself. They are quite flexible depending on the use case.

Agent code contains the most of the application logic for the typical operations supported by the applications. This makes the implementation of the client pretty straightforward. The client only needs to implement native GUI for a given operating system. It is faster, easier and possible bugs can be fixed in one place - the agent.

## Basic INDIGO Agents

There are five basic agents - Imager Agent, Mount Agent, Guider Agent, Auxiliary Agent and Snoop Agent.

- **Imager Agent** controls main imaging cameras, filter wheels and focusers. It can capture a given number of images in a single batch or in a more complicated sequence, focus automatically, generate stretched JPEG previews or upload captured images to the client.

- **Mount Agent** controls mounts, domes, GPS units and joysticks. It provides a common source of information about observatory position and source of the local time, creates links between the joysticks and the mounts and also acts as a gateway between INDIGO protocol and LX200 protocol for 3rd party applications like SkySafari.

- **Guider Agent** controls guiding cameras and guiding devices. It can calibrate and guide with a different drift detection algorithms, it generates stats and data for different kinds of graphs. In future it will also control active optic devices.

- **Auxiliary Agent** controls auxiliary devices like power boxes, flat boxes, weather stations, sky quality meters, etc.

- **Snoop Agent** is a special agent that enables the communication between the device drivers. Device drivers can not communicate between each other natively. To make it possible the **Snoop Agent** is used. For example The Mount can synchronize the time and the geographical coordinates from the GPS using the **Snoop Agent**.

**Agents** can also talk to each other. E.g. **Imager Agent** can initiate dithering in **Guider Agent** or to sync coordinates in **Mount Agent** to the center of a plate solved image. **Mount Agent** can set FITS metadata in **Imager Agent** or to stop guiding upon slew or parking request. Such agents we refer as related agents.


## Auxiliary Notes

- Agents are loaded and unloaded as regular drivers. (see TBD)

- (FIXME) Since version 4.0 AstroImager and AstroDSLR are clients for Imager Agent, AstroTelescope is a client for Mount Agent and AstroGuider is a client for Guider Agent. Most of the applications are also clients for Auxiliary Agent. These agents are referred as primary agents.

- (FIXME) Client application, which is just a GUI of the agent, shows the device names as seen by the agent. So if e.g. AstroTelescope is connected to its embedded Imager Agent it shows SynScan mount connected directly to your Mac as "Mount SynScan", while the same mount connected to INDIGO Sky will be named "Mount SynScan @ indigosky". On the other hand, if AstroTelescope is connected to the Imager Agent running inside INDIGO Sky, it will not show mount connected to your Mac (because INDIGO Sky is not configured to connect remote devices) and the mount connected to Raspberry Pi will be called just "Mount SynScan". Always keep in mind, that names shown by GUI are relative to the selected primary agent.

- (FIXME) Technically the same is true for related agents. So Imager Agent running in INDIGO Sky as seen from Mount Agent running in the same Raspberry Pi is named "Imager Agent", while as seen from Mount Agent embedded into AstroTelescope is named "Imager Agent @ indigosky". This is what you will see in any control panel or a similar tool. But to make it less confusing, the recent builds of client applications will show just user friendly service names for well known agents, so e.g. "INDIGO Sky" for "AstroTelescope @ indigosky" as a primary agent and "INDIGO Sky" for "Imager Agent @ indigosky" or "AstroImager" for "Imager Agent @ AstroImager" as a related agent.
