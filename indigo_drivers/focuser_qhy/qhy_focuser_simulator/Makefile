CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lncurses -lpthread -lm

TARGET  = qhy_focuser_simulator
SRC     = qhy_focuser_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
