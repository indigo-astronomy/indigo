// Copyright (c) 2026 Rumen G. Bogdanovski
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
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO quad based plate solver - command line tool solving an image file with the indexes of the Platesolver Agent
 \file indigo_solver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_timer.h>
#include <indigo/indigo_uni_io.h>

#include <agent_platesolver/indigo_solver.h>

#define TOOL_NAME									"indigo_solver"

// the same limits as the Platesolver Agent
#define MAX_IMAGE_STAR_COUNT			500
#define MAX_IMAGE_QUAD_COUNT			1000000
#define MAX_QUAD_MATCH_COUNT			1000000
#define MAX_STAR_MATCH_COUNT			1000
#define IMAGE_GRID_CELLS					10
#define MAX_DEPTH_STAGES					3

// the defaults of AGENT_PLATESOLVER_HINTS
#define DEFAULT_RADIUS						0
#define DEFAULT_EPOCH							2000
#define DEFAULT_SCALE							0
#define DEFAULT_PARITY						0
#define DEFAULT_DOWNSAMPLE				2
#define DEFAULT_DEPTH							0
#define DEFAULT_CPU_LIMIT					180

typedef enum {
	FORMAT_TEXT,
	FORMAT_JSON,
	FORMAT_XML,
	FORMAT_WCS
} output_format;

static const char *format_names[] = { "text", "json", "xml", "wcs" };

// besides solving an image the tool lists, downloads and removes the indexes
typedef enum {
	MODE_SOLVE,
	MODE_LIST,
	MODE_DOWNLOAD,
	MODE_REMOVE
} tool_mode;

static const char *mode_options[] = { NULL, "--list", "--download", "--remove" };

typedef struct {
	char index_dir[PATH_MAX];
	float fovs[INDIGO_SOLVER_INDEX_FOV_COUNT];
	int fov_count;
	double radius;
	double ra;
	double dec;
	bool ra_set;
	bool dec_set;
	double epoch;
	double scale;
	int parity;
	int downsample;
	int depth;
	double cpu_limit;
	int verbose;
	output_format format;
	const char *file_name;
	tool_mode mode;
	const char *index_url;
	const char *solving_option;
} solver_options;

// The outcome of a run: the solution, or the reason the image was not solved
typedef struct {
	bool solved;
	const char *message;
	double time;
	int image_width, image_height;
	int matched, star_match_count;
	double rms;
	double ra, dec;
	double ra_j2000, dec_j2000;
	double angle;
	double scale;
	int parity;
	char index[128];
} solver_result;

// the progress of -v goes to stderr with a structured format, so stdout holds the document alone
static FILE *progress;

static void default_index_dir(char *path, size_t size) {
	char *env = getenv("INDIGO_CACHE_BASE");
	if (env) {
		snprintf(path, size, "%s/solver", env);
	} else {
		snprintf(path, size, "%s/.indigo/solver", indigo_uni_home_folder());
	}
}

static void print_usage(const char *name) {
	char index_dir[PATH_MAX];
	default_index_dir(index_dir, sizeof(index_dir));
	printf("INDIGO Solver - plate solves an image file with the indexes of the Platesolver Agent\n\n");
	printf("Usage: %s [options] image_file\n", name);
	printf("       %s [-i <dir>] -l | --download <scales> | --remove <scales>\n\n", name);
	printf("The image can be INDIGO RAW, FITS, JPEG or a camera raw file.\n\n");
	printf("Options:\n");
	printf("  -i, --index-dir <dir>     directory with the index files (default: %s)\n", index_dir);
	printf("  -x, --index <scales>      comma separated quad scales of the indexes to use, in degrees (default: all installed of");
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		printf("%s%g", i ? "," : " ", indigo_solver_index_fovs[i]);
	}
	printf(")\n");
	printf("  -s, --scale <deg>         pixel scale hint in degrees per pixel, 0 = unknown (default: %g)\n", (double)DEFAULT_SCALE);
	printf("  -r, --ra <hours>          RA hint in hours, decimal or h:m:s (default: none)\n");
	printf("  -d, --dec <deg>           Dec hint in degrees, decimal or d:m:s (default: none)\n");
	printf("  -R, --radius <deg>        search radius around the RA/Dec hint in degrees, 0 = whole sky (default: %g)\n", (double)DEFAULT_RADIUS);
	printf("  -e, --epoch <epoch>       epoch of the hint and the solution, 2000 = J2000, 0 = JNow (default: %g)\n", (double)DEFAULT_EPOCH);
	printf("  -p, --parity <parity>     accept only solutions of this parity, -1, 1 or 0 = any (default: %d)\n", DEFAULT_PARITY);
	printf("  -D, --downsample <n>      bin the image n x n before the star detection, 1 = none (default: %d)\n", DEFAULT_DOWNSAMPLE);
	printf("  -t, --depth <n>           try the n brightest stars first and then all, 0 = 40, 100 and all (default: %d)\n", DEFAULT_DEPTH);
	printf("  -c, --cpu-limit <sec>     give up after this many seconds, 0 = no limit (default: %g)\n", (double)DEFAULT_CPU_LIMIT);
	printf("  -f, --format <format>     output format of the result, text, json, xml or wcs (default: text)\n");
	printf("  -v, --verbose             report the progress and errors, -vv adds the debug log\n");
	printf("  -h, --help                show this help\n\n");
	printf("Index management:\n");
	printf("  -l, --list                list the published indexes and the ones installed in the index directory\n");
	printf("      --download <scales>   download the indexes of the comma separated quad scales or all, valid\n");
	printf("                            installed indexes are kept, invalid ones replaced\n");
	printf("      --remove <scales>     remove the installed indexes of the comma separated quad scales or all\n");
	printf("      --index-url <url>     download the indexes from this URL (default:\n");
	printf("                            %s)\n\n", INDIGO_SOLVER_INDEX_URL);
	printf("With json or xml the result, solved or not, is a single document on the standard output and the progress\n");
	printf("of -v goes to the standard error.\n\n");
	printf("With wcs the solution is a WCS FITS header on the standard output, as the .wcs files of Astrometry.net and\n");
	printf("ASTAP, always in J2000; nothing is written to the standard output when the image is not solved.\n\n");
	printf("The exit code is 0 when the image is solved, 1 when it is not and 2 on wrong arguments. With --list,\n");
	printf("--download and --remove it is 0 on success, 1 when an index can't be downloaded or removed or the\n");
	printf("download is interrupted and 2 on wrong arguments.\n");
}

