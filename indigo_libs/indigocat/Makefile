CC       = gcc
CFLAGS   = -I. -fPIC -g -O3 -Wall -Wextra
LDFLAGS  = -shared -lm

TARGET_LIB  = libindigocat.so

SOURCES = dynamical_time.c earth.c jupiter.c mars.c mercury.c moon.c neptune.c nutation.c pluto.c saturn.c sun.c transform.c uranus.c venus.c vsop87.c
HEADERS = dynamical_time.h nutation.h solar_system.h transform.h vsop87.h
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all
all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_LIB) $(OBJECTS)

.PHONY: clean
clean:
	rm -f ${TARGET_LIB} ${OBJECTS}
