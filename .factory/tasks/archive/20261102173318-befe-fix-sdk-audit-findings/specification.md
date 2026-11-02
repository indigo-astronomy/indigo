# Specification

Atik2: rejected replacement relay writes preserve the pending original stop at its existing deadline, for either axis and with the other axis active. Successful replacement retains existing behavior. Pentax: full device registry must acquire no camera ownership/resources and retain existing registered devices; later free-slot arrival attaches normally. No properties change.

Baseline audit: Atik2 generated version18 uses serialized queue and accept_while_busy; cancellation precedes fallible SDK write. Pentax hand-written version0x03000005 allocates/refers USB before slot search. Existing fake SDK and USB suites cover healthy/error/lifecycle cases but lack these paths. Hardware testing: no hardware is authorized or operated. Host is macOS arm64; Linux/Windows unavailable.
