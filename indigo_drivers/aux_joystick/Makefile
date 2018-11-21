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

DRIVER = indigo_aux_joystick

DRIVER_A = $(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
DRIVER_EXECUTABLE = $(BUILD_DRIVERS)/$(DRIVER)


.PHONY: all status install uninstall

all: status $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

status:
	@printf "\n$(DRIVER) ---------------------------------------\n\n"
	@printf "Archive:    $(notdir $(DRIVER_A))\n"
	@printf "SO library: $(notdir $(DRIVER_SO))\n"
	@printf "Executable: $(notdir $(DRIVER_EXECUTABLE))\n"
	@printf "\n"

ifeq ($(OS_DETECTED),Darwin)

indigo_aux_joystick.o: indigo_aux_joystick.m
	$(CC) $(MFLAGS) -Wno-shadow-ivar -c -o $@ $< -Iexternal/DDHidLib

$(DRIVER_A): indigo_aux_joystick.o $(addsuffix .o, $(basename $(wildcard external/DDHidLib/*.m)))
	$(AR) $(ARFLAGS) $@ $^

endif

ifeq ($(OS_DETECTED),Linux)

indigo_aux_joystick.o: indigo_aux_joystick.c
	$(CC)  $(CFLAGS) -c -o $@ $<

$(DRIVER_A): indigo_aux_joystick.o
	$(AR) $(ARFLAGS) $@ $^

endif

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

$(DRIVER_EXECUTABLE): indigo_aux_joystick_main.o $(DRIVER_A)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo

install: status
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)

uninstall: status
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER)

clean: status
	rm -f *.o external/DDHidLib/*.o $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)
