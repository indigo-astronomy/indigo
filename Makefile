CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iindigo_drivers/ccd_simulator -std=c99
AR=ar
ARFLAGS=-rv

all: test driver client server

libindigo.a:\
	indigo_bus/indigo_bus.o\
	indigo_bus/indigo_xml.o\
	indigo_bus/indigo_server_xml.o\
	indigo_bus/indigo_driver_xml.o\
	indigo_bus/indigo_client_xml.o\
	indigo_bus/indigo_timer.o\
	indigo_drivers/indigo_driver.o\
	indigo_drivers/indigo_ccd_driver.o
	$(AR) $(ARFLAGS) $@ $^

libindigosim.a: indigo_drivers/ccd_simulator/ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

test: indigo_test/test.o libindigo.a libindigosim.a
	$(CC) $(CFLAGS) -o $@ $^

driver: indigo_test/driver.o libindigo.a libindigosim.a
	$(CC) $(CFLAGS) -o $@ $^

client: indigo_test/client.o libindigo.a
	$(CC) $(CFLAGS) -o $@ $^

server: indigo_test/server.o libindigo.a libindigosim.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test driver client server libindigo.a libindigosim.a indigo_test/*.o indigo_bus/*.o indigo_drivers/*.o indigo_drivers/ccd_simulator/*.o