static bool parse_number(const char *string, double *value) {
	char *end;
	errno = 0;
	*value = strtod(string, &end);
	return errno == 0 && end != string && *end == 0 && isfinite(*value);
}

static bool parse_integer(const char *string, int *value) {
	double number;
	if (!parse_number(string, &number) || number != floor(number) || number < INT_MIN || number > INT_MAX) {
		return false;
	}
	*value = (int)number;
	return true;
}

// decimal or sexagesimal (h:m:s, d:m:s, d:m)
static bool parse_sexagesimal(const char *string, double *value) {
	bool digits = false;
	for (const char *c = string; *c; c++) {
		if (*c >= '0' && *c <= '9') {
			digits = true;
		} else if (strchr("+-.:", *c) == NULL) {
			return false;
		}
	}
	if (!digits) {
		return false;
	}
	*value = indigo_stod(string);
	return isfinite(*value);
}

static bool parse_fovs(const char *string, solver_options *options) {
	if (!strcmp(string, "all")) {
		for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
			options->fovs[i] = indigo_solver_index_fovs[i];
		}
		options->fov_count = INDIGO_SOLVER_INDEX_FOV_COUNT;
		return true;
	}
	char copy[256];
	snprintf(copy, sizeof(copy), "%s", string);
	options->fov_count = 0;
	for (char *token = strtok(copy, ","); token != NULL; token = strtok(NULL, ",")) {
		double fov;
		bool known = false;
		if (parse_number(token, &fov)) {
			for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
				known = known || indigo_solver_index_fovs[i] == (float)fov;
			}
		}
		if (!known) {
			fprintf(stderr, "%s: unknown index scale '%s'\n", TOOL_NAME, token);
			return false;
		}
		bool listed = false;
		for (int i = 0; i < options->fov_count; i++) {
			listed = listed || options->fovs[i] == (float)fov;
		}
		if (!listed) {
			options->fovs[options->fov_count++] = (float)fov;
		}
	}
	return options->fov_count > 0;
}

static bool set_mode(solver_options *options, tool_mode mode) {
	if (options->mode != MODE_SOLVE) {
		fprintf(stderr, "%s: only one of --list, --download and --remove can be given\n", TOOL_NAME);
		return false;
	}
	options->mode = mode;
	return true;
}

