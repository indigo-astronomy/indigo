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

all: $(BUILD_BIN)/indigo_prop_tool $(BUILD_BIN)/indigo_drivers

install:
	cp $(BUILD_BIN)/indigo_prop_tool $(INSTALL_BIN)

uninstall:
	rm -f $(INSTALL_BIN)/indigo_prop_tool

status:

clean:
	rm -f $(BUILD_BIN)/indigo_prop_tool

clean-all:

$(BUILD_BIN)/indigo_prop_tool: indigo_prop_tool.o
	$(CC) $(CFLAGS)  -o $@ indigo_prop_tool.o $(LDFLAGS) -lindigo
