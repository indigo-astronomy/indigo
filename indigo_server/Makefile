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

$(BUILD_BIN)/indigo_server: ctrlpanel indigo_server.o $(SIMULATOR_LIBS)
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server.o $(SIMULATOR_LIBS) $(LDFLAGS) -lstdc++ -lindigo
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

$(BUILD_BIN)/indigo_server_standalone: ctrlpanel indigo_server.c $(DRIVER_LIBS) $(BUILD_LIB)/libindigo.a
	$(CC) -DSTATIC_DRIVERS $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server.c $(DRIVER_LIBS) $(BUILD_LIB)/libindigo.a $(LDFLAGS) -lstdc++  -lcurl -ltoupcam  -laltaircam -ldc1394 -lusb-1.0
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

#---------------------------------------------------------------------
#
#	Control panel
#
#---------------------------------------------------------------------
ctrlpanel: ctrl.data resource/angular.min.js.data resource/bootstrap.min.js.data resource/bootstrap.css.data resource/jquery.min.js.data resource/glyphicons-halflings-regular.ttf.data resource/logo.png.data

ctrl.data:	ctrl.html
#	python tools/rjsmin.py <ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >ctrl.data
	cat ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >ctrl.data

resource/angular.min.js.data:	resource/angular.min.js
	cat resource/angular.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/angular.min.js.data

resource/bootstrap.min.js.data:	resource/bootstrap.min.js
	cat resource/bootstrap.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/bootstrap.min.js.data

resource/bootstrap.css.data:	resource/bootstrap.css
	cat resource/bootstrap.css | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/bootstrap.css.data

resource/jquery.min.js.data:	resource/jquery.min.js
	cat resource/jquery.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/jquery.min.js.data

resource/glyphicons-halflings-regular.ttf.data:	resource/glyphicons-halflings-regular.ttf
	cat resource/glyphicons-halflings-regular.ttf | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/glyphicons-halflings-regular.ttf.data

resource/logo.png.data:	resource/logo.png
	cat resource/logo.png | gzip | hexdump -v -e '1/1 "0x%02x, "' > resource/logo.png.data
