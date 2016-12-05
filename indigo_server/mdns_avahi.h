// Copyright (c) 2016 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** mDNS AVAHI
 \file mdns_avahi.h
 */

#ifndef __MDNS_AVAHI_H__
#define __MDNS_AVAHI_H__

#include "indigo_bus.h"

int mdns_init(char *name, char *type, char *text, int port);
int mdns_start();
int mdns_stop();

#ifdef HAVE_LIBAVAHI_CLIENT
#ifdef HAVE_LIBAVAHI_COMMON
	#define HAVE_LIBAVAHI 1
#endif
#endif

#ifdef HAVE_LIBAVAHI
        #define HAVE_MDNS 1
#endif


#define LOG(msg, ...) {\
		indigo_log(msg,  ## __VA_ARGS__); \
	}\


#define LOG_DBG(msg, ...)  { \
		indigo_debug(msg,  ## __VA_ARGS__);\
	}\




#endif /*__MDNS_AVAHI_H__*/
