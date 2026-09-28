# The generated driver source includes the shared Dragonfly implementation, so
# the object file has to be rebuilt when that implementation changes.
indigo_aux_dragonfly.o: shared/dragonfly_shared.h shared/dragonfly_shared.c

# Makefile.inc is included before Makefile.drv defines its own rules, so the
# rule above would otherwise become the default goal of a bare make run.
.DEFAULT_GOAL := all
