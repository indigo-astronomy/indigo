CC     ?= gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lpthread

TARGET  = aux_svbpowerbox_simulator
SRC     = aux_svbpowerbox_simulator.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
