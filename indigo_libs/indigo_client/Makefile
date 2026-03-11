#---------------------------------------------------------------------
#
# Copyright (c) 2026 CloudMakers, s. r. o.
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

include ../../Makefile.inc

ifeq ($(OS_DETECTED),Darwin)
	FORCE_ALL_ON=-Wl,-force_load
	FORCE_ALL_OFF=
endif

ifeq ($(OS_DETECTED),Linux)
	FORCE_ALL_ON=-Wl,--whole-archive
	FORCE_ALL_OFF=-Wl,--no-whole-archive
endif

BIN_EXTERNALS = $(INDIGO_ROOT)/bin_externals

.PHONY: all clean

all:	status libindigo_client

status:
	@printf "\nindigo_client -------------------------\n\n"

install: status libindigo_client
	cp $(BUILD_LIB)/libindigo_client.$(SOEXT) $(INSTALL_LIB)
	cp $(BUILD_LIB)/libindigo_client.a $(INSTALL_LIB)

uninstall: status
	rm -f $(INSTALL_LIB)/libindigo_client.$(SOEXT)
	rm -f $(INSTALL_LIB)/libindigo_client.a

clean: status
	rm -f *.o *.orig $(BUILD_LIB)/libindigo_client.*

clean-all: status
	git clean -dfx


#---------------------------------------------------------------------
#
#	Build libindigo_client
#
#---------------------------------------------------------------------

LIBINDIGO_CLIENT_SRCS := \
	indigo_bus.c \
	indigo_version.c \
	indigo_uni_io.c \
	indigo_xml.c \
	indigo_client_xml.c \
	indigo_base64.c \
	indigo_md5.c \
	indigo_fits.c \
	indigo_client.c \
	indigo_service_discovery.c \
	indigo_token.c

LIBINDIGO_CLIENT_FILES := $(LIBINDIGO_CLIENT_SRCS:.c=.o)

%.o: ../%.c
	$(CC) $(CFLAGS) -DINDIGO_CLIENT -c $< -o $@

libindigo_client: $(BUILD_LIB)/libindigo_client.a $(BUILD_LIB)/libindigo_client.$(SOEXT)

$(BUILD_LIB)/libindigo_client.a: $(LIBINDIGO_CLIENT_FILES)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigo_client.$(SOEXT): $(LIBINDIGO_CLIENT_FILES)
ifeq ($(OS_DETECTED),Darwin)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldns_sd -lz -lstdc++
else
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FORCE_ALL_OFF) -lavahi-client -lavahi-common -lz -lstdc++
endif
