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

DRIVER = indigo_mount_nexstar

DRIVER_A = $(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
DRIVER_EXECUTABLE = $(BUILD_DRIVERS)/$(DRIVER)

SDK_DIR = externals/libnexstar
SDK_INCLUDE = -I$(SDK_DIR)/include
SDK_LIB_A = $(SDK_DIR)/libnexstar.a
SDK_LIB = -L$(SDK_DIR) -lnexstar

ifeq ($(OS_DETECTED),Darwin)
	FORCE_ALL_ON=-Wl,-force_load
	FORCE_ALL_OFF=
endif
ifeq ($(OS_DETECTED),Linux)
	FORCE_ALL_ON=-Wl,--whole-archive
	FORCE_ALL_OFF=-Wl,--no-whole-archive
endif

CFLAGS += -Wno-constant-conversion

.PHONY: all status install uninstall

all: status $(SDK_LIB_A) $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

status:
	@printf "\n$(DRIVER) ---------------------------------------\n\n"
	@printf "Archive:    $(notdir $(DRIVER_A))\n"
	@printf "SO library: $(notdir $(DRIVER_SO))\n"
	@printf "Executable: $(notdir $(DRIVER_EXECUTABLE))\n"
	@printf "SDK:        $(notdir $(SDK_LIB) $(SDK_INCLUDE))\n"
	@printf "\n"

indigo_mount_nexstar.o: indigo_mount_nexstar.c
	$(CC)  $(CXXFLAGS) -c $(SDK_INCLUDE) -o $@ $<

$(DRIVER_A): indigo_mount_nexstar.o
	$(AR) $(ARFLAGS) $@ $^

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $(FORCE_ALL_ON) $(DRIVER_A) $(FORCE_ALL_OFF) $(LDFLAGS) $(SDK_LIB) -lindigo

$(DRIVER_EXECUTABLE): indigo_mount_nexstar_main.o $(DRIVER_A)
	$(CC) -o $@ $^ -lindigo $(SDK_LIB) -lstdc++ $(LDFLAGS)

install: status
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)

uninstall: status
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER)

clean: status
	rm -f $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

clean-all:
	rm -rf $(SDK_DIR)

$(SDK_LIB_A):
	git clone https://github.com/indigo-astronomy/libnexstar.git externals/libnexstar
	cd externals/libnexstar; autoreconf -fiv; ./configure --prefix=`pwd` --libdir=`pwd` --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)" --with-pic; make; make install
