#---------------------------------------------------------------------
#
# Copyright (c) 2021 Rumen G.bogdanovski <rumenastro@gmail.com>
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

OS_DETECTED = $(shell uname -s)
ARCH_DETECTED = $(shell uname -m)
ifeq ($(OS_DETECTED),Linux)
	ifeq ($(ARCH_DETECTED),armv6l)
		ARCH_DETECTED = arm
		DEBIAN_ARCH = armhf
	endif
	ifeq ($(ARCH_DETECTED),armv7l)
		ARCH_DETECTED = arm
		DEBIAN_ARCH = armhf
	endif
	ifeq ($(ARCH_DETECTED),aarch64)
		ARCH_DETECTED = arm64
		DEBIAN_ARCH = arm64
	endif
	ifeq ($(ARCH_DETECTED),i686)
		ARCH_DETECTED = x86
		DEBIAN_ARCH = i386
	endif
	ifeq ($(ARCH_DETECTED),x86_64)
		ifneq ($(wildcard /lib/x86_64-linux-gnu/),)
			ARCH_DETECTED = x64
			DEBIAN_ARCH = amd64
		else ifneq ($(wildcard /lib64/),)
			ARCH_DETECTED = x64
			DEBIAN_ARCH = amd64
		else
			ARCH_DETECTED = x86
			DEBIAN_ARCH = i386
		endif
	endif
	CC = gcc
	AR = ar
	SDK_LIB_A ?= $(wildcard ../qhyccd/lib/Linux/$(ARCH_DETECTED)/*.a)
endif

pthread_yield_compat.o: pthread_yield_compat.c
	$(CC) -c -o $@ $< $(CFLAGS)

patchlib: pthread_yield_compat.o
	$(AR) rbvs cmosdll.o $(SDK_LIB_A) $<

.PHONY: clean

clean:
	rm -f *.o
