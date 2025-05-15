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

LDFLAGS += -lstdc++ -lftd2xx -lusb-1.0
SDK_INCLUDE = -I bin_externals/qsiapi-7.6.0/build/include
ifeq ($(OS_DETECTED),Darwin)
SDK_LIB_A ?= $(wildcard bin_externals/qsiapi-7.6.0/build/lib/macOS/*.a)
endif
ifeq ($(OS_DETECTED),Linux)
SDK_LIB_A ?= $(wildcard bin_externals/qsiapi-7.6.0/build/lib/Linux/$(ARCH_DETECTED)/*.a)
endif

