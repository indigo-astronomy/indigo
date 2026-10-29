// Copyright (c) 2025 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 3.0 index management, index optimization and performance optimization by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO plate solver agent
 \file indigo_agent_solver.c
 */

#define DRIVER_VERSION 0x03000017
#define DRIVER_NAME	"indigo_agent_solver"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#if defined(INDIGO_WINDOWS)
#include <windows.h>
#include <wininet.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_filter.h>
#include <indigo/indigo_raw_utils.h>
#include <indigo/indigo_platesolver.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_agent_solver.h"
#include "indigo_solver.h"

#define SOLVER_DEVICE_PRIVATE_DATA				((solver_private_data *)device->private_data)
#define SOLVER_CLIENT_PRIVATE_DATA				((solver_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_SOLVER_INDEX_PROPERTY				(SOLVER_DEVICE_PRIVATE_DATA->index_property)
#define AGENT_SOLVER_INDEX_PROPERTY_NAME	"X_AGENT_SOLVER_INDEX"

// Quad indexes built by indigo_solver_indexer are downloaded from here as index-<scale>deg-tyc2.isix (Tycho-2) and
// index-<scale>deg-gaia.isix (Gaia DR3, scales below 1 degree), the scale zero padded so the files sort by it
#define AGENT_SOLVER_INDEX_URL						"https://github.com/indigo-astronomy/indigo/releases/download/index_release_v2"

#define MAX_IMAGE_STAR_COUNT							500
#define MAX_IMAGE_QUAD_COUNT							1000000
#define MAX_QUAD_MATCH_COUNT							1000000
#define MAX_STAR_MATCH_COUNT							1000
#define IMAGE_GRID_CELLS									10

static char base_dir[512];

// approximate sizes of the published index files, in the order of indigo_solver_index_fovs
static const char *index_sizes[INDIGO_SOLVER_INDEX_FOV_COUNT] = { "1216 MB", "911 MB", "538 MB", "358 MB", "178 MB", "92 MB", "36 MB" };


typedef struct {
	platesolver_private_data platesolver;
	indigo_property *index_property;
	int index_pending;
	volatile bool index_download_aborted;
	volatile bool abort_requested;
#if !defined(INDIGO_WINDOWS)
	volatile pid_t download_pid;
#endif
	char loaded_index[PATH_MAX];
	indigo_solver_catalog catalog;
} solver_private_data;

static pthread_mutex_t index_pending_mutex = PTHREAD_MUTEX_INITIALIZER;

// -------------------------------------------------------------------------------- Index files

// INDEX_<scale>DEG_<CATALOG> as the file name: INDEX_00_2DEG_GAIA and INDEX_00_5DEG_GAIA for the Gaia DR3 indexes of
// scale 0.2 and 0.5, INDEX_01DEG_TYC2 … INDEX_20DEG_TYC2 for the Tycho-2 indexes, the scale zero padded so the names
// sort by it and with an underscore for the decimal point (a dot in an item name is not safe for every client)
static void index_item_name(float fov, char *name, size_t size) {
	char scale[16];
	indigo_solver_index_scale(fov, scale, sizeof(scale));
	snprintf(name, size, "INDEX_%sDEG_%s", scale, indigo_solver_index_catalog(fov));
	for (char *c = name; *c; c++) {
		*c = *c == '.' ? '_' : toupper(*c);
	}
}

static bool index_item_fov(const char *name, float *fov) {
	if (strncmp(name, "INDEX_", 6)) {
		return false;
	}
	char scale[INDIGO_NAME_SIZE];
	snprintf(scale, sizeof(scale), "%s", name + 6);
	char *end = strstr(scale, "DEG_");
	if (end == NULL) {
		return false;
	}
	*end = 0;
	char *point = strchr(scale, '_');
	if (point != NULL) {
		*point = '.';
	}
	float value = atof(scale);
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		if (indigo_solver_index_fovs[i] == value) {
			*fov = value;
			return true;
		}
	}
	return false;
}

