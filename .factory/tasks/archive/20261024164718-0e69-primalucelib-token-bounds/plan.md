# Development plan

Status: final

1. Unit case with a 1024-token reply; ASan run against the unchanged library (overflow in `primalucelib_find_token`).
2. Store the count in `primalucelib_link`, bound every index in `primalucelib_find_token`; ASan clean.
3. Driver versions, regenerated output, recorded runs of the three drivers, REFACTOR.md, PROTOCOL.md.
