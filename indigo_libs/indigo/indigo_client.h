// Copyright (c) 2016 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO client
 \file indigo_client.h
 */

#ifndef indigo_client_h
#define indigo_client_h

#include <pthread.h>
#include <stdbool.h>

#include <indigo/indigo_bus.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <indigo/indigo_driver.h>
#else
#include <indigo/indigo_names.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define INDIGO_MAX_SERVERS    10

/** Use <enableBLOB>URL</enableBLOB> for remote INDIGO servers;
 *  defined in indigo_xml.c
 */
extern bool indigo_use_blob_urls;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#define INDIGO_MAX_DRIVERS    128

/** Driver entry type.
 */
typedef struct {
	char description[INDIGO_NAME_SIZE];     ///< driver description
	char name[INDIGO_NAME_SIZE];            ///< driver name (entry point name)
	driver_entry_point driver;              ///< driver entry point
	void *dl_handle;                        ///< dynamic library handle (NULL for statically linked driver)
	bool initialized;												///< driver is initialized
} indigo_driver_entry;

/** Remote executable entry type.
 */
typedef struct {
  char executable[INDIGO_NAME_SIZE];      ///< executable path name
  pthread_t thread;                       ///< client thread ID
  bool thread_started;                    ///< client thread started/stopped
  int pid;																///< process pid
  indigo_device *protocol_adapter;        ///< server protocol adapter
	char last_error[256];										///< last error reported within client thread
} indigo_subprocess_entry;

/** Array of all available drivers (statically & dynamically linked).
 */
extern indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];

/** Array of all available subprocesses.
 */
extern indigo_subprocess_entry indigo_available_subprocesses[INDIGO_MAX_SERVERS];

/** Add statically linked driver.
 */
extern indigo_result indigo_add_driver(driver_entry_point entry_point, bool init, indigo_driver_entry **driver);

/** Check if the driver is initialized, This function is not protected by mutex so it must be called in synchronized sections.
    Its intended useage is in driver entrypoints.
 */
extern bool indigo_driver_initialized(char *driver_name);

/** Remove statically linked driver or remove & unload dynamically linked driver
 */
extern indigo_result indigo_remove_driver(indigo_driver_entry *driver);

/** Load & add dynamically linked driver.
 */
extern indigo_result indigo_load_driver(const char *name, bool init, indigo_driver_entry **driver);

/** Start thread for subprocess.
 */
extern indigo_result indigo_start_subprocess(const char *executable, indigo_subprocess_entry **subprocess);

/** Stop thread for subprocess.
 */
extern indigo_result indigo_kill_subprocess(indigo_subprocess_entry *subprocess);

#endif

/** Remote server entry type.
 */
typedef struct {
	char name[INDIGO_NAME_SIZE];            ///< service name
	char host[INDIGO_NAME_SIZE];            ///< server host name
	int port;                               ///< server port
	uint32_t connection_id;                 ///< client connection ID
	pthread_t thread;                       ///< client thread ID
	bool thread_started;                    ///< client thread started/stopped
	int socket;                             ///< stream socket
	indigo_device *protocol_adapter;        ///< server protocol adapter
	char last_error[256];										///< last error reported within client thread
	bool shutdown;													///< request shutdown
} indigo_server_entry;


/** Array of all available servers.
 */
extern indigo_server_entry indigo_available_servers[INDIGO_MAX_SERVERS];

/** Create bonjour service name.
 */
void indigo_service_name(const char *host, int port, char *name);

/** Connect and start thread for remote server.
 */
extern indigo_result indigo_connect_server(const char *name, const char *host, int port, indigo_server_entry **server);
extern indigo_result indigo_connect_server_id(const char *name, const char *host, int port, uint32_t connection_id, indigo_server_entry **server);

/** If connected to the server returns true else returns false and last_error (if not NULL) will contain the last error
    reported within client thread. Last_error should have length of 256.
 */
extern bool indigo_connection_status(indigo_server_entry *server, char *last_error);

/** Disconnect and stop thread for remote server.
 */
extern indigo_result indigo_disconnect_server(indigo_server_entry *server);

/** Format item to string.
 */
extern indigo_result indigo_format_number(char *buffer, int buffer_size, char *format, double value);

#ifdef __cplusplus
}
#endif

#endif /* indigo_client_h */
