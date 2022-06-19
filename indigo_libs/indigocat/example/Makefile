CC       = gcc
CFLAGS   = -I../../indigo/ -I../../ -L../../../build/lib -g -O3 -Wall -Wextra
LDFLAGS  = -lindigocat -lm

TARGET = planets_test

SOURCES = planets_test.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${TARGET} ${OBJECTS}