static bool parse_arguments(int argc, char **argv, solver_options *options) {
	default_index_dir(options->index_dir, sizeof(options->index_dir));
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		options->fovs[i] = indigo_solver_index_fovs[i];
	}
	options->fov_count = INDIGO_SOLVER_INDEX_FOV_COUNT;
	options->radius = DEFAULT_RADIUS;
	options->epoch = DEFAULT_EPOCH;
	options->scale = DEFAULT_SCALE;
	options->parity = DEFAULT_PARITY;
	options->downsample = DEFAULT_DOWNSAMPLE;
	options->depth = DEFAULT_DEPTH;
	options->cpu_limit = DEFAULT_CPU_LIMIT;
	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
			options->verbose++;
			continue;
		}
		if (!strcmp(arg, "-vv")) {
			options->verbose += 2;
			continue;
		}
		if (!strcmp(arg, "-l") || !strcmp(arg, "--list")) {
			if (!set_mode(options, MODE_LIST)) {
				return false;
			}
			continue;
		}
		if (arg[0] != '-' || arg[1] == 0) {
			if (options->file_name != NULL) {
				fprintf(stderr, "%s: only one image file can be solved, '%s' and '%s' given\n", TOOL_NAME, options->file_name, arg);
				return false;
			}
			options->file_name = arg;
			continue;
		}
		if (i + 1 >= argc) {
			fprintf(stderr, "%s: unknown option or missing value for '%s'\n", TOOL_NAME, arg);
			return false;
		}
		const char *value = argv[++i];
		bool ok = true;
		if (!strcmp(arg, "-i") || !strcmp(arg, "--index-dir")) {
			snprintf(options->index_dir, sizeof(options->index_dir), "%s", value);
			continue;
		} else if (!strcmp(arg, "--download") || !strcmp(arg, "--remove")) {
			if (!set_mode(options, arg[2] == 'd' ? MODE_DOWNLOAD : MODE_REMOVE)) {
				return false;
			}
			ok = parse_fovs(value, options);
		} else if (!strcmp(arg, "--index-url")) {
			options->index_url = value;
			ok = value[0] != 0;
		} else if (!strcmp(arg, "-x") || !strcmp(arg, "--index")) {
			ok = parse_fovs(value, options);
		} else if (!strcmp(arg, "-s") || !strcmp(arg, "--scale")) {
			ok = parse_number(value, &options->scale) && options->scale >= 0 && options->scale <= 5;
		} else if (!strcmp(arg, "-r") || !strcmp(arg, "--ra")) {
			ok = options->ra_set = parse_sexagesimal(value, &options->ra) && options->ra >= 0 && options->ra < 24;
		} else if (!strcmp(arg, "-d") || !strcmp(arg, "--dec")) {
			ok = options->dec_set = parse_sexagesimal(value, &options->dec) && options->dec >= -90 && options->dec <= 90;
		} else if (!strcmp(arg, "-R") || !strcmp(arg, "--radius")) {
			ok = parse_number(value, &options->radius) && options->radius >= 0 && options->radius <= 360;
		} else if (!strcmp(arg, "-e") || !strcmp(arg, "--epoch")) {
			ok = parse_number(value, &options->epoch) && (options->epoch == 0 || options->epoch == 2000);
		} else if (!strcmp(arg, "-p") || !strcmp(arg, "--parity")) {
			ok = parse_integer(value, &options->parity) && options->parity >= -1 && options->parity <= 1;
		} else if (!strcmp(arg, "-D") || !strcmp(arg, "--downsample")) {
			ok = parse_integer(value, &options->downsample) && options->downsample >= 1 && options->downsample <= 16;
		} else if (!strcmp(arg, "-t") || !strcmp(arg, "--depth")) {
			ok = parse_integer(value, &options->depth) && options->depth >= 0 && options->depth <= 1000;
		} else if (!strcmp(arg, "-c") || !strcmp(arg, "--cpu-limit")) {
			ok = parse_number(value, &options->cpu_limit) && options->cpu_limit >= 0;
		} else if (!strcmp(arg, "-f") || !strcmp(arg, "--format")) {
			ok = false;
			for (int format = FORMAT_TEXT; format <= FORMAT_WCS; format++) {
				if (!strcmp(value, format_names[format])) {
					options->format = format;
					ok = true;
				}
			}
		} else {
			fprintf(stderr, "%s: unknown option '%s'\n", TOOL_NAME, arg);
			return false;
		}
		// every option but -i, -v, --index-url and the index management modes is a solving option
		if (arg[1] != '-' || (strcmp(arg, "--download") && strcmp(arg, "--remove") && strcmp(arg, "--index-url"))) {
			options->solving_option = arg;
		}
		if (!ok) {
			fprintf(stderr, "%s: invalid value '%s' for '%s', see '%s -h'\n", TOOL_NAME, value, arg, TOOL_NAME);
			return false;
		}
	}
	if (options->index_url != NULL && options->mode != MODE_DOWNLOAD) {
		fprintf(stderr, "%s: --index-url can be used with --download only\n", TOOL_NAME);
		return false;
	}
	if (options->mode != MODE_SOLVE) {
		if (options->file_name != NULL) {
			fprintf(stderr, "%s: an image file can't be solved with %s\n", TOOL_NAME, mode_options[options->mode]);
			return false;
		}
		if (options->solving_option != NULL) {
			fprintf(stderr, "%s: '%s' can't be used with %s\n", TOOL_NAME, options->solving_option, mode_options[options->mode]);
			return false;
		}
		return true;
	}
	if (options->file_name == NULL) {
		fprintf(stderr, "%s: no image file given, see '%s -h'\n", TOOL_NAME, TOOL_NAME);
		return false;
	}
	if (options->radius > 0 && !(options->ra_set && options->dec_set)) {
		fprintf(stderr, "%s: the search radius needs both the RA and the Dec hint\n", TOOL_NAME);
		return false;
	}
	return true;
}

static void *read_file(const char *file_name, long *size) {
	FILE *file = fopen(file_name, "rb");
	if (file == NULL) {
		fprintf(stderr, "%s: can't open '%s': %s\n", TOOL_NAME, file_name, strerror(errno));
		return NULL;
	}
	void *data = NULL;
	if (fseek(file, 0, SEEK_END) == 0 && (*size = ftell(file)) > 0 && fseek(file, 0, SEEK_SET) == 0) {
		data = indigo_safe_malloc(*size);
		if (fread(data, 1, *size, file) != (size_t)*size) {
			indigo_safe_free(data);
			data = NULL;
		}
	}
	if (data == NULL) {
		fprintf(stderr, "%s: can't read '%s'\n", TOOL_NAME, file_name);
	}
	fclose(file);
	return data;
}

