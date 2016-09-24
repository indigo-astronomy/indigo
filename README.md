INDIGO is a proof-of-concept of future INDI based on layered architecture and software bus.

This is the list of requirements taken into the consideration:

1. No GPL/LGPL dependency to allow commercial use under application stores licenses.
2. ANSI C for portability and to allow simple wrapping into .Net, Java, GOlang, Objective-C or Swift in future
3. Layered architecture to allow direct linking of the drivers into applications and or to replace wire protocol.

To build PoC, use

make all

and then execute either (client and driver in single executable with direct communication over the bus)

test

or (client executing driver as child proces with the legacy INDI wire protocol)

client driver
