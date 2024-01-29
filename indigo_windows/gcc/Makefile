#---------------------------------------------------------------------
#
# Copyright (c) 2023 Rumen G.Bogdanovski
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#---------------------------------------------------------------------

LIB_NAME        = libindigo_client
OBJ_DIR         = ./obj
OUTPUT_DIR      = ./build
MDNS_DIR        = ../externals/bonjour-sdk/
INDIGO_BASE     = ../../
INDIGO_DIR      = $(INDIGO_BASE)/indigo_libs/

CC              = gcc
MDNS_DEFINES    = -DUNICODE -D_UNICODE -DWIN32 -DMINGW_HAS_SECURE_API=1 -DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DMDNS_DEBUGMSGS=0 -DWIN32_LEAN_AND_MEAN -DUSE_TCP_LOOPBACK -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -DNOT_HAVE_SA_LEN -DQT_BUILD_ZEROCONF_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB
MDNS_CFLAGS     = -fno-keep-inline-dllexport -g -Wall -Wextra $(MDNS_DEFINES)
INDIGO_DEFINES  = -DINDIGO_WINDOWS
INDIGO_CFLAGS   = -O3 -fno-keep-inline-dllexport -g -Wall -Wextra $(INDIGO_DEFINES)
INCPATH         = -I$(INDIGO_DIR) -I$(MDNS_DIR)
LINKER          = gcc
LFLAGS          = -shared -Wl,-subsystem,windows -mthreads -Wl,--out-implib,$(OUTPUT_DIR)/$(LIB_NAME).lib -Wl,--output-def,$(OUTPUT_DIR)/$(LIB_NAME).def -static-libgcc -static-libstdc++
LIBS            = -lws2_32 -lwsock32

MDNS_OBJECTS = $(OBJ_DIR)/dnssd_clientlib.o \
          $(OBJ_DIR)/dnssd_clientstub.o \
          $(OBJ_DIR)/dnssd_ipc.o

INDIGO_OBJECTS = $(OBJ_DIR)/indigo_base64.o \
          $(OBJ_DIR)/indigo_bus.o \
          $(OBJ_DIR)/indigo_client.o \
          $(OBJ_DIR)/indigo_client_xml.o \
          $(OBJ_DIR)/indigo_io.o\
          $(OBJ_DIR)/indigo_service_discovery.o\
          $(OBJ_DIR)/indigo_token.o\
          $(OBJ_DIR)/indigo_xml.o\
          $(OBJ_DIR)/indigo_md5.o\
          $(OBJ_DIR)/indigo_fits.o\
          $(OBJ_DIR)/indigo_version.o

all: $(OUTPUT_DIR)/$(LIB_NAME).dll $(OUTPUT_DIR)/indigo_prop_tool.exe

$(OUTPUT_DIR)/$(LIB_NAME).dll: $(INDIGO_OBJECTS) $(MDNS_OBJECTS) directories
	$(LINKER) $(LFLAGS) -o $(OUTPUT_DIR)/$(LIB_NAME).dll $(INDIGO_OBJECTS) $(MDNS_OBJECTS) $(LIBS)

$(OBJ_DIR)/dnssd_clientlib.o: $(MDNS_DIR)/dnssd_clientlib.c $(MDNS_DIR)/dns_sd.h directories
	$(CC) -c $(MDNS_CFLAGS) $(INCPATH) -o$(OBJ_DIR)/dnssd_clientlib.o $(MDNS_DIR)/dnssd_clientlib.c

$(OBJ_DIR)/dnssd_clientstub.o: $(MDNS_DIR)/dnssd_clientstub.c $(MDNS_DIR)/dnssd_ipc.h $(MDNS_DIR)/dns_sd.h directories
	$(CC) -c $(MDNS_CFLAGS) $(INCPATH) -o $(OBJ_DIR)/dnssd_clientstub.o $(MDNS_DIR)/dnssd_clientstub.c