// "2° Tycho-2 (FOV 2° - 4.5°)", with the size of the file "2° Tycho-2 (FOV 2° - 4.5°, 358 MB)"; the items are listed
// in the order of the scale
static void index_item_label(int index, char *label, size_t size, bool with_size) {
	indigo_solver_index_label(indigo_solver_index_fovs[index], label, size);
	size_t length = strlen(label);
	if (with_size && length > 0 && label[length - 1] == ')') {
		snprintf(label + length - 1, size - length + 1, ", %s)", index_sizes[index]);
	}
}

static bool index_installed(float fov) {
	char path[PATH_MAX];
	indigo_solver_index_path(path, sizeof(path), base_dir, fov);
	return indigo_uni_is_readable(path);
}

// an interrupted transfer is never taken for an installed index, so partial files are removed on start
static void remove_partial_index_files(void) {
	char path[PATH_MAX + 8];
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		char index[PATH_MAX];
		indigo_solver_index_path(index, sizeof(index), base_dir, indigo_solver_index_fovs[i]);
		snprintf(path, sizeof(path), "%s.part", index);
		if (indigo_uni_is_readable(path)) {
			indigo_uni_remove(path);
		}
	}
}

static void free_catalog(indigo_device *device) {
	indigo_solver_free_catalog(&SOLVER_DEVICE_PRIVATE_DATA->catalog);
	SOLVER_DEVICE_PRIVATE_DATA->loaded_index[0] = 0;
}

// the last used index stays loaded, the next solve with the same index skips the file
static bool load_catalog(indigo_device *device, const char *path) {
	if (!strcmp(SOLVER_DEVICE_PRIVATE_DATA->loaded_index, path)) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Index %s is already loaded", path);
		return true;
	}
	free_catalog(device);
	if (!indigo_solver_load_index(path, &SOLVER_DEVICE_PRIVATE_DATA->catalog)) {
		return false;
	}
	snprintf(SOLVER_DEVICE_PRIVATE_DATA->loaded_index, sizeof(SOLVER_DEVICE_PRIVATE_DATA->loaded_index), "%s", path);
	return true;
}

#if defined(INDIGO_WINDOWS)

/* Fetches the index natively over HTTP(S) into part_path, so that the agent does not depend on an external curl
   executable on Windows; the transfer is interrupted by solver_abort() or kill_children(). */
static bool download_index_file(indigo_device *device, const char *url, const char *part_path) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> GET %s", url);
	bool res = false;
	HINTERNET session = InternetOpenA("INDIGO Solver Agent", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (session == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "InternetOpen() failed (%s)", indigo_last_windows_error());
		return false;
	}
	HINTERNET request = InternetOpenUrlA(session, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (request == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "InternetOpenUrl() failed (%s)", indigo_last_windows_error());
	} else {
		DWORD status = 0, status_size = sizeof(status);
		if (!HttpQueryInfoA(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &status_size, NULL) || status != 200) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "HTTP status %lu", (unsigned long)status);
		} else {
			FILE *file = fopen(part_path, "wb");
			if (file != NULL) {
				unsigned char *buffer = indigo_safe_malloc(64 * 1024);
				DWORD bytes_read = 0;
				res = true;
				while (true) {
					if (!InternetReadFile(request, buffer, 64 * 1024, &bytes_read)) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "InternetReadFile() failed (%s)", indigo_last_windows_error());
						res = false;
						break;
					}
					if (bytes_read == 0) {
						break;
					}
					if (fwrite(buffer, 1, bytes_read, file) != bytes_read) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to write %s", part_path);
						res = false;
						break;
					}
					if (SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted) {
						res = false;
						break;
					}
				}
				free(buffer);
				fclose(file);
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to create %s (%s)", part_path, strerror(errno));
			}
		}
		InternetCloseHandle(request);
	}
	InternetCloseHandle(session);
	return res;
}

#else

/* Fetches the index with curl into part_path; curl runs in its own process group, so that solver_abort() and
   kill_children() stop the transfer. */
