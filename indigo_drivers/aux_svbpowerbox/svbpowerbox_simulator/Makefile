CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lncurses -lpthread -lm

TARGET  = svbpowerbox_simulator
SRC     = svbpowerbox_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
