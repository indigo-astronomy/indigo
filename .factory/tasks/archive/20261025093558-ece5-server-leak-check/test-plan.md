# Test and regression plan

Status: final

The check itself is the test. It was validated by running it with 1 and 3 cycles and by checking every exercise
operation against a server without Valgrind. Each reported leak was confirmed by reading the allocating and releasing
code.