static bool download_index_file(indigo_device *device, const char *url, const char *part_path) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> curl -L -s -f -o %s %s", part_path, url);
	pid_t pid = fork();
	if (pid < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to execute curl (%s)", strerror(errno));
		return false;
	}
	if (pid == 0) {
		setpgid(0, 0);
		execlp("curl", "curl", "-L", "-s", "-f", "-o", part_path, url, (char *)NULL);
		_exit(127);
	}
	setpgid(pid, pid);
	SOLVER_DEVICE_PRIVATE_DATA->download_pid = pid;
	/* an abort requested between fork() and the assignment above did not see the child */
	if (SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted) {
		kill(-pid, SIGTERM);
	}
	int status = 0;
	pid_t waited;
	do {
		waited = waitpid(pid, &status, 0);
	} while (waited < 0 && errno == EINTR);
	SOLVER_DEVICE_PRIVATE_DATA->download_pid = 0;
	if (waited != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "curl is not available");
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "curl failed with status %d", waited == pid && WIFEXITED(status) ? WEXITSTATUS(status) : -1);
		}
		return false;
	}
	return true;
}

#endif

// a downloaded file is accepted only with a valid header for the requested scale
static bool validate_index_file(const char *path, float fov) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		return false;
	}
	indigo_solve_index_header header;
	bool valid = indigo_solver_index_read_header(file, &header) && header.fov == fov && header.star_count > 0 && header.quad_count > 0;
	fclose(file);
	return valid;
}

static void unselect_missing_indexes(indigo_device *device) {
	for (int i = 0; i < AGENT_SOLVER_INDEX_PROPERTY->count; i++) {
		indigo_item *item = AGENT_SOLVER_INDEX_PROPERTY->items + i;
		float fov;
		if (item->sw.value && index_item_fov(item->name, &fov) && !index_installed(fov)) {
			item->sw.value = false;
		}
	}
}

// every installed index is listed, so an index not listed yet was just downloaded and is selected for use
static void rebuild_use_index(indigo_device *device) {
	indigo_delete_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
	bool selected[INDIGO_SOLVER_INDEX_FOV_COUNT];
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		char name[INDIGO_NAME_SIZE];
		index_item_name(indigo_solver_index_fovs[i], name, sizeof(name));
		selected[i] = true;
		for (int j = 0; j < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; j++) {
			if (!strcmp(name, AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items[j].name)) {
				selected[i] = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items[j].sw.value;
			}
		}
	}
	AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count = 0;
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		float fov = indigo_solver_index_fovs[i];
		if (index_installed(fov)) {
			char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
			index_item_name(fov, name, sizeof(name));
			index_item_label(i, label, sizeof(label), false);
			indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, selected[i]);
		}
	}
	AGENT_PLATESOLVER_USE_INDEX_PROPERTY->state = INDIGO_OK_STATE;
	indigo_define_property(device, AGENT_PLATESOLVER_USE_INDEX_PROPERTY, NULL);
}

// Downloads every newly selected index and removes every unselected one, then republishes the usable set
static void sync_installed_indexes(indigo_device *device) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char path[PATH_MAX], part_path[PATH_MAX + 8], url[INDIGO_VALUE_SIZE];
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < AGENT_SOLVER_INDEX_PROPERTY->count; i++) {
		indigo_item *item = AGENT_SOLVER_INDEX_PROPERTY->items + i;
		float fov;
		if (!index_item_fov(item->name, &fov)) {
			continue;
		}
		indigo_solver_index_path(path, sizeof(path), base_dir, fov);
		if (item->sw.value) {
			if (indigo_uni_is_readable(path)) {
				continue;
			}
			if (SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted) {
				goto aborted;
			}
			snprintf(part_path, sizeof(part_path), "%s.part", path);
			char *file_name = strrchr(path, '/');
			file_name = file_name ? file_name + 1 : path;
			snprintf(url, sizeof(url), "%s/%s", AGENT_SOLVER_INDEX_URL, file_name);
			indigo_send_message(device, IDLE_PROPERTY, "Downloading %s...", file_name);
			bool downloaded = download_index_file(device, url, part_path);
			if (SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted) {
				indigo_uni_remove(part_path);
				goto aborted;
			}
			if (!downloaded || !validate_index_file(part_path, fov) || rename(part_path, path)) {
				indigo_uni_remove(part_path);
				item->sw.value = false;
				AGENT_SOLVER_INDEX_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_SOLVER_INDEX_PROPERTY, "Index download failed: '%s'", url);
				goto done;
			}
			indigo_send_message(device, OK_PROPERTY, "Done");
		} else if (indigo_uni_is_readable(path)) {
			if (!strcmp(SOLVER_DEVICE_PRIVATE_DATA->loaded_index, path)) {
				free_catalog(device);
			}
			if (!indigo_uni_remove(path)) {
				AGENT_SOLVER_INDEX_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, AGENT_SOLVER_INDEX_PROPERTY, "%s", strerror(errno));
				goto done;
			}
		}
	}
	goto done;
