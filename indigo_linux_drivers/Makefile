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

FOLDERS=$(dir $(wildcard agent_*/Makefile) $(wildcard aux_*/Makefile) $(wildcard ccd_*/Makefile) $(wildcard dome_*/Makefile) $(wildcard focuser_*/Makefile) $(wildcard gps_*/Makefile) $(wildcard guider_*/Makefile) $(wildcard mount_*/Makefile) $(wildcard wheel_*/Makefile) $(wildcard system_*/Makefile))

.PHONY: all clean $(FOLDERS)

all: OP=all
all: $(FOLDERS)

install: OP=install
install: $(FOLDERS)

uninstall: OP=uninstall
uninstall: $(FOLDERS)

clean: OP=clean
clean: $(FOLDERS)

$(FOLDERS):
	$(MAKE) -C $@ $(OP)
