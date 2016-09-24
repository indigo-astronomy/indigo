CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iindigo_drivers/ccd_simulator

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: indigo_test/test.o indigo_bus/indigo_bus.o indigo_drivers/indigo_driver.o indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

driver: indigo_test/driver.o indigo_bus/indigo_bus.o indigo_bus/indigo_xml.o indigo_drivers/indigo_driver.o indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

all: test driver

clean:
	rm -f test driver indigo_test/*.o indigo_bus/*.o indigo_drivers/*.o indigo_drivers/ccd_simulator/*.o
