LIB_NAME = libmdns
OBJ_DIR  = ./obj
LIB_DIR  = ./lib

CC       = gcc
DEFINES  = -DUNICODE -D_UNICODE -DWIN32 -DMINGW_HAS_SECURE_API=1 -DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN -DQT_BUILD_ZEROCONF_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB
CFLAGS   = -fno-keep-inline-dllexport -g -Wall -Wextra -Wextra $(DEFINES)
INCPATH  = -I.
LINKER   = gcc
LFLAGS   = -shared -Wl,-subsystem,windows -mthreads -Wl,--out-implib,$(LIB_DIR)/$(LIB_NAME).a
LIBS     = -lws2_32 -lwsock32

OBJECTS = $(OBJ_DIR)/dnssd_clientlib.o \
          $(OBJ_DIR)/dnssd_clientstub.o \
          $(OBJ_DIR)/dnssd_ipc.o

all: $(LIB_DIR)/$(LIB_NAME).dll

$(LIB_DIR)/$(LIB_NAME).dll: $(OBJECTS)
	$(LINKER) $(LFLAGS) -o $(LIB_DIR)/$(LIB_NAME).dll $(OBJECTS)  $(LIBS)

$(OBJ_DIR)/dnssd_clientlib.o: dnssd_clientlib.c dns_sd.h  directories
	$(CC) -c $(CFLAGS) $(INCPATH) -o$(OBJ_DIR)/dnssd_clientlib.o dnssd_clientlib.c

$(OBJ_DIR)/dnssd_clientstub.o: dnssd_clientstub.c dnssd_ipc.h dns_sd.h  directories
	$(CC) -c $(CFLAGS) $(INCPATH) -o $(OBJ_DIR)/dnssd_clientstub.o dnssd_clientstub.c

$(OBJ_DIR)/dnssd_ipc.o: dnssd_ipc.c dnssd_ipc.h dns_sd.h  directories
	$(CC) -c $(CFLAGS) $(INCPATH) -o $(OBJ_DIR)/dnssd_ipc.o dnssd_ipc.c

directories:
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(OUT_DIR)/*.a $(OUT_DIR)/*.dll

rebuild: clean build