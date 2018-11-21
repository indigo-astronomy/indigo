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

DRIVER = indigo_ccd_apogee

DRIVER_A = $(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
DRIVER_EXECUTABLE = $(BUILD_DRIVERS)/$(DRIVER)

ifeq ($(OS_DETECTED),Linux)
	SDK_LIB = externals/libapogee/libapogee.a externals/boost_regex/build/gcc/libboost_regex-gcc-1_53.a
endif

ifeq ($(OS_DETECTED),Darwin)
	SDK_LIB = externals/libapogee/libapogee.a
endif

.PHONY: all status install uninstall

all: status $(SDK_LIB) $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

status:
	@printf "\n$(DRIVER) ---------------------------------------\n\n"
	@printf "Archive:    $(notdir $(DRIVER_A))\n"
	@printf "SO library: $(notdir $(DRIVER_SO))\n"
	@printf "Executable: $(notdir $(DRIVER_EXECUTABLE))\n"
	@printf "\n"

indigo_ccd_apogee.o: indigo_ccd_apogee.cpp
	$(CC)  $(CXXFLAGS) -c -Iexternals -o $@ $<

$(DRIVER_A): indigo_ccd_apogee.o
	$(AR) $(ARFLAGS) $@ $^

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo  $(SDK_LIB) -lstdc++ -lcurl

$(DRIVER_EXECUTABLE): indigo_ccd_apogee_main.o $(DRIVER_A)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo $(SDK_LIB) -lstdc++ -lcurl

install: status
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)
	cd externals/libapogee; make install-config CONFIG_PREFIX=$(INSTALL_ETC)/apogee RULES_PREFIX=$(INSTALL_RULES)

uninstall: status
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER)

clean: status
	cd externals/libapogee; make clean
	cd externals/boost_regex/build; make clean
	rm -f *.o external/DDHidLib/*.o $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

externals/libapogee/libapogee.a:
	cd externals/libapogee; make clean; make

externals/boost_regex/build/gcc/libboost_regex-gcc-1_53.a:
	cd externals/boost_regex/build; make clean; make