aborted:
	unselect_missing_indexes(device);
	AGENT_SOLVER_INDEX_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, AGENT_SOLVER_INDEX_PROPERTY, "Index download aborted");
done:
	rebuild_use_index(device);
	indigo_platesolver_save_config(device);
	pthread_mutex_unlock(&mutex);
}

/* Index changes are accepted while a download is running, each one queues another handler;
   the property stays busy until the last queued request is processed. */

static void index_request_queued(indigo_device *device) {
	pthread_mutex_lock(&index_pending_mutex);
	SOLVER_DEVICE_PRIVATE_DATA->index_pending++;
	pthread_mutex_unlock(&index_pending_mutex);
}

static void index_handler(indigo_device *device) {
	sync_installed_indexes(device);
	pthread_mutex_lock(&index_pending_mutex);
	bool more = --SOLVER_DEVICE_PRIVATE_DATA->index_pending > 0;
	pthread_mutex_unlock(&index_pending_mutex);
	if (more && !SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted) {
		return;
	}
	if (AGENT_SOLVER_INDEX_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_SOLVER_INDEX_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, AGENT_SOLVER_INDEX_PROPERTY, NULL);
}

// -------------------------------------------------------------------------------- Solving

static void solver_abort(indigo_device *device) {
	SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted = true;
	SOLVER_DEVICE_PRIVATE_DATA->abort_requested = true;
#if !defined(INDIGO_WINDOWS)
	pid_t pid = SOLVER_DEVICE_PRIVATE_DATA->download_pid;
	if (pid > 0) {
		kill(-pid, SIGTERM);
	}
#endif
}

// Orders the selected indexes: the one fitting the field size first, when the scale is known, then the others
static int select_candidate_fovs(indigo_device *device, double scale, int width, int height, float *candidates) {
	float selected[INDIGO_SOLVER_INDEX_FOV_COUNT];
	int selected_count = 0;
	for (int i = 0; i < AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count; i++) {
		indigo_item *item = AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + i;
		float fov;
		if (item->sw.value && index_item_fov(item->name, &fov) && index_installed(fov)) {
			selected[selected_count++] = fov;
		}
	}
	int count = 0;
	if (scale > 0 && selected_count > 0) {
		char path[PATH_MAX];
		float fov;
		if (indigo_solver_select_index(base_dir, selected, selected_count, width * scale * DEG_TO_RAD, height * scale * DEG_TO_RAD, path, sizeof(path), &fov)) {
			candidates[count++] = fov;
		}
	}
	// the other selected indexes follow, the nearest to the field size first when the scale is known, otherwise the
	// largest index first
	float target = scale > 0 ? INDIGO_SOLVER_INDEX_FIELD_RATIO * sqrt((double)width * height) * scale : 0;
	while (true) {
		float best = 0, best_distance = 0;
		for (int i = 0; i < selected_count; i++) {
			bool listed = false;
			for (int j = 0; j < count; j++) {
				listed = listed || candidates[j] == selected[i];
			}
			if (listed) {
				continue;
			}
			float distance = target > 0 ? fabs(log(selected[i] / target)) : -selected[i];
			if (best == 0 || distance < best_distance) {
				best = selected[i];
				best_distance = distance;
			}
		}
		if (best == 0) {
			break;
		}
		candidates[count++] = best;
	}
	return count;
}

