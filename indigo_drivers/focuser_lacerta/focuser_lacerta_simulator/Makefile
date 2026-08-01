CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lpthread

TARGET  = focuser_lacerta_simulator
SRC     = focuser_lacerta_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
