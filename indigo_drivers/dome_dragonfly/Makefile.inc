# The generated driver source includes the shared Dragonfly implementation, so
# the object file has to be rebuilt when that implementation changes.
indigo_dome_dragonfly.o: ../aux_dragonfly/shared/dragonfly_shared.h ../aux_dragonfly/shared/dragonfly_shared.c

# Makefile.inc is included before Makefile.drv defines its own rules, so the
# rule above would otherwise become the default goal of a bare make run.
.DEFAULT_GOAL := all