// The image star counts of the progressive stages: the DEPTH hint first and then all stars, or the built-in
// progression when the hint is 0; the stages above the detected count collapse into the final one
static int depth_stages(int depth, int star_count, int *stages) {
	static const int progression[] = { 40, 100 };
	int count = 0;
	if (depth > 0) {
		if (depth < star_count) {
			stages[count++] = depth;
		}
	} else {
		for (unsigned i = 0; i < sizeof(progression) / sizeof(progression[0]); i++) {
			if (progression[i] < star_count) {
				stages[count++] = progression[i];
			}
		}
	}
	stages[count++] = star_count;
	return count;
}

#define MAX_DEPTH_STAGES 3

// The matching loops stop at the CPU limit or an abort; the abort also stops the index loop before any matching
static bool limits_exceeded_or_aborted(indigo_device *device, indigo_solver_limits *limits) {
	return limits->exceeded || SOLVER_DEVICE_PRIVATE_DATA->abort_requested;
}

static bool solver_solve(indigo_device *device, indigo_platesolver_task *task) {
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) != 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solver is busy");
		return false;
	}
	if (AGENT_SOLVER_INDEX_PROPERTY->state == INDIGO_BUSY_STATE) {
		pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "Index download in progress, solving is not possible");
		return false;
	}
	/* an abort since the image arrived stays requested, an older one is cleared */
	SOLVER_DEVICE_PRIVATE_DATA->abort_requested = task->abort_generation != INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->abort_generation;
	indigo_send_message(device, IDLE_PROPERTY, "Solving started");
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = true;
	const char *message = "";
	AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_BUSY_STATE;
	AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = 0;
	AGENT_PLATESOLVER_WCS_STATE_ITEM->number.value = INDIGO_SOLVER_STATE_SOLVING;
	indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
	indigo_raw_header *image = NULL;
	indigo_solver_star *image_stars = NULL;
	indigo_solver_quad *image_quads = NULL;
	indigo_solver_quad_match *quad_matches = NULL;
	indigo_solver_star_match *star_matches = NULL;
	// the CPU limit and the abort flag bound every matching loop; the deadline starts now, so index loading counts
	indigo_solver_limits limits = { 0 };
	limits.abort = &SOLVER_DEVICE_PRIVATE_DATA->abort_requested;
	if (AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value > 0) {
		limits.deadline = indigo_monotonic_time() + AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM->number.value;
	}
	// 1. the input image as RAW MONO16: INDIGO RAW as it is, FITS, JPEG and camera raw files converted; a Bayer
	//    mosaic is binned at least 2 x 2 so the colour pattern does not reach the star detector
	bool bayered = false;
	int bin, full_width, full_height;
	image = indigo_solver_load_image(task->image, task->size, (int)AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM->number.value, &bin, &full_width, &full_height, &bayered, &message);
	if (image == NULL) {
		goto cleanup;
	}
	if (bayered) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bayer mosaic, binning %d x %d", bin, bin);
	}
	int width = image->width;
	int height = image->height;
	// 2. image stars, brightest first
	image_stars = indigo_safe_malloc(MAX_IMAGE_STAR_COUNT * sizeof(indigo_solver_star));
	int star_count = indigo_solver_image_stars(image, image_stars, MAX_IMAGE_STAR_COUNT);
	if (star_count < 4) {
		message = "Not enough stars detected";
		goto cleanup;
	}
	if (SOLVER_DEVICE_PRIVATE_DATA->abort_requested) {
		message = "Aborted";
		goto cleanup;
	}
	image_quads = indigo_safe_malloc(MAX_IMAGE_QUAD_COUNT * sizeof(indigo_solver_quad));
	int grid = (width > height ? width : height) / IMAGE_GRID_CELLS;
	int stages[MAX_DEPTH_STAGES];
	int stage_count = depth_stages((int)AGENT_PLATESOLVER_HINTS_DEPTH_ITEM->number.value, star_count, stages);
	// 3. indexes to try, the one fitting the expected field size first, then the nearest other ones
	float candidates[INDIGO_SOLVER_INDEX_FOV_COUNT];
	double scale = AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value > 0 ? AGENT_PLATESOLVER_HINTS_SCALE_ITEM->number.value : INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->pixel_scale;
	int candidate_count = select_candidate_fovs(device, scale, full_width, full_height, candidates);
	if (candidate_count == 0) {
		message = "No index files are selected, select at least one";
		goto cleanup;
	}
	// the position hint, in the hints epoch, as a J2000 cone around which the catalog is restricted
	double hint_radius = AGENT_PLATESOLVER_HINTS_RADIUS_ITEM->number.value;
	double hint_ra = AGENT_PLATESOLVER_HINTS_RA_ITEM->number.value, hint_dec = AGENT_PLATESOLVER_HINTS_DEC_ITEM->number.value;
	if (hint_radius > 0 && AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.value == 0) {
		indigo_jnow_to_j2k(&hint_ra, &hint_dec);
	}
	int hint_parity = (int)AGENT_PLATESOLVER_HINTS_PARITY_ITEM->number.value;
	quad_matches = indigo_safe_malloc(MAX_QUAD_MATCH_COUNT * sizeof(indigo_solver_quad_match));
	star_matches = indigo_safe_malloc(MAX_STAR_MATCH_COUNT * sizeof(indigo_solver_star_match));
	float ra = 0, dec = 0, rotation = 0, pixel = 0, fov = 0, rms = 0;
	int parity = 0, matched = 0, star_match_count = 0;
	bool solved = false, wrong_parity = false;
	// The indexes are searched stage by stage: the first stage on every index, then the next stage on every index, so
	// an index that does not fit the field costs only its cheap first stage before the others are tried. With the
	// scale every stage starts with the index fitting the field; without it the first stage starts with the largest
	// index, as the larger indexes are the smaller files and fail fast, and every next stage runs in the opposite
	// order, so the index loaded last is used again without loading it
	for (int pass = 0; pass < stage_count && !solved && !limits_exceeded_or_aborted(device, &limits); pass++) {
		int first_stage = pass;
		int last_stage = pass + 1;
		for (int k = 0; k < candidate_count && !solved && !limits_exceeded_or_aborted(device, &limits); k++) {
			int i = scale == 0 && pass % 2 ? candidate_count - 1 - k : k;
			char path[PATH_MAX];
			indigo_solver_index_path(path, sizeof(path), base_dir, candidates[i]);
			char label[INDIGO_VALUE_SIZE];
			indigo_solver_index_label(candidates[i], label, sizeof(label));
			indigo_send_message(device, IDLE_PROPERTY, "Solving with index %s...", label);
			if (!load_catalog(device, path)) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to load %s", path);
				continue;
			}
			// the catalog restricted to the hint cone: the field centre is within the radius, its stars within the
			// radius plus half the field diagonal (or the index scale when the pixel scale is unknown); a quad needs
			// four stars within the cone
			const indigo_solver_catalog *catalog = &SOLVER_DEVICE_PRIVATE_DATA->catalog;
			indigo_solver_catalog cone = { 0 };
			bool usable = catalog->quad_count > 0;
			if (hint_radius > 0) {
				double extent = scale > 0 ? 0.5 * sqrt((double)full_width * full_width + (double)full_height * full_height) * scale : candidates[i];
				usable = usable && indigo_solver_cone_catalog(catalog, hint_ra * 15 * DEG_TO_RAD, hint_dec * DEG_TO_RAD, (hint_radius + extent) * DEG_TO_RAD, &cone) >= 4;
				catalog = &cone;
			}
			int catalog_quad_count = catalog->quad_count;
			for (int stage = first_stage; stage < last_stage && !solved && usable && !limits_exceeded_or_aborted(device, &limits); stage++) {
				int used_star_count = 0;
				int quad_count = indigo_solver_build_image_quads(image_stars, stages[stage], width, height, grid, &used_star_count, image_quads, MAX_IMAGE_QUAD_COUNT);
				if (quad_count == 0) {
					continue;
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stage %d: %d brightest stars, %d quads, %d catalog quads", stage + 1, stages[stage], quad_count, catalog_quad_count);
				int quad_match_count = indigo_solver_match_quads(image_quads, quad_count, catalog, quad_matches, MAX_QUAD_MATCH_COUNT, &limits);
				star_match_count = indigo_solver_match_stars(quad_matches, quad_match_count, star_matches, MAX_STAR_MATCH_COUNT, &limits);
				if (limits.exceeded) {
					break;
				}
				if (indigo_solver_compute_center(star_matches, star_match_count, width, height, &ra, &dec, &rotation, &pixel, &parity, &matched, &rms)) {
					if (hint_parity != 0 && parity != hint_parity) {
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Solution with parity %d rejected by the parity hint %d", parity, hint_parity);
						wrong_parity = true;
						continue;
					}
					fov = candidates[i];
					solved = true;
				}
			}
			indigo_solver_free_catalog(&cone);
		}
	}
	if (!solved) {
		message = limits.exceeded ? limits.reason : SOLVER_DEVICE_PRIVATE_DATA->abort_requested ? "Aborted" : wrong_parity ? "Solution has the wrong parity" : "No solution found";
		goto cleanup;
	}
	// 4. WCS result: RA in hours, Dec and angles in degrees, scale in degrees per unbinned pixel
	double ra_hours = ra * RAD_TO_DEG / 15;
	double dec_degrees = dec * RAD_TO_DEG;
	if (AGENT_PLATESOLVER_HINTS_EPOCH_ITEM->number.target == 0) {
		indigo_j2k_to_jnow(&ra_hours, &dec_degrees);
		AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 0;
	} else {
		AGENT_PLATESOLVER_WCS_EPOCH_ITEM->number.value = 2000;
	}
	AGENT_PLATESOLVER_WCS_RA_ITEM->number.value = ra_hours;
	AGENT_PLATESOLVER_WCS_DEC_ITEM->number.value = dec_degrees;
	AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value = pixel * RAD_TO_DEG / bin;
	AGENT_PLATESOLVER_WCS_WIDTH_ITEM->number.value = full_width * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
	AGENT_PLATESOLVER_WCS_HEIGHT_ITEM->number.value = full_height * AGENT_PLATESOLVER_WCS_SCALE_ITEM->number.value;
	AGENT_PLATESOLVER_WCS_ANGLE_ITEM->number.value = rotation * RAD_TO_DEG;
	AGENT_PLATESOLVER_WCS_PARITY_ITEM->number.value = parity;
	AGENT_PLATESOLVER_WCS_INDEX_ITEM->number.value = fov;
	INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed = false;
	AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_send_message(device, OK_PROPERTY, "Solved: %d of %d stars matched, rms %.2f\"", matched, star_match_count, rms * RAD_TO_DEG * 3600);
