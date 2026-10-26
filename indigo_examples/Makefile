DEBUG_BUILD = -g
INDIGO_ROOT = ..
BUILD_ROOT = $(INDIGO_ROOT)/build
BUILD_LIB = $(BUILD_ROOT)/lib

ifeq ($(OS),Windows_NT)
	OS_DETECTED = Windows
else
	OS_DETECTED = $(shell uname -s)
	ifeq ($(OS_DETECTED),Darwin)
		CFLAGS = $(DEBUG_BUILD) -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_libs -std=gnu11 -DINDIGO_MACOS -pthread
		LDFLAGS = -L$(BUILD_LIB) -lindigo -pthread
	endif
	ifeq ($(OS_DETECTED),Linux)
		CFLAGS = $(DEBUG_BUILD) -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_libs -std=gnu11 -DINDIGO_LINUX -pthread
		LDFLAGS = -L$(BUILD_LIB) -lindigo -pthread
	endif
endif

all: client executable_driver_client dynamic_driver_client remote_server_client remote_server_client_mount service_discovery

client: client.c example_process.h
	$(CC) -o $@ client.c $(CFLAGS) $(LDFLAGS)

executable_driver_client: executable_driver_client.c example_process.h example_image.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

dynamic_driver_client: dynamic_driver_client.c example_image.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

remote_server_client: remote_server_client.c example_image.h example_remote.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

remote_server_client_mount: remote_server_client_mount.c example_remote.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

service_discovery: service_discovery.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

.PHONY: all clean

clean:
	rm -f client executable_driver_client dynamic_driver_client remote_server_client remote_server_client_mount service_discovery servce_discovery
