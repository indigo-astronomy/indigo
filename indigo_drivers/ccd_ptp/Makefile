#---------------------------------------------------------------------
#
# Copyright (c) 2023 CloudMakers, s. r. o.
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

DRIVER = indigo_ccd_ptp

DRIVER_A = $(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
DRIVER_EXECUTABLE = $(BUILD_DRIVERS)/$(DRIVER)

ifeq ($(OS_DETECTED),Darwin)
	FORCE_ALL_ON=-Wl,-force_load
	FORCE_ALL_OFF=
endif
ifeq ($(OS_DETECTED),Linux)
	FORCE_ALL_ON=-Wl,--whole-archive
	FORCE_ALL_OFF=-Wl,--no-whole-archive
endif


.PHONY: all status install uninstall clean clean-all

all: status $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

status:
	@printf "\n$(DRIVER) ---------------------------------------\n\n"
	@printf "Archive:    $(notdir $(DRIVER_A))\n"
	@printf "SO library: $(notdir $(DRIVER_SO))\n"
	@printf "Executable: $(notdir $(DRIVER_EXECUTABLE))\n"
	@printf "\n"

ifeq ($(OS_DETECTED),Darwin)

indigo_ccd_ptp.o: indigo_ccd_ptp.m indigo_ccd_ptp.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ccd_ptp.m

indigo_ptp.o: indigo_ptp.m indigo_ptp.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ptp.m

indigo_ptp_canon.o: indigo_ptp_canon.m  indigo_ptp_canon.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ptp_canon.m

indigo_ptp_nikon.o: indigo_ptp_nikon.m  indigo_ptp_nikon.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ptp_nikon.m

indigo_ptp_sony.o: indigo_ptp_sony.m  indigo_ptp_sony.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ptp_sony.m

indigo_ptp_fuji.o: indigo_ptp_fuji.m  indigo_ptp_fuji.c
	$(CC) $(MFLAGS) -c -o $@ indigo_ptp_fuji.m

$(DRIVER_EXECUTABLE): indigo_ccd_ptp_main.o $(DRIVER_A)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo -framework IOKit -framework Cocoa

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $(FORCE_ALL_ON) $(DRIVER_A) $(FORCE_ALL_OFF) $(LDFLAGS) -lindigo -framework IOKit -framework Cocoa

endif

ifeq ($(OS_DETECTED),Linux)

indigo_ccd_ptp.o: indigo_ccd_ptp.c
	$(CC) $(CFLAGS) -c -o $@ $<

indigo_ptp.o: indigo_ptp.c
	$(CC) $(CFLAGS) -c -o $@ $<

indigo_ptp_canon.o: indigo_ptp_canon.c
	$(CC) $(CFLAGS) -c -o $@ $<

indigo_ptp_nikon.o: indigo_ptp_nikon.c
	$(CC) $(CFLAGS) -c -o $@ $<

indigo_ptp_sony.o: indigo_ptp_sony.c
	$(CC) $(CFLAGS) -c -o $@ $<

indigo_ptp_fuji.o: indigo_ptp_fuji.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(DRIVER_EXECUTABLE): indigo_ccd_ptp_main.o $(DRIVER_A)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo -lusb-1.0

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $(FORCE_ALL_ON) $(DRIVER_A) $(FORCE_ALL_OFF) $(LDFLAGS) -lindigo -lusb-1.0

endif

$(DRIVER_A): indigo_ccd_ptp.o indigo_ptp.o indigo_ptp_canon.o indigo_ptp_nikon.o indigo_ptp_sony.o indigo_ptp_fuji.o
	$(AR) $(ARFLAGS) $@ $^

install: status
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)

uninstall: status
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER)

clean: status
	rm -f *.o $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

clean-all: clean


