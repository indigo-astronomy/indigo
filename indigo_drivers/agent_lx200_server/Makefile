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

DRIVER=indigo_$(notdir $(shell pwd))

DRIVER_A=$(BUILD_DRIVERS)/$(DRIVER).a
DRIVER_SO=$(BUILD_DRIVERS)/$(DRIVER).$(SOEXT)
ifeq ($(wildcard $(DRIVER)_main.c),)
	DRIVER_EXECUTABLE=
	DRIVER_MAIN_O=
else
	DRIVER_EXECUTABLE=$(BUILD_DRIVERS)/$(DRIVER)
	DRIVER_MAIN_O=$(DRIVER)_main.o
endif
DRIVER_O=$(filter-out $(DRIVER_MAIN),$(addsuffix .o, $(basename $(notdir $(wildcard $(DRIVER)*.c)))))
RULES=$(notdir $(wildcard *.rules))
XML=$(notdir $(wildcard indi_go_*.xml))

all: $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)

$(DRIVER_A): $(DRIVER_O)
	$(AR) $(ARFLAGS) $@ $^

$(DRIVER_SO): $(DRIVER_A)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

$(DRIVER_EXECUTABLE): $(DRIVER_A) $(DRIVER_MAIN_O)
	$(CC) -o $@ $^ $(LDFLAGS) -lindigo

install:
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
ifneq ($(DRIVER_EXECUTABLE),)
	install -m 0755 $(DRIVER_EXECUTABLE) $(INSTALL_BIN)
endif
ifneq ($(RULES),)
	install -m 0644 $(RULES) $(INSTALL_RULES)/99-$(DRIVER).rules
endif
ifneq ($(XML),)
	install -m 0644 $(XML) $(INSTALL_SHARE)/indi
endif

uninstall:
	rm -f $(INSTALL_LIB)/$(DRIVER).$(SOEXT) $(INSTALL_BIN)/$(DRIVER) $(INSTALL_RULES)/99-$(DRIVER).rules $(INSTALL_SHARE)/indi/$(XML)

clean:
	rm -f $(DRIVER_O) $(DRIVER_MAIN_O) $(DRIVER_A) $(DRIVER_SO) $(DRIVER_EXECUTABLE)
