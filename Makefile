CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iindigo_drivers/ccd_simulator

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: indigo_test/main.o indigo_bus/indigo_bus.o indigo_drivers/indigo_driver.o indigo_drivers/ccd_simulator/ccd_simulator.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f test indigo_test/*.o indigo_bus/*.o indigo_drivers/*.o indigo_drivers/ccd_simulator/*.o
