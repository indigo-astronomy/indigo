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

SIMULATOR_LIBS=$(wildcard $(BUILD_DRIVERS)/indigo_*_simulator.a)
DRIVER_LIBS=$(wildcard $(BUILD_DRIVERS)/indigo_*.a)

all: $(BUILD_BIN)/indigo_server

install:
	cp $(BUILD_BIN)/indigo_server $(INSTALL_BIN)

uninstall:
	rm -f $(INSTALL_BIN)/indigo_server

status:
	@printf "\nindigo_server -------------------------\n\n"

clean:
	rm -f *.o $(BUILD_BIN)/indigo_server *.data resource/*.data

clean-all: clean


$(BUILD_BIN)/indigo_server: ctrlpanel indigo_server.o indigo_cat_data.o $(SIMULATOR_LIBS)
ifeq ($(OS_DETECTED),Darwin)
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server.o indigo_cat_data.o $(SIMULATOR_LIBS) $(LDFLAGS) -lstdc++ -lz -lindigo
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
else
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server.o indigo_cat_data.o $(SIMULATOR_LIBS) $(LDFLAGS) -lz -ldns_sd -lstdc++ -lindigo
endif

#---------------------------------------------------------------------
#
#	Control panel
#
#---------------------------------------------------------------------

ctrlpanel_data: resource/data/constellations.bounds.json.data resource/data/mw.json.data resource/data/constellations.json.data resource/data/planets.json.data

ctrlpanel: ctrlpanel_data resource/celestial.min.js.data resource/d3.min.js.data resource/celestial.css.data resource/mng.html.data resource/ctrl.html.data resource/imager.html.data resource/mount.html.data resource/indigo.js.data resource/mng.png.data resource/components.js.data resource/mount.png.data resource/ctrl.png.data resource/imager.png.data resource/guider.png.data resource/indigo.css.data resource/bootstrap.min.css.data resource/glyphicons.css.data resource/jquery.min.js.data resource/bootstrap.min.js.data resource/popper.min.js.data resource/vue.min.js.data resource/glyphicons-regular.ttf.data

%.data: %
	cat $< | gzip | hexdump -v -e '1/1 "0x%02x, "' >$@
