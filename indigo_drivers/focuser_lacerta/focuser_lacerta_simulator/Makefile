CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lm

TARGET  = focuser_lacerta_simulator
SRC     = focuser_lacerta_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) ../../../indigo_test/simulator_common/serial_simulator_common.h ../../../indigo_test/simulator_common/serial_motion.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