// Orders the installed indexes like the agent orders the selected ones: the one fitting the field size first, when
// the scale is known, then the others nearest to the field size first; without the scale the largest index first
static int select_candidate_fovs(solver_options *options, double scale, int width, int height, float *candidates) {
	float installed[INDIGO_SOLVER_INDEX_FOV_COUNT];
	int installed_count = 0;
	for (int i = 0; i < options->fov_count; i++) {
		char path[PATH_MAX];
		indigo_solver_index_path(path, sizeof(path), options->index_dir, options->fovs[i]);
		if (indigo_uni_is_readable(path)) {
			installed[installed_count++] = options->fovs[i];
		} else if (options->verbose) {
			fprintf(progress, "Index %s is not installed\n", path);
		}
	}
	int count = 0;
	if (scale > 0 && installed_count > 0) {
		char path[PATH_MAX];
		float fov;
		if (indigo_solver_select_index(options->index_dir, installed, installed_count, width * scale * DEG_TO_RAD, height * scale * DEG_TO_RAD, path, sizeof(path), &fov)) {
			candidates[count++] = fov;
		}
	}
	float target = scale > 0 ? INDIGO_SOLVER_INDEX_FIELD_RATIO * sqrt((double)width * height) * scale : 0;
	while (true) {
		float best = 0, best_distance = 0;
		for (int i = 0; i < installed_count; i++) {
			bool listed = false;
			for (int j = 0; j < count; j++) {
				listed = listed || candidates[j] == installed[i];
			}
			if (listed) {
				continue;
			}
			float distance = target > 0 ? fabs(log(installed[i] / target)) : -installed[i];
			if (best == 0 || distance < best_distance) {
				best = installed[i];
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

// The image star counts of the progressive stages, the same as in the agent
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

static void solve(solver_options *options, const void *data, long size, solver_result *result) {
	double start = indigo_monotonic_time();
	const char *message = NULL;
	bool solved = false;
	indigo_raw_header *image = NULL;
	indigo_solver_star *image_stars = NULL;
	indigo_solver_quad *image_quads = NULL;
	indigo_solver_quad_match *quad_matches = NULL;
	indigo_solver_star_match *star_matches = NULL;
	indigo_solver_catalog catalog = { 0 };
	indigo_solver_limits limits = { 0 };
	if (options->cpu_limit > 0) {
		limits.deadline = start + options->cpu_limit;
	}
	bool bayered = false;
	int bin, full_width, full_height;
	image = indigo_solver_load_image(data, size, options->downsample, &bin, &full_width, &full_height, &bayered, &message);
	if (image == NULL) {
		goto cleanup;
	}
	result->image_width = full_width;
	result->image_height = full_height;
	if (options->verbose) {
		fprintf(progress, "Image %d x %d%s, binned %d x %d\n", full_width, full_height, bayered ? " Bayer mosaic" : "", bin, bin);
	}
	int width = image->width;
	int height = image->height;
	image_stars = indigo_safe_malloc(MAX_IMAGE_STAR_COUNT * sizeof(indigo_solver_star));
	int star_count = indigo_solver_image_stars(image, image_stars, MAX_IMAGE_STAR_COUNT);
	if (options->verbose) {
		fprintf(progress, "%d stars detected\n", star_count);
	}
	if (star_count < 4) {
		message = "Not enough stars detected";
		goto cleanup;
	}
	image_quads = indigo_safe_malloc(MAX_IMAGE_QUAD_COUNT * sizeof(indigo_solver_quad));
	int grid = (width > height ? width : height) / IMAGE_GRID_CELLS;
	int stages[MAX_DEPTH_STAGES];
	int stage_count = depth_stages(options->depth, star_count, stages);
	float candidates[INDIGO_SOLVER_INDEX_FOV_COUNT];
	int candidate_count = select_candidate_fovs(options, options->scale, full_width, full_height, candidates);
	if (candidate_count == 0) {
		message = "No index files found, download them with --download or the Platesolver Agent, or select another index directory";
		goto cleanup;
	}
	double hint_ra = options->ra, hint_dec = options->dec;
	if (options->radius > 0 && options->epoch == 0) {
		indigo_jnow_to_j2k(&hint_ra, &hint_dec);
	}
	quad_matches = indigo_safe_malloc(MAX_QUAD_MATCH_COUNT * sizeof(indigo_solver_quad_match));
	star_matches = indigo_safe_malloc(MAX_STAR_MATCH_COUNT * sizeof(indigo_solver_star_match));
	float ra = 0, dec = 0, rotation = 0, pixel = 0, fov = 0, rms = 0;
	int parity = 0, matched = 0, star_match_count = 0;
	bool wrong_parity = false;
	// The indexes are searched stage by stage: the first stage on every index, then the next stage on every index, so
	// an index that does not fit the field costs only its cheap first stage before the others are tried. With the
	// scale every stage starts with the index fitting the field; without it the first stage starts with the largest
	// index, as the larger indexes are the smaller files and fail fast, and every next stage runs in the opposite
	// order, so the index loaded last is used again without loading it
	float loaded_fov = 0;
	for (int pass = 0; pass < stage_count && !solved && !limits.exceeded; pass++) {
		int first_stage = pass;
		int last_stage = pass + 1;
		for (int k = 0; k < candidate_count && !solved && !limits.exceeded; k++) {
			int i = options->scale == 0 && pass % 2 ? candidate_count - 1 - k : k;
			char path[PATH_MAX];
			indigo_solver_index_path(path, sizeof(path), options->index_dir, candidates[i]);
			if (options->verbose) {
				char label[128];
				indigo_solver_index_label(candidates[i], label, sizeof(label));
				fprintf(progress, "Solving with index %s...\n", label);
			}
			if (loaded_fov != candidates[i]) {
				indigo_solver_free_catalog(&catalog);
				loaded_fov = 0;
				if (!indigo_solver_load_index(path, &catalog)) {
					fprintf(stderr, "%s: failed to load %s\n", TOOL_NAME, path);
					continue;
				}
				loaded_fov = candidates[i];
			}
			// the catalog restricted to the hint cone: the field centre is within the radius, its stars within the
			// radius plus half the field diagonal (or the index scale when the pixel scale is unknown); a quad needs
			// four stars within the cone
			const indigo_solver_catalog *quads = &catalog;
			indigo_solver_catalog cone = { 0 };
			bool usable = catalog.quad_count > 0;
			if (options->radius > 0) {
				double extent = options->scale > 0 ? 0.5 * sqrt((double)full_width * full_width + (double)full_height * full_height) * options->scale : candidates[i];
				int cone_star_count = indigo_solver_cone_catalog(&catalog, hint_ra * 15 * DEG_TO_RAD, hint_dec * DEG_TO_RAD, (options->radius + extent) * DEG_TO_RAD, &cone);
				if (options->verbose) {
					fprintf(progress, "%d catalog stars within %.2f° of the position hint\n", cone_star_count, options->radius + extent);
				}
				usable = usable && cone_star_count >= 4;
				quads = &cone;
			}
			int quad_count = quads->quad_count;
			for (int stage = first_stage; stage < last_stage && !solved && usable && !limits.exceeded; stage++) {
				int used_star_count = 0;
				int image_quad_count = indigo_solver_build_image_quads(image_stars, stages[stage], width, height, grid, &used_star_count, image_quads, MAX_IMAGE_QUAD_COUNT);
				if (image_quad_count == 0) {
					continue;
				}
				if (options->verbose) {
					fprintf(progress, "Stage %d: %d brightest stars, %d quads, %d catalog quads\n", stage + 1, stages[stage], image_quad_count, quad_count);
				}
				int quad_match_count = indigo_solver_match_quads(image_quads, image_quad_count, quads, quad_matches, MAX_QUAD_MATCH_COUNT, &limits);
				star_match_count = indigo_solver_match_stars(quad_matches, quad_match_count, star_matches, MAX_STAR_MATCH_COUNT, &limits);
				if (limits.exceeded) {
					break;
				}
				if (indigo_solver_compute_center(star_matches, star_match_count, width, height, &ra, &dec, &rotation, &pixel, &parity, &matched, &rms)) {
					if (options->parity != 0 && parity != options->parity) {
						if (options->verbose) {
							fprintf(progress, "Solution with parity %d rejected by the parity hint %d\n", parity, options->parity);
						}
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
		message = limits.exceeded ? limits.reason : wrong_parity ? "Solution has the wrong parity" : "No solution found";
		goto cleanup;
	}
	// RA in hours, Dec and angles in degrees, scale in degrees per unbinned pixel, as in AGENT_PLATESOLVER_WCS
	double ra_hours = ra * RAD_TO_DEG / 15;
	double dec_degrees = dec * RAD_TO_DEG;
	if (options->epoch == 0) {
		indigo_j2k_to_jnow(&ra_hours, &dec_degrees);
	}
	result->matched = matched;
	result->star_match_count = star_match_count;
	result->rms = rms * RAD_TO_DEG * 3600;
	result->ra = ra_hours;
	result->dec = dec_degrees;
	result->ra_j2000 = ra * RAD_TO_DEG / 15;
	result->dec_j2000 = dec * RAD_TO_DEG;
	result->angle = rotation * RAD_TO_DEG;
	result->scale = pixel * RAD_TO_DEG / bin;
	result->parity = parity;
	indigo_solver_index_label(fov, result->index, sizeof(result->index));
cleanup:
	indigo_solver_free_catalog(&catalog);
	indigo_safe_free(star_matches);
	indigo_safe_free(quad_matches);
	indigo_safe_free(image_quads);
	indigo_safe_free(image_stars);
	indigo_safe_free(image);
	result->solved = solved;
	result->message = solved ? NULL : message ? message : "Unknown error";
	result->time = indigo_monotonic_time() - start;
}

static void print_text(solver_options *options, solver_result *result) {
	if (!result->solved) {
		return;
	}
	char ra_string[32], dec_string[32];
	indigo_dtos_r(result->ra, "%d:%02d:%05.2f", ra_string, sizeof(ra_string));
	indigo_dtos_r(result->dec, "%d:%02d:%04.1f", dec_string, sizeof(dec_string));
	printf("Solved in %.2f s: %d of %d stars matched, rms %.2f\"\n", result->time, result->matched, result->star_match_count, result->rms);
	printf("RA:       %.6f h (%s)\n", result->ra, ra_string);
	printf("Dec:      %+.6f° (%s%s)\n", result->dec, result->dec < 0 ? "" : "+", dec_string);
	printf("Epoch:    %s\n", options->epoch == 0 ? "JNow" : "J2000");
	printf("Angle:    %.4f°\n", result->angle);
	printf("Scale:    %.8f°/pixel (%.3f\"/pixel)\n", result->scale, result->scale * 3600);
	printf("Field:    %.4f° x %.4f°\n", result->image_width * result->scale, result->image_height * result->scale);
	printf("Parity:   %d\n", result->parity);
	printf("Index:    %s\n", result->index);
}

// The file name for a document: control characters and bytes that are not valid UTF-8 become '?', as the escape
// functions keep them and they make a JSON or XML document invalid
static const char *document_file_name(const char *file_name) {
	static char name[PATH_MAX];
	const unsigned char *in = (const unsigned char *)file_name;
	size_t length = 0;
	while (*in && length < sizeof(name) - 1) {
		int extra = *in < 0x80 ? 0 : (*in & 0xE0) == 0xC0 && *in >= 0xC2 ? 1 : (*in & 0xF0) == 0xE0 ? 2 : (*in & 0xF8) == 0xF0 && *in <= 0xF4 ? 3 : -1;
		bool valid = extra >= 0;
		for (int i = 1; valid && i <= extra; i++) {
			valid = (in[i] & 0xC0) == 0x80;
		}
		if (!valid || length + extra >= sizeof(name) - 1) {
			name[length++] = '?';
			in++;
		} else if (extra == 0) {
			name[length++] = *in < 0x20 || *in == 0x7F ? '?' : *in;
			in++;
		} else {
			memcpy(name + length, in, extra + 1);
			length += extra + 1;
			in += extra + 1;
		}
	}
	name[length] = 0;
	return name;
}

// The string as JSON string content or XML text: the quotes, the backslash, newline, carriage return and tab are
// escaped for JSON, the five predefined entities for XML; the result is valid until the next call
static const char *escape(const char *string, bool xml) {
	static char *buffer = NULL;
	static size_t buffer_size = 0;
	size_t size = 6 * strlen(string) + 1;
	if (size > buffer_size) {
		buffer = indigo_safe_realloc(buffer, size);
		buffer_size = size;
	}
	char *out = buffer;
	for (const char *in = string; *in; in++) {
		const char *replacement = NULL;
		if (xml) {
			switch (*in) {
				case '&':
					replacement = "&amp;";
					break;
				case '<':
					replacement = "&lt;";
					break;
				case '>':
					replacement = "&gt;";
					break;
				case '"':
					replacement = "&quot;";
					break;
				case '\'':
					replacement = "&apos;";
					break;
			}
		} else {
			switch (*in) {
				case '"':
					replacement = "\\\"";
					break;
				case '\\':
					replacement = "\\\\";
					break;
				case '\n':
					replacement = "\\n";
					break;
				case '\r':
					replacement = "\\r";
					break;
				case '\t':
					replacement = "\\t";
					break;
			}
		}
		if (replacement == NULL) {
			*out++ = *in;
		} else {
			size_t length = strlen(replacement);
			memcpy(out, replacement, length);
			out += length;
		}
	}
	*out = 0;
	return buffer;
}

static const char *json_escape(const char *string) {
	return escape(string, false);
}

static const char *xml_escape(const char *string) {
	return escape(string, true);
}

// RA in hours, Dec, angle and field in degrees, scale in degrees per pixel, rms in arcseconds, time in seconds
static void print_json(solver_options *options, solver_result *result) {
	printf("{\n");
	printf("  \"file\": \"%s\",\n", json_escape(document_file_name(options->file_name)));
	printf("  \"solved\": %s,\n", result->solved ? "true" : "false");
	printf("  \"time\": %.3f,\n", result->time);
	if (result->solved) {
		char ra_string[32], dec_string[32];
		indigo_dtos_r(result->ra, "%d:%02d:%05.2f", ra_string, sizeof(ra_string));
		indigo_dtos_r(result->dec, "%d:%02d:%04.1f", dec_string, sizeof(dec_string));
		printf("  \"ra\": %.8f,\n", result->ra);
		printf("  \"ra_hms\": \"%s\",\n", ra_string);
		printf("  \"dec\": %.8f,\n", result->dec);
		printf("  \"dec_dms\": \"%s%s\",\n", result->dec < 0 ? "" : "+", dec_string);
		printf("  \"epoch\": \"%s\",\n", options->epoch == 0 ? "JNow" : "J2000");
		printf("  \"angle\": %.6f,\n", result->angle);
		printf("  \"scale\": %.10f,\n", result->scale);
		printf("  \"field_width\": %.6f,\n", result->image_width * result->scale);
		printf("  \"field_height\": %.6f,\n", result->image_height * result->scale);
		printf("  \"image_width\": %d,\n", result->image_width);
		printf("  \"image_height\": %d,\n", result->image_height);
		printf("  \"parity\": %d,\n", result->parity);
		printf("  \"matched_stars\": %d,\n", result->matched);
		printf("  \"star_matches\": %d,\n", result->star_match_count);
		printf("  \"rms\": %.3f,\n", result->rms);
		printf("  \"index\": \"%s\"\n", json_escape(result->index));
	} else {
		printf("  \"message\": \"%s\"\n", json_escape(result->message));
	}
	printf("}\n");
}

// The same values as the JSON document, one element each
static void print_xml(solver_options *options, solver_result *result) {
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<solution>\n");
	printf("  <file>%s</file>\n", xml_escape(document_file_name(options->file_name)));
	printf("  <solved>%s</solved>\n", result->solved ? "true" : "false");
	printf("  <time>%.3f</time>\n", result->time);
	if (result->solved) {
		char ra_string[32], dec_string[32];
		indigo_dtos_r(result->ra, "%d:%02d:%05.2f", ra_string, sizeof(ra_string));
		indigo_dtos_r(result->dec, "%d:%02d:%04.1f", dec_string, sizeof(dec_string));
		printf("  <ra>%.8f</ra>\n", result->ra);
		printf("  <ra_hms>%s</ra_hms>\n", ra_string);
		printf("  <dec>%.8f</dec>\n", result->dec);
		printf("  <dec_dms>%s%s</dec_dms>\n", result->dec < 0 ? "" : "+", dec_string);
		printf("  <epoch>%s</epoch>\n", options->epoch == 0 ? "JNow" : "J2000");
		printf("  <angle>%.6f</angle>\n", result->angle);
		printf("  <scale>%.10f</scale>\n", result->scale);
		printf("  <field_width>%.6f</field_width>\n", result->image_width * result->scale);
		printf("  <field_height>%.6f</field_height>\n", result->image_height * result->scale);
		printf("  <image_width>%d</image_width>\n", result->image_width);
		printf("  <image_height>%d</image_height>\n", result->image_height);
		printf("  <parity>%d</parity>\n", result->parity);
		printf("  <matched_stars>%d</matched_stars>\n", result->matched);
		printf("  <star_matches>%d</star_matches>\n", result->star_match_count);
		printf("  <rms>%.3f</rms>\n", result->rms);
		printf("  <index>%s</index>\n", xml_escape(result->index));
	} else {
		printf("  <message>%s</message>\n", xml_escape(result->message));
	}
	printf("</solution>\n");
}

// A FITS header in memory: printable ASCII cards of 80 characters, no newlines, room for two 2880 byte blocks
typedef struct {
	char data[2 * 2880 + 1];
	int length;
} fits_header;

static void fits_text_card(fits_header *header, const char *text) {
	if (header->length + 80 < (int)sizeof(header->data)) {
		snprintf(header->data + header->length, sizeof(header->data) - header->length, "%-80.80s", text);
		header->length += 80;
	}
}

// One 80 character FITS header card: strings start in column 11, other values end in column 30, the comment is cut
// at the end of the card
static void fits_card(fits_header *header, const char *key, const char *value, const char *comment) {
	char card[128];
	int length = snprintf(card, sizeof(card), "%-8.8s", key);
	if (value != NULL) {
		length += snprintf(card + length, sizeof(card) - length, *value == '\'' ? "= %-20s" : "= %20s", value);
	}
	if (comment != NULL && length < 80) {
		snprintf(card + length, sizeof(card) - length, " / %s", comment);
	}
	fits_text_card(header, card);
}

static void fits_number_card(fits_header *header, const char *key, double value, const char *comment) {
	char string[32];
	snprintf(string, sizeof(string), "%.12G", value);
	fits_card(header, key, string, comment);
}

// The solution as a .wcs file of Astrometry.net and ASTAP on the standard output: a FITS header without data, padded
// to 2880 bytes, the same as Ain Platesolver writes; it has no newlines, so a text mode standard output keeps it. The
// TAN projection is centred on the image centre; x grows to the right and y with the rows as stored, i.e. FITS pixel
// coordinates of a FITS image. x maps to -RA and y to +Dec before the rotation, and the coordinates are J2000
// whatever the epoch option.
static void print_wcs(solver_options *options, solver_result *result) {
	fits_header header = { 0 };
	double angle = (result->angle + (result->parity == -1 ? 180 : 0)) * DEG_TO_RAD;
	double flip = result->parity == -1 ? -1 : 1;
	double scale = result->scale;
	char string[96];
	fits_card(&header, "SIMPLE", "T", "file does conform to FITS standard");
	fits_card(&header, "BITPIX", "8", "no image data, the WCS header only");
	fits_card(&header, "NAXIS", "0", NULL);
	fits_card(&header, "WCSAXES", "2", "number of WCS axes");
	fits_card(&header, "CTYPE1", "'RA---TAN'", "TAN (gnomonic) projection");
	fits_card(&header, "CTYPE2", "'DEC--TAN'", "TAN (gnomonic) projection");
	fits_card(&header, "CUNIT1", "'deg'", "unit of CRVAL1 and CD1_n");
	fits_card(&header, "CUNIT2", "'deg'", "unit of CRVAL2 and CD2_n");
	fits_card(&header, "RADESYS", "'ICRS'", "reference system of the catalogs");
	fits_card(&header, "EQUINOX", "2000.0", "equinox of the coordinates");
	fits_number_card(&header, "CRVAL1", result->ra_j2000 * 15, "RA of the reference pixel, degrees");
	fits_number_card(&header, "CRVAL2", result->dec_j2000, "Dec of the reference pixel, degrees");
	fits_number_card(&header, "CRPIX1", (result->image_width + 1) / 2.0, "X of the reference pixel, the image center");
	fits_number_card(&header, "CRPIX2", (result->image_height + 1) / 2.0, "Y of the reference pixel, the image center");
	fits_number_card(&header, "CD1_1", -scale * cos(angle), "transformation matrix, degrees per pixel");
	fits_number_card(&header, "CD1_2", scale * sin(angle) * flip, "transformation matrix, degrees per pixel");
	fits_number_card(&header, "CD2_1", scale * sin(angle), "transformation matrix, degrees per pixel");
	fits_number_card(&header, "CD2_2", scale * cos(angle) * flip, "transformation matrix, degrees per pixel");
	fits_number_card(&header, "IMAGEW", result->image_width, "image width, pixels");
	fits_number_card(&header, "IMAGEH", result->image_height, "image height, pixels");
	fits_card(&header, "PLTSOLVD", "T", "plate solved");
	time_t now = time(NULL);
	struct tm *utc = gmtime(&now);
	if (utc != NULL && strftime(string, sizeof(string), "'%Y-%m-%dT%H:%M:%S'", utc) > 0) {
		fits_card(&header, "DATE", string, "creation date of this file, UTC");
	}
	fits_card(&header, "CREATOR", "'" TOOL_NAME "'", "software that solved the image");
	// a header holds printable ASCII only, the image name is the last component of its path
	const char *name = options->file_name;
	for (const char *c = options->file_name; *c; c++) {
		if (*c == '/' || *c == '\\') {
			name = c + 1;
		}
	}
	int length = snprintf(string, sizeof(string), "COMMENT Solution of %s", name);
	for (int i = 0; i < length && i < (int)sizeof(string) - 1; i++) {
		if (string[i] < 0x20 || string[i] > 0x7E) {
			string[i] = '?';
		}
	}
	fits_text_card(&header, string);
	fits_card(&header, "END", NULL, NULL);
	while (header.length % 2880) {
		header.data[header.length++] = ' ';
	}
	fwrite(header.data, 1, header.length, stdout);
}

static void discard_log_message(indigo_log_levels level, const char *message) {
}

// -------------------------------------------------------------------------------- Index management

static indigo_solver_download download;

// the interrupt stops curl, which runs in its own process group, and the partial file is removed
static void interrupt_download(int sig) {
	indigo_solver_abort_download(&download, false);
}

static int index_position(float fov) {
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		if (indigo_solver_index_fovs[i] == fov) {
			return i;
		}
	}
	return 0;
}

static int list_indexes(solver_options *options) {
	printf("Index directory: %s\n", options->index_dir);
	for (int i = 0; i < INDIGO_SOLVER_INDEX_FOV_COUNT; i++) {
		float fov = indigo_solver_index_fovs[i];
		char path[PATH_MAX], label[128];
		indigo_solver_index_path(path, sizeof(path), options->index_dir, fov);
		indigo_solver_index_label(fov, label, sizeof(label));
		const char *state = "-";
		if (indigo_uni_is_readable(path)) {
			state = indigo_solver_validate_index(path, fov) ? "installed" : "invalid";
		}
		printf("  %-5g %-10s %s, %s\n", fov, state, label, indigo_solver_index_sizes[i]);
	}
	return 0;
}

static int download_indexes(solver_options *options) {
	if (!indigo_uni_mkdir(options->index_dir)) {
		fprintf(stderr, "%s: can't create the index directory '%s': %s\n", TOOL_NAME, options->index_dir, strerror(errno));
		return 1;
	}
	signal(SIGINT, interrupt_download);
	signal(SIGTERM, interrupt_download);
	bool failed = false;
	for (int i = 0; i < options->fov_count && !download.aborted; i++) {
		float fov = options->fovs[i];
		char path[PATH_MAX], label[128];
		indigo_solver_index_path(path, sizeof(path), options->index_dir, fov);
		indigo_solver_index_label(fov, label, sizeof(label));
		if (indigo_uni_is_readable(path)) {
			if (indigo_solver_validate_index(path, fov)) {
				printf("%s is already installed\n", label);
				continue;
			}
			printf("%s is invalid, it is replaced\n", label);
			if (!indigo_uni_remove(path)) {
				fprintf(stderr, "%s: %s can't be removed: %s\n", TOOL_NAME, path, strerror(errno));
				failed = true;
				continue;
			}
		}
		printf("Downloading %s, %s...\n", label, indigo_solver_index_sizes[index_position(fov)]);
		fflush(stdout);
		const char *message = NULL;
		if (indigo_solver_download_index(options->index_dir, options->index_url, fov, &download, &message)) {
			printf("Installed %s\n", path);
		} else if (!download.aborted) {
			fprintf(stderr, "%s: %s can't be downloaded: %s\n", TOOL_NAME, label, message);
			failed = true;
		}
	}
	if (download.aborted) {
		fprintf(stderr, "%s: the download was interrupted\n", TOOL_NAME);
		return 1;
	}
	return failed ? 1 : 0;
}

static int remove_indexes(solver_options *options) {
	bool failed = false;
	for (int i = 0; i < options->fov_count; i++) {
		float fov = options->fovs[i];
		char path[PATH_MAX], label[128];
		indigo_solver_index_path(path, sizeof(path), options->index_dir, fov);
		indigo_solver_index_label(fov, label, sizeof(label));
		if (!indigo_uni_is_readable(path)) {
			printf("%s is not installed\n", label);
		} else if (indigo_uni_remove(path)) {
			printf("Removed %s\n", path);
		} else {
			fprintf(stderr, "%s: %s can't be removed: %s\n", TOOL_NAME, path, strerror(errno));
			failed = true;
		}
	}
	return failed ? 1 : 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(TOOL_NAME);
			return 0;
		}
	}
	if (argc < 2) {
		print_usage(TOOL_NAME);
		return 2;
	}
	solver_options options = { 0 };
	if (!parse_arguments(argc, argv, &options)) {
		return 2;
	}
	// the library errors repeat the reason the tool reports, they are logged with -v only
	indigo_set_log_level(options.verbose > 1 ? INDIGO_LOG_DEBUG : INDIGO_LOG_ERROR);
	if (options.verbose == 0) {
		indigo_log_message_handler = discard_log_message;
	}
	switch (options.mode) {
		case MODE_LIST:
			return list_indexes(&options);
		case MODE_DOWNLOAD:
			return download_indexes(&options);
		case MODE_REMOVE:
			return remove_indexes(&options);
		default:
			break;
	}
	progress = options.format == FORMAT_TEXT ? stdout : stderr;
	solver_result result = { 0 };
	long size = 0;
	void *data = read_file(options.file_name, &size);
	if (data == NULL) {
		result.message = "Can't read the image file";
	} else {
		solve(&options, data, size, &result);
		indigo_safe_free(data);
		if (!result.solved) {
			fprintf(stderr, "Not solved: %s\n", result.message);
		}
	}
	switch (options.format) {
		case FORMAT_TEXT:
			print_text(&options, &result);
			break;
		case FORMAT_JSON:
			print_json(&options, &result);
			break;
		case FORMAT_XML:
			print_xml(&options, &result);
			break;
		case FORMAT_WCS:
			if (result.solved) {
				print_wcs(&options, &result);
			}
			break;
	}
	return result.solved ? 0 : 1;
}