cleanup:
	indigo_safe_free(star_matches);
	indigo_safe_free(quad_matches);
	indigo_safe_free(image_quads);
	indigo_safe_free(image_stars);
	indigo_safe_free(image);
	if (INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed) {
		AGENT_PLATESOLVER_WCS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	if (message[0] == '\0') {
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, NULL);
	} else {
		indigo_update_property(device, AGENT_PLATESOLVER_WCS_PROPERTY, "%s", message);
	}
	pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	return !INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->failed;
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_platesolver_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- AGENT_SOLVER_INDEX
		AGENT_SOLVER_INDEX_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SOLVER_INDEX_PROPERTY_NAME, "Index management", "Installed quad indexes", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_SOLVER_INDEX_FOV_COUNT);
		if (AGENT_SOLVER_INDEX_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		strcpy(AGENT_SOLVER_INDEX_PROPERTY->hints, "warn_on_clear:\"Delete index file?\";");
		for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
			float fov = indigo_solver_index_fovs[i];
			char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE];
			index_item_name(fov, name, sizeof(name));
			index_item_label(i, label, sizeof(label), true);
			bool present = index_installed(fov);
			indigo_item *item = AGENT_SOLVER_INDEX_PROPERTY->items + i;
			indigo_init_switch_item(item, name, label, present);
			index_item_label(i, label, sizeof(label), false);
			snprintf(item->hints, sizeof(item->hints), "warn_on_clear:\"Delete index %s?\";", label);
			if (present) {
				indigo_init_switch_item(AGENT_PLATESOLVER_USE_INDEX_PROPERTY->items + AGENT_PLATESOLVER_USE_INDEX_PROPERTY->count++, name, label, false);
			}
		}
		// --------------------------------------------------------------------------------
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.save_config = indigo_platesolver_save_config;
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.solve = solver_solve;
		SOLVER_DEVICE_PRIVATE_DATA->platesolver.abort = solver_abort;
		indigo_load_properties(device, false);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return agent_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
	INDIGO_DEFINE_MATCHING_PROPERTY(AGENT_SOLVER_INDEX_PROPERTY);
	return indigo_platesolver_enumerate_properties(device, client, property);
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (client == FILTER_DEVICE_CONTEXT->client) {
		return INDIGO_OK;
	}
	if (indigo_property_match(AGENT_PLATESOLVER_IMAGE_PROPERTY, property) && AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE) {
		AGENT_PLATESOLVER_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, AGENT_PLATESOLVER_IMAGE_PROPERTY, "Solver is busy");
		return INDIGO_OK;
	} else if (indigo_property_match(AGENT_SOLVER_INDEX_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AGENT_SOLVER_INDEX
		INDIGO_REJECT_CHANGE_IF(AGENT_PLATESOLVER_WCS_PROPERTY->state == INDIGO_BUSY_STATE, AGENT_SOLVER_INDEX_PROPERTY, "Solving in progress, index change is not possible");
		SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted = false;
		index_request_queued(device);
		INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME(AGENT_SOLVER_INDEX_PROPERTY, index_handler);
		return INDIGO_OK;
	}
	return indigo_platesolver_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	solver_abort(device);
	indigo_cancel_pending_handlers(device);
	indigo_cancel_all_timers(device);
	free_catalog(device);
	indigo_release_property(AGENT_SOLVER_INDEX_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_platesolver_device_detach(device);
}

// -------------------------------------------------------------------------------- Initialization

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

// a running download is stopped when the server exits, the partial file is removed on the next start
static void kill_children() {
	indigo_device *device = agent_device;
	if (device && device->private_data) {
		SOLVER_DEVICE_PRIVATE_DATA->index_download_aborted = true;
		SOLVER_DEVICE_PRIVATE_DATA->abort_requested = true;
#if !defined(INDIGO_WINDOWS)
		pid_t pid = SOLVER_DEVICE_PRIVATE_DATA->download_pid;
		if (pid > 0) {
			kill(-pid, SIGKILL);
		}
#endif
	}
}

indigo_result indigo_agent_solver(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(
		SOLVER_AGENT_NAME,
		agent_device_attach,
		agent_enumerate_properties,
		agent_change_property,
		NULL,
		agent_device_detach
	);

	static indigo_client agent_client_template = {
		SOLVER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
		indigo_platesolver_client_attach,
		indigo_platesolver_define_property,
		indigo_platesolver_update_property,
		indigo_platesolver_delete_property,
		NULL,
		indigo_platesolver_client_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, SOLVER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch(action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			char *env = getenv("INDIGO_CACHE_BASE");
			if (env) {
				snprintf(base_dir, sizeof(base_dir), "%s/solver", env);
			} else {
				snprintf(base_dir, sizeof(base_dir), "%s/.indigo/solver", indigo_uni_home_folder());
			}
			indigo_uni_mkdir(base_dir);
			remove_partial_index_files();
			void *private_data = indigo_safe_malloc(sizeof(solver_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			static bool first_time = true;
			if (first_time) {
				first_time = false;
				atexit(kill_children);
			}
			break;
		}
		case INDIGO_DRIVER_SHUTDOWN: {
			last_action = action;
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_detach_device(agent_device);
				free(agent_device);
				agent_device = NULL;
			}
			break;
		}
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
