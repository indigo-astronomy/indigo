# Test and regression plan

Status: final

`request_versus_poll` reproduces the race deterministically: the poll that sees an external move answers 400 ms late, the
request is sent once that poll is in the journal, so it is accepted while the poll is in flight and its handler is queued
behind it. The full simulator suite runs through `run_driver_test.py aux_alto`.
