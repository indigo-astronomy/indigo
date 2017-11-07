CC = $(CROSS_COMPILE)gcc
CC:=$(strip $(CC))

TARGET = libnmea
BIN:=lib/$(TARGET).a
MODULES = generate generator parse parser tok context time info gmath sentence
SAMPLES = generate generator parse parse_file math
 
OBJ = $(MODULES:%=build/nmea_$(CC)/%.o) 
LINKOBJ = $(OBJ) $(RES)

SMPLS = $(SAMPLES:%=samples_%)
SMPLOBJ = $(SAMPLES:%=samples/%/main.o)

INCS = -I include 
LIBS = -Llib -lnmea -lm
 
.PHONY: all all-before all-after clean clean-custom doc debug shared

_default: static

all: all-before $(BIN) samples all-after 

all-before:
	mkdir -p lib
	mkdir -p build/nmea_$(CC)

clean: clean-custom 
	rm -f $(LINKOBJ) $(BIN) $(SMPLOBJ) $(SMPLS)

doc:
	$(MAKE) -C doc
	
remake: clean all

debug: CFLAGS+=-g
debug: all

static: BIN := lib/$(TARGET).a
static: all-before lib/$(TARGET).a samples all-after 

info:
	echo $(BIN)

shared: CFLAGS += -fPIC
shared: BIN += lib/$(TARGET).so
shared: all-before lib/$(TARGET).so samples all-after 

lib/$(TARGET).so: $(LINKOBJ)
	$(CC) -shared -Wl,-soname=$(TARGET) -o $@ $^ -lm -ldl -lpthread

lib/$(TARGET).a: $(LINKOBJ)
	$(CROSS_COMPILE)ar rsc $@ $^
	$(CROSS_COMPILE)ranlib $@

build/nmea_$(CC)/%.o: src/%.c 
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

samples: $(SMPLS)

samples_%: samples/%/main.o
	$(CC) $< $(LIBS) -o build/$@

samples/%/main.o: samples/%/main.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@
