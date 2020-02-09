src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lusb-1.0

fxload: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) fxload
