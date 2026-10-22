#---------------------------------------------------------------------
#
# Copyright (c) 2026 Rumen G. Bogdanovski
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

# Static PrimaLuce Lab protocol library, built by the drivers that link it (SDK_LIB / MAKE_SDK in
# their Makefile.inc). Several drivers may build it at the same time, so every build compiles in a
# directory of its own and moves the finished archive into place.

include ../../../Makefile.inc

LIB = $(BUILD_LIB)/libprimalucelib.a
SOURCES = primalucelib.c
HEADERS = primalucelib.h jsmn.h

.PHONY: all status clean clean-all

all: $(LIB)

status:
	@printf "Library:    $(notdir $(LIB))\n"

$(LIB): $(SOURCES) $(HEADERS) Makefile
	@install -d $(BUILD_LIB)
	@tmp=`mktemp -d $(BUILD_LIB)/primalucelib.XXXXXX` && $(CC) $(CFLAGS) -c -o $$tmp/primalucelib.o primalucelib.c && $(AR) rcs $$tmp/libprimalucelib.a $$tmp/primalucelib.o && mv -f $$tmp/libprimalucelib.a $@; status=$$?; rm -rf $$tmp; exit $$status

clean:
	rm -rf $(LIB) $(BUILD_LIB)/primalucelib.*

clean-all: clean
