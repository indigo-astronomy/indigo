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

include ../Makefile.inc

ifeq ($(OS_DETECTED),Linux)
	INDIGO_LIBS = -lindigo
else
	INDIGO_LIBS = $(BUILD_LIB)/libindigo.a -lz -ldl -lm
endif

all: $(BUILD_BIN)/indigo_prop_tool $(BUILD_BIN)/indigo_raw_to_fits $(BUILD_BIN)/indigo_drivers $(BUILD_BIN)/indigo_driver_metadata

install: all
	cp $(BUILD_BIN)/indigo_prop_tool $(INSTALL_BIN)
	cp $(BUILD_BIN)/indigo_raw_to_fits $(INSTALL_BIN)
	cp ./indigo_log_analyzer.pl $(INSTALL_BIN)/indigo_log_analyzer

uninstall:
	rm -f $(INSTALL_BIN)/indigo_prop_tool $(INSTALL_BIN)/indigo_raw_to_fits $(INSTALL_BIN)/indigo_log_analyzer

status:
	@printf "\nindigo_tools -------------------------\n\n"

clean: status
	rm -f *.o $(BUILD_BIN)/indigo_prop_tool $(BUILD_BIN)/indigo_raw_to_fits $(BUILD_BIN)/indigo_drivers

clean-all: status
	git clean -dfx

$(BUILD_BIN)/indigo_prop_tool: indigo_prop_tool.o
	$(CC) $(CFLAGS)  -o $@ indigo_prop_tool.o $(LDFLAGS) $(INDIGO_LIBS)

$(BUILD_BIN)/indigo_raw_to_fits: indigo_raw_to_fits.o
	$(CC) $(CFLAGS)  -o $@ indigo_raw_to_fits.o $(LDFLAGS) $(INDIGO_LIBS)

$(BUILD_BIN)/indigo_drivers: indigo_drivers.o
	$(CC) $(CFLAGS)  -o $@ indigo_drivers.o $(LDFLAGS) -lindigo

$(BUILD_BIN)/indigo_driver_metadata: indigo_driver_metadata.o
	$(CC) $(CFLAGS)  -o $@ indigo_driver_metadata.o $(LDFLAGS) -lindigo
