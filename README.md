### INDIGO is a proof-of-concept of future INDI based on layered architecture and software bus.

This is the list of requirements taken into the consideration:

1. No GPL/LGPL dependency to allow commercial use under application stores licenses.
2. ANSI C for portability and to allow simple wrapping into .Net, Java, GOlang, Objective-C or Swift in future
3. Layered architecture to allow direct linking of the drivers into applications and or to replace wire protocol.
4. Atomic approach to device drivers. E.g. if camera has imaging and guding chip, driver should expose two independent simple devices instead of one complex. It is much easier and transparent for client.
5. Drivers should support hot-plug at least for USB devices. If device is connected/disconnected while driver is running, its properties should appear/disappear on the bus.

------------------------------------------------------------------------------------------------

To build PoC, use

`git clone https://github.com/indigo-astronomy/indigo.git`

`cd indigo`

`git submodule update --init --recursive`

`make all`

and then execute either (client and driver in single executable with direct communication over the bus)

`./test`

or (client executing driver as child proces with the legacy INDI wire protocol)

`./client`

or (network server, which can be accessed by some INDI control panel)

`./server`
