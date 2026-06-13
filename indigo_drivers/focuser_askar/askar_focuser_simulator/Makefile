CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lncurses -lpthread -lm

TARGET  = askar_focuser_simulator
SRC     = askar_focuser_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