$(OBJ_DIR)/dnssd_ipc.o: $(MDNS_DIR)/dnssd_ipc.c $(MDNS_DIR)/dnssd_ipc.h $(MDNS_DIR)/dns_sd.h directories
	$(CC) -c $(MDNS_CFLAGS) $(INCPATH) -o $(OBJ_DIR)/dnssd_ipc.o $(MDNS_DIR)/dnssd_ipc.c

$(OBJ_DIR)/indigo_base64.o: $(INDIGO_DIR)/indigo_base64.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_bus.o: $(INDIGO_DIR)/indigo_bus.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_client.o: $(INDIGO_DIR)/indigo_client.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_client_xml.o: $(INDIGO_DIR)/indigo_client_xml.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_io.o: $(INDIGO_DIR)/indigo_io.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_service_discovery.o: $(INDIGO_DIR)/indigo_service_discovery.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_md5.o: $(INDIGO_DIR)/indigo_md5.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_token.o: $(INDIGO_DIR)/indigo_token.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_xml.o: $(INDIGO_DIR)/indigo_xml.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_fits.o: $(INDIGO_DIR)/indigo_fits.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OBJ_DIR)/indigo_version.o: $(INDIGO_DIR)/indigo_version.c directories
	$(CC) -c $(INDIGO_CFLAGS) $(INCPATH) -o $@ $<

$(OUTPUT_DIR)/indigo_prop_tool.exe: $(INDIGO_BASE)/indigo_tools/indigo_prop_tool.c
	$(CC) $(INDIGO_CFLAGS) $(INCPATH) -o $@ $< $(OUTPUT_DIR)/$(LIB_NAME).lib

BUNDLE_DIR = indigo_client_sdk_${VERSION}_${ARCH}
PTHREAD_PATH = $(shell dirname `which $(CC)`)
PTHREAD_BASE = $(shell dirname $(PTHREAD_PATH))
PTHREAD_INCLUDE_PATH = $(shell ls -d $(PTHREAD_BASE)/*-w64-mingw32)/include
bundle: all
	@mkdir -p $(BUNDLE_DIR)/include/indigo
	@mkdir -p $(BUNDLE_DIR)/include/pthread
	@mkdir -p $(BUNDLE_DIR)/lib
	@mkdir -p $(BUNDLE_DIR)/doc
	echo "INDIGO Client SDK for Windows v.${VERSION} ${ARCH}" > $(BUNDLE_DIR)/VERSION
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_config.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_names.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_bus.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_client.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_service_discovery.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_token.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_md5.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_io.h $(BUNDLE_DIR)/include/indigo/
	cp $(INDIGO_BASE)/indigo_libs/indigo/indigo_fits.h $(BUNDLE_DIR)/include/indigo/
	cp $(OUTPUT_DIR)/$(LIB_NAME).lib $(BUNDLE_DIR)/lib/
	cp $(OUTPUT_DIR)/$(LIB_NAME).dll $(BUNDLE_DIR)/lib/
	cp $(PTHREAD_PATH)/libwinpthread-1.dll $(BUNDLE_DIR)/lib/
	cp $(OUTPUT_DIR)/indigo_prop_tool.exe $(BUNDLE_DIR)/lib/
	cp $(PTHREAD_INCLUDE_PATH)/pthread* $(BUNDLE_DIR)/include/pthread
	cp $(INDIGO_BASE)/indigo_docs/PROPERTY_MANIPULATION.md $(BUNDLE_DIR)/doc
	cp $(INDIGO_BASE)/indigo_docs/CLIENT_DEVELOPMENT_BASICS.md $(BUNDLE_DIR)/doc
	cp $(INDIGO_BASE)/LICENSE.md $(BUNDLE_DIR)
	cp README.md $(BUNDLE_DIR)
	powershell compress-archive -Force $(BUNDLE_DIR) $(BUNDLE_DIR).zip
	rm -r $(BUNDLE_DIR)

directories:
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(OUTPUT_DIR)/*.a $(OUTPUT_DIR)/*.lib $(OUTPUT_DIR)/*.dll $(OUTPUT_DIR)/*.exe $(BUNDLE_DIR)

rebuild: clean build
