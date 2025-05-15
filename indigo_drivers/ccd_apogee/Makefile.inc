#---------------------------------------------------------------------
#
# Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

LDFLAGS += -lstdc++ -lcurl
INSTALL_SDK = install_sdk
ifeq ($(OS_DETECTED),Linux)
SDK_INCLUDE = -I bin_externals -I bin_externals/boost_regex
SDK_LIB = bin_externals/libapogee/lib/Linux/$(ARCH_DETECTED)/libapogee.a bin_externals/boost_regex/lib/Linux/$(ARCH_DETECTED)/libboost_regex-gcc-1_53.a
else
SDK_INCLUDE = -I bin_externals
endif

install_sdk:
	cd bin_externals/libapogee; make install-config CONFIG_PREFIX=$(INSTALL_ETC)/apogee RULES_PREFIX=$(INSTALL_RULES)
