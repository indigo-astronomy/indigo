// Copyright (C) 2023 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

#ifndef indigo_service_discovery_h
#define indigo_service_discovery_h

#include <stdbool.h>
#include <indigo/indigo_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)

typedef enum {
	INDIGO_SERVICE_ADDED,
	INDIGO_SERVICE_ADDED_UNIQUE,
	INDIGO_SERVICE_REMOVED,
	INDIGO_SERVICE_REMOVED_UNIQUE,
	INDIGO_SERVICE_END_OF_RECORD
} indigo_service_discovery_event;

extern indigo_result indigo_resolve_service(const char *name, uint32_t interface, void (*callback)(const char *name, uint32_t interface, const char *host, int port));
extern indigo_result indigo_start_service_browser(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface));
extern void indigo_stop_service_browser(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* indigo_client_h */
