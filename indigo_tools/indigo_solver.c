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

/** INDIGO quad based plate solver - command line tool solving an image file with the indexes of the Solver agent
 \file indigo_solver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_timer.h>
#include <indigo/indigo_uni_io.h>

#include <agent_solver/indigo_solver.h>

#define TOOL_NAME									"indigo_solver"

// the same limits as the Solver agent
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
	const char *file_name;
} solver_options;

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
	printf("INDIGO Solver - plate solves an image file with the indexes of the Solver agent\n\n");
	printf("Usage: %s [options] image_file\n\n", name);
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
	printf("  -v, --verbose             report the progress and errors, -vv adds the debug log\n");
	printf("  -h, --help                show this help\n\n");
	printf("The exit code is 0 when the image is solved, 1 when it is not and 2 on wrong arguments.\n");
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
		} else {
			fprintf(stderr, "%s: unknown option '%s'\n", TOOL_NAME, arg);
			return false;
		}
		if (!ok) {
			fprintf(stderr, "%s: invalid value '%s' for '%s', see '%s -h'\n", TOOL_NAME, value, arg, TOOL_NAME);
			return false;
		}
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
			printf("Index %s is not installed\n", path);
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

static bool solve(solver_options *options, const void *data, long size) {
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
	image = indigo_solver_load_image(data, size, &bayered, &message);
	if (image == NULL) {
		goto cleanup;
	}
	int full_width = image->width;
	int full_height = image->height;
	int bin = options->downsample;
	if (bayered && (bin < 2 || bin % 2)) {
		bin = bin < 2 ? 2 : bin + 1;
	}
	if (options->verbose) {
		printf("Image %d x %d%s, binned %d x %d\n", full_width, full_height, bayered ? " Bayer mosaic" : "", bin, bin);
	}
	if (bin > 1) {
		indigo_solver_bin_image(image, bin);
	}
	int width = image->width;
	int height = image->height;
	image_stars = indigo_safe_malloc(MAX_IMAGE_STAR_COUNT * sizeof(indigo_solver_star));
	int star_count = indigo_solver_image_stars(image, image_stars, MAX_IMAGE_STAR_COUNT);
	if (options->verbose) {
		printf("%d stars detected\n", star_count);
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
		message = "No index files found, download them with the Solver agent or select another index directory";
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
				printf("Solving with index %s...\n", label);
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
					printf("%d catalog stars within %.2f° of the position hint\n", cone_star_count, options->radius + extent);
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
					printf("Stage %d: %d brightest stars, %d quads, %d catalog quads\n", stage + 1, stages[stage], image_quad_count, quad_count);
				}
				int quad_match_count = indigo_solver_match_quads(image_quads, image_quad_count, quads, quad_matches, MAX_QUAD_MATCH_COUNT, &limits);
				star_match_count = indigo_solver_match_stars(quad_matches, quad_match_count, star_matches, MAX_STAR_MATCH_COUNT, &limits);
				if (limits.exceeded) {
					break;
				}
				if (indigo_solver_compute_center(star_matches, star_match_count, width, height, &ra, &dec, &rotation, &pixel, &parity, &matched, &rms)) {
					if (options->parity != 0 && parity != options->parity) {
						if (options->verbose) {
							printf("Solution with parity %d rejected by the parity hint %d\n", parity, options->parity);
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
	double scale = pixel * RAD_TO_DEG / bin;
	char ra_string[32], dec_string[32];
	indigo_dtos_r(ra_hours, "%d:%02d:%05.2f", ra_string, sizeof(ra_string));
	indigo_dtos_r(dec_degrees, "%d:%02d:%04.1f", dec_string, sizeof(dec_string));
	printf("Solved in %.2f s: %d of %d stars matched, rms %.2f\"\n", indigo_monotonic_time() - start, matched, star_match_count, rms * RAD_TO_DEG * 3600);
	printf("RA:       %.6f h (%s)\n", ra_hours, ra_string);
	printf("Dec:      %+.6f° (%s%s)\n", dec_degrees, dec_degrees < 0 ? "" : "+", dec_string);
	printf("Epoch:    %s\n", options->epoch == 0 ? "JNow" : "J2000");
	printf("Angle:    %.4f°\n", rotation * RAD_TO_DEG);
	printf("Scale:    %.8f°/pixel (%.3f\"/pixel)\n", scale, scale * 3600);
	printf("Field:    %.4f° x %.4f°\n", full_width * scale, full_height * scale);
	printf("Parity:   %d\n", parity);
	char label[128];
	indigo_solver_index_label(fov, label, sizeof(label));
	printf("Index:    %s\n", label);
cleanup:
	indigo_solver_free_catalog(&catalog);
	indigo_safe_free(star_matches);
	indigo_safe_free(quad_matches);
	indigo_safe_free(image_quads);
	indigo_safe_free(image_stars);
	indigo_safe_free(image);
	if (!solved) {
		fprintf(stderr, "Not solved: %s\n", message);
	}
	return solved;
}

static void discard_log_message(indigo_log_levels level, const char *message) {
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
	long size = 0;
	void *data = read_file(options.file_name, &size);
	if (data == NULL) {
		return 1;
	}
	bool solved = solve(&options, data, size);
	indigo_safe_free(data);
	return solved ? 0 : 1;
}
