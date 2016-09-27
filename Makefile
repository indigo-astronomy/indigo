CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iindigo_drivers/ccd_simulator -std=c99

all: test driver client server

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: indigo_test/test.o indigo_bus/indigo_bus.o indigo_drivers/indigo_driver.o indigo_drivers/indigo_ccd_driver.o indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

driver: indigo_test/driver.o indigo_bus/indigo_bus.o indigo_bus/indigo_xml.o indigo_bus/indigo_driver_xml.o indigo_drivers/indigo_driver.o indigo_drivers/indigo_ccd_driver.o  indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

client: indigo_test/client.o indigo_bus/indigo_bus.o indigo_bus/indigo_xml.o indigo_bus/indigo_client_xml.o
	$(CC) $(CFLAGS) -o $@ $^

server: indigo_test/server.o indigo_bus/indigo_bus.o indigo_bus/indigo_xml.o indigo_bus/indigo_server_xml.o indigo_bus/indigo_driver_xml.o indigo_drivers/indigo_driver.o indigo_drivers/indigo_ccd_driver.o  indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test driver client indigo_test/*.o indigo_bus/*.o indigo_drivers/*.o indigo_drivers/ccd_simulator/*.o
