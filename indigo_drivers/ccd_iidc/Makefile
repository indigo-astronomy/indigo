#---------------------------------------------------------------------
#
# Copyright (c) 2018 CloudMakers, s. r. o.
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

DRIVER = indigo_ccd_iidc

DRIVER_A = $(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
DRIVER_EXECUTABLE = $(BUILD_DRIVERS)/$(DRIVER)

SDK_LIB = $(BUILD_LIB)/libdc1394.$(SOEXT)

.PHONY: all status install uninstall

all: status $(SDK_LIB) $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

status:
	@printf "\n$(DRIVER) ---------------------------------------\n\n"
	@printf "Archive:    $(notdir $(DRIVER_A))\n"
	@printf "SO library: $(notdir $(DRIVER_SO))\n"
	@printf "Executable: $(notdir $(DRIVER_EXECUTABLE))\n"
	@printf "SDK:        $(notdir $(SDK_LIB))\n"
	@printf "\n"

indigo_ccd_iidc.o: indigo_ccd_iidc.c
	$(CC)  $(CXXFLAGS) -c -Iexternals -o $@ $<

$(DRIVER_A): indigo_ccd_iidc.o
	$(AR) $(ARFLAGS) $@ $^

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo  $(SDK_LIB) -ldc1394

$(DRIVER_EXECUTABLE): indigo_ccd_iidc_main.o $(DRIVER_A)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo $(SDK_LIB) -lstdc++ -ldc1394

install: status
	install -m 0644 $(SDK_LIB) $(INSTALL_LIB)
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)

uninstall: status
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER) $(INSTALL_LIB)/$(SDK_LIB)

clean: status
	rm -f $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

clean-all:

$(SDK_LIB):
	git clone https://github.com/indigo-astronomy/libdc1394.git
	cd libdc1394; autoreconf -fiv; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) --enable-shared=yes --enable-static=no CFLAGS="$(CFLAGS)" --with-pic; make; make install
	rm -rf libdc1394

	
