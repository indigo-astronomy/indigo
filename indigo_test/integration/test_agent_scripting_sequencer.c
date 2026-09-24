// Copyright (c) 2026 INDIGO initiative
// All rights reserved.
//
// You may use this software under the terms of 'INDIGO Astronomy
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

#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <indigo/indigo_driver.h>
#include <indigo_drivers/agent_guider/indigo_agent_guider.h>
#include <indigo_drivers/agent_imager/indigo_agent_imager.h>
#include <indigo_drivers/agent_mount/indigo_agent_mount.h>
#include <indigo_drivers/agent_scripting/indigo_agent_scripting.h>
#include <indigo_drivers/ccd_simulator/indigo_ccd_simulator.h>
#include <indigo_drivers/dome_simulator/indigo_dome_simulator.h>
#include <indigo_drivers/gps_simulator/indigo_gps_simulator.h>
#include <indigo_drivers/mount_simulator/indigo_mount_simulator.h>
#include <indigo_drivers/rotator_simulator/indigo_rotator_simulator.h>

#include "../test_runner.h"

#define SCRIPTING "Scripting Agent"
#define IMAGER "Imager Agent"
#define MOUNT "Mount Agent"
#define GUIDER "Guider Agent"
#define RUN "AGENT_SCRIPTING_RUN_SCRIPT"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); indigo_test_failures++; return false; } } while (0)

typedef struct {
	char device[INDIGO_NAME_SIZE];
	char name[INDIGO_NAME_SIZE];
	indigo_property *property;
	unsigned revision;
} observation;

static observation cache[4096];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static indigo_client observer;
static indigo_device *scripting_device;
static indigo_property *config_load_property, *drivers_property, *solver_exposure_property, *solver_target_property, *solver_process_property;
static char test_root[256], messages[131072];
static atomic_int message_count;
static atomic_int orchestration_changes;
static bool bus_started, observer_attached, peers_attached, ccd_started, mount_started, dome_started, gps_started, rotator_started, imager_started, mount_agent_started, guider_started, scripting_started;
static time_t fixed_time = 1704112496;

time_t sequencer_test_time(time_t *result) {
	if (result) {
		*result = fixed_time;
	}
	return fixed_time;
}

static indigo_result orchestration_enumerate(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_property *properties[3] = { NULL, NULL, NULL };
	if (!strcmp(device->name, "Configuration Agent")) {
		properties[0] = config_load_property;
	} else if (!strcmp(device->name, "Server")) {
		properties[0] = drivers_property;
	} else {
		properties[0] = solver_exposure_property;
		properties[1] = solver_target_property;
		properties[2] = solver_process_property;
	}
	for (int i = 0; i < 3; i++) {
		if (properties[i] && indigo_property_match(properties[i], property)) {
			indigo_define_property(device, properties[i], NULL);
		}
	}
	return INDIGO_OK;
}

static indigo_result orchestration_change(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_property *target = NULL;
	if (!strcmp(device->name, "Configuration Agent")) {
		target = config_load_property;
	} else if (!strcmp(device->name, "Server")) {
		target = drivers_property;
	} else if (!strcmp(property->name, "AGENT_PLATESOLVER_EXPOSURE")) {
		target = solver_exposure_property;
	} else if (!strcmp(property->name, "AGENT_PLATESOLVER_GOTO_SETTINGS")) {
		target = solver_target_property;
	} else if (!strcmp(property->name, "AGENT_START_PROCESS")) {
		target = solver_process_property;
	}
	if (indigo_property_match_changeable(target, property)) {
		indigo_property_copy_values(target, property, false);
		target->state = INDIGO_OK_STATE;
		atomic_fetch_add(&orchestration_changes, 1);
		indigo_update_property(device, target, NULL);
	}
	return INDIGO_OK;
}

static indigo_device configuration_peer = INDIGO_DEVICE_INITIALIZER("Configuration Agent", NULL, orchestration_enumerate, orchestration_change, NULL, NULL);
static indigo_device server_peer = INDIGO_DEVICE_INITIALIZER("Server", NULL, orchestration_enumerate, orchestration_change, NULL, NULL);
static indigo_device astrometry_peer = INDIGO_DEVICE_INITIALIZER("Astrometry Agent", NULL, orchestration_enumerate, orchestration_change, NULL, NULL);

static bool attach_orchestration_peers(void) {
	config_load_property = indigo_init_switch_property(NULL, configuration_peer.name, "AGENT_CONFIG_LOAD", "Test", "Load configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
	indigo_init_switch_item(config_load_property->items, "Config_A", "Config \"A\\B", false);
	drivers_property = indigo_init_switch_property(NULL, server_peer.name, "DRIVERS", "Test", "Drivers", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
	indigo_init_switch_item(drivers_property->items, "Driver_A", "Driver \"A\\B", false);
	solver_exposure_property = indigo_init_number_property(NULL, astrometry_peer.name, "AGENT_PLATESOLVER_EXPOSURE", "Test", "Exposure", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_number_item(solver_exposure_property->items, "EXPOSURE", "Exposure", 0, 60, 0, 1);
	solver_target_property = indigo_init_number_property(NULL, astrometry_peer.name, "AGENT_PLATESOLVER_GOTO_SETTINGS", "Test", "Target", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
	indigo_init_number_item(solver_target_property->items, "RA", "RA", 0, 24, 0, 0);
	indigo_init_number_item(solver_target_property->items + 1, "DEC", "Dec", -90, 90, 0, 0);
	solver_process_property = indigo_init_switch_property(NULL, astrometry_peer.name, "AGENT_START_PROCESS", "Test", "Process", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	indigo_init_switch_item(solver_process_property->items, "CENTER", "Center", false);
	indigo_init_switch_item(solver_process_property->items + 1, "PRECISE_GOTO", "Precise goto", false);
	REQUIRE(indigo_attach_device(&configuration_peer) == INDIGO_OK);
	REQUIRE(indigo_attach_device(&server_peer) == INDIGO_OK);
	REQUIRE(indigo_attach_device(&astrometry_peer) == INDIGO_OK);
	peers_attached = true;
	return true;
}

static observation *find_observation(const char *device, const char *name) {
	observation *free_entry = NULL;
	for (unsigned i = 0; i < ARRAY_SIZE(cache); i++) {
		if (!*cache[i].name) {
			if (!free_entry) {
				free_entry = cache + i;
			}
		} else if (!strcmp(cache[i].device, device) && !strcmp(cache[i].name, name)) {
			return cache + i;
		}
	}
	if (free_entry) {
		INDIGO_COPY_NAME(free_entry->device, device);
		INDIGO_COPY_NAME(free_entry->name, name);
	}
	return free_entry;
}

static indigo_result observe(indigo_device *device, indigo_property *property) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find_observation(property->device, property->name);
	if (entry) {
		indigo_release_property(entry->property);
		entry->property = indigo_copy_property(NULL, property);
		entry->revision++;
	}
	if (!strcmp(property->device, SCRIPTING)) {
		scripting_device = device;
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result defined(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property);
}

static indigo_result updated(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	return observe(device, property);
}

static indigo_result deleted(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	pthread_mutex_lock(&cache_mutex);
	for (unsigned i = 0; i < ARRAY_SIZE(cache); i++) {
		observation *entry = cache + i;
		if (*entry->name && !strcmp(entry->device, property->device) && (!*property->name || !strcmp(entry->name, property->name))) {
			indigo_release_property(entry->property);
			memset(entry, 0, sizeof(*entry));
		}
	}
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_result message_received(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	pthread_mutex_lock(&cache_mutex);
	if (message && strlen(messages) + strlen(message) + 2 < sizeof(messages)) {
		strcat(messages, message);
		strcat(messages, "\n");
	}
	message_count++;
	pthread_mutex_unlock(&cache_mutex);
	return INDIGO_OK;
}

static indigo_property *snapshot(const char *device, const char *name) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find_observation(device, name);
	indigo_property *result = entry && entry->property ? indigo_copy_property(NULL, entry->property) : NULL;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static unsigned revision(const char *device, const char *name) {
	pthread_mutex_lock(&cache_mutex);
	observation *entry = find_observation(device, name);
	unsigned result = entry ? entry->revision : 0;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static bool wait_state(const char *device, const char *name, unsigned before, indigo_property_state state, double timeout) {
	double deadline = indigo_monotonic_time() + timeout;
	while (indigo_monotonic_time() < deadline) {
		indigo_property *property = snapshot(device, name);
		bool ready = property && revision(device, name) > before && property->state == state;
		indigo_release_property(property);
		if (ready) {
			return true;
		}
		indigo_usleep(1000);
	}
	indigo_property *property = snapshot(device, name);
	fprintf(stderr, "Timeout waiting for %s.%s state %d (state %d, revision %u/%u)\n", device, name, state, property ? property->state : -1, revision(device, name), before);
	indigo_release_property(property);
	return false;
}

static void barrier_handler(indigo_device *device, void *data) {
	atomic_store((atomic_bool *)data, true);
}

static bool drain(void) {
	static atomic_bool reached;
	reached = false;
	indigo_execute_handler_with_data(scripting_device, barrier_handler, &reached);
	for (int i = 0; i < 5000 && !reached; i++) {
		indigo_usleep(1000);
	}
	REQUIRE(reached);
	return true;
}

static bool run_js_state(const char *script, indigo_property_state state) {
	unsigned before = revision(SCRIPTING, RUN);
	REQUIRE(indigo_change_text_property_1_raw(&observer, SCRIPTING, RUN, "SCRIPT", script) == INDIGO_OK);
	if (!wait_state(SCRIPTING, RUN, before, state, 10)) {
		pthread_mutex_lock(&cache_mutex);
		fprintf(stderr, "%s", messages);
		pthread_mutex_unlock(&cache_mutex);
		return false;
	}
	return drain();
}

static bool run_js(const char *script) {
	return run_js_state(script, INDIGO_OK_STATE);
}

static bool check_js(const char *expression) {
	size_t size = strlen(expression) + 128;
	char *script = indigo_safe_malloc(size);
	snprintf(script, size, "if (!(%s)) throw new Error('sequencer test assertion failed');", expression);
	bool result = run_js(script);
	if (!result) {
		fprintf(stderr, "JavaScript assertion failed: %s\n", expression);
	}
	free(script);
	return result;
}

static bool load_sequencer(void) {
	FILE *file = fopen(SEQUENCER_JS_PATH, "rb");
	REQUIRE(file != NULL);
	REQUIRE(fseek(file, 0, SEEK_END) == 0);
	long length = ftell(file);
	REQUIRE(length > 0);
	rewind(file);
	char *source = indigo_safe_malloc((size_t)length + 1);
	REQUIRE(fread(source, 1, (size_t)length, file) == (size_t)length);
	fclose(file);
	source[length] = 0;
	bool result = run_js(source);
	free(source);
	return result;
}

static bool change_switch(const char *device, const char *property, const char *item, bool value, indigo_property_state state) {
	unsigned before = revision(device, property);
	REQUIRE(indigo_change_switch_property_1(&observer, device, property, item, value) == INDIGO_OK);
	return wait_state(device, property, before, state, 20);
}

static bool wait_sequence(indigo_property_state state, double timeout) {
	double deadline = indigo_monotonic_time() + timeout;
	while (indigo_monotonic_time() < deadline) {
		indigo_property *property = snapshot(SCRIPTING, "SEQUENCE_STATE");
		bool ready = property && property->state == state;
		indigo_release_property(property);
		if (ready) {
			return true;
		}
		indigo_usleep(1000);
	}
	indigo_property *property = snapshot(SCRIPTING, "SEQUENCE_STATE");
	fprintf(stderr, "Timeout waiting for %s.SEQUENCE_STATE state %d (state %d)\n", SCRIPTING, state, property ? property->state : -1);
	indigo_release_property(property);
	property = snapshot(SCRIPTING, "SEQUENCE_STEP_STATE");
	if (property) {
		for (int i = 0; i < property->count; i++) {
			fprintf(stderr, "  step %s=%d\n", property->items[i].name, property->items[i].light.value);
		}
	}
	indigo_release_property(property);
	pthread_mutex_lock(&cache_mutex);
	fprintf(stderr, "%s", messages);
	pthread_mutex_unlock(&cache_mutex);
	return false;
}

static bool has_message(const char *text) {
	pthread_mutex_lock(&cache_mutex);
	bool result = strstr(messages, text) != NULL;
	pthread_mutex_unlock(&cache_mutex);
	return result;
}

static bool wait_exists(const char *device, const char *name, double timeout) {
	double deadline = indigo_monotonic_time() + timeout;
	while (indigo_monotonic_time() < deadline) {
		indigo_property *property = snapshot(device, name);
		if (property) {
			indigo_release_property(property);
			return true;
		}
		indigo_usleep(1000);
	}
	return false;
}

static bool setup(void) {
	indigo_set_log_level(getenv("INDIGO_TEST_DEBUG") ? INDIGO_LOG_DEBUG : INDIGO_LOG_ERROR);
	REQUIRE(indigo_start() == INDIGO_OK);
	bus_started = true;
	INDIGO_COPY_NAME(observer.name, "Sequencer integration observer");
	observer.version = INDIGO_VERSION_CURRENT;
	observer.define_property = defined;
	observer.update_property = updated;
	observer.delete_property = deleted;
	observer.send_message = message_received;
	REQUIRE(indigo_attach_client(&observer) == INDIGO_OK);
	observer_attached = true;
	REQUIRE(attach_orchestration_peers());
	REQUIRE(indigo_ccd_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	ccd_started = true;
	REQUIRE(indigo_mount_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	mount_started = true;
	REQUIRE(indigo_dome_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	dome_started = true;
	REQUIRE(indigo_gps_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	gps_started = true;
	REQUIRE(indigo_rotator_simulator(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	rotator_started = true;
	REQUIRE(indigo_agent_imager(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	imager_started = true;
	REQUIRE(indigo_agent_mount(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	mount_agent_started = true;
	REQUIRE(indigo_agent_guider(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	guider_started = true;
	REQUIRE(indigo_agent_scripting(INDIGO_DRIVER_INIT, NULL) == INDIGO_OK);
	scripting_started = true;
	REQUIRE(scripting_device != NULL);
	REQUIRE(load_sequencer());
	REQUIRE(wait_exists(SCRIPTING, "FLIPPER_STATE", 5));
	return true;
}

static void cleanup(void) {
	if (scripting_started) {
		indigo_agent_scripting(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (guider_started) {
		indigo_agent_guider(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (mount_agent_started) {
		indigo_agent_mount(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (imager_started) {
		indigo_agent_imager(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (rotator_started) {
		indigo_rotator_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (gps_started) {
		indigo_gps_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (dome_started) {
		indigo_dome_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (mount_started) {
		indigo_mount_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (ccd_started) {
		indigo_ccd_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	}
	if (peers_attached) {
		indigo_detach_device(&astrometry_peer);
		indigo_detach_device(&server_peer);
		indigo_detach_device(&configuration_peer);
	}
	if (observer_attached) {
		indigo_detach_client(&observer);
	}
	if (bus_started) {
		indigo_stop();
	}
	for (unsigned i = 0; i < ARRAY_SIZE(cache); i++) {
		indigo_release_property(cache[i].property);
	}
	indigo_release_property(drivers_property);
	indigo_release_property(config_load_property);
	indigo_release_property(solver_process_property);
	indigo_release_property(solver_target_property);
	indigo_release_property(solver_exposure_property);
}

static void library_inventory(void) {
	ASSERT_TRUE(check_js("typeof Sequence==='function'"));
	ASSERT_TRUE(check_js("(function(){var names=Object.getOwnPropertyNames(Sequence.prototype); if(names.length!==107) throw new Error('prototype count '+names.length); return true})()"));
	ASSERT_TRUE(check_js("indigo_event_handlers.indigo_sequencer===indigo_sequencer"));
	const char *properties[] = { "SEQUENCE_NAME", "SEQUENCE_STATE", "SEQUENCE_STEP_STATE", "AGENT_ABORT_PROCESS", "AGENT_PAUSE_PROCESS", "SEQUENCE_RESET", "FLIPPER_STATE" };
	for (unsigned i = 0; i < ARRAY_SIZE(properties); i++) {
		indigo_property *property = snapshot(SCRIPTING, properties[i]);
		ASSERT_TRUE(property != NULL);
		indigo_release_property(property);
	}
}

static void published_property_schema(void) {
	indigo_property *property = snapshot(SCRIPTING, "SEQUENCE_NAME");
	ASSERT_TRUE(property != NULL);
	ASSERT_EQ_INT(INDIGO_TEXT_VECTOR, property->type);
	ASSERT_EQ_INT(INDIGO_RO_PERM, property->perm);
	ASSERT_EQ_INT(1, property->count);
	indigo_release_property(property);
	property = snapshot(SCRIPTING, "SEQUENCE_STATE");
	ASSERT_TRUE(property != NULL);
	ASSERT_EQ_INT(INDIGO_NUMBER_VECTOR, property->type);
	ASSERT_EQ_INT(INDIGO_RO_PERM, property->perm);
	ASSERT_EQ_INT(5, property->count);
	indigo_release_property(property);
	property = snapshot(SCRIPTING, "SEQUENCE_STEP_STATE");
	ASSERT_TRUE(property != NULL);
	ASSERT_EQ_INT(INDIGO_LIGHT_VECTOR, property->type);
	ASSERT_EQ_INT(INDIGO_RO_PERM, property->perm);
	indigo_release_property(property);
	const char *controls[] = { "AGENT_ABORT_PROCESS", "AGENT_PAUSE_PROCESS", "SEQUENCE_RESET" };
	for (unsigned i = 0; i < ARRAY_SIZE(controls); i++) {
		property = snapshot(SCRIPTING, controls[i]);
		ASSERT_TRUE(property != NULL);
		ASSERT_EQ_INT(INDIGO_SWITCH_VECTOR, property->type);
		ASSERT_EQ_INT(INDIGO_RW_PERM, property->perm);
		ASSERT_EQ_INT(1, property->count);
		ASSERT_EQ_INT(INDIGO_ONE_OF_MANY_RULE, property->rule);
		indigo_release_property(property);
	}
	property = snapshot(SCRIPTING, "FLIPPER_STATE");
	ASSERT_TRUE(property != NULL);
	ASSERT_EQ_INT(INDIGO_SWITCH_VECTOR, property->type);
	ASSERT_EQ_INT(INDIGO_RO_PERM, property->perm);
	ASSERT_EQ_INT(7, property->count);
	ASSERT_EQ_INT(INDIGO_ANY_OF_MANY_RULE, property->rule);
	indigo_release_property(property);
}

static void all_public_constructors(void) {
	ASSERT_TRUE(run_js("publicMethods=Object.getOwnPropertyNames(Sequence.prototype).filter(function(n){return n!=='constructor'}); publicCalls={}; publicMethods.forEach(function(n){var original=Sequence.prototype[n]; Sequence.prototype[n]=function(){publicCalls[n]=(publicCalls[n]||0)+1; return original.apply(this,arguments)}}); var s=new Sequence('all'); s.enable_reset_loop_content_state(); s.disable_reset_loop_content_state(); s.repeat(1,function(){s.send_message('loop')}); s.enable_verbose(); s.disable_verbose(); s.recovery_point(); s.wait(0); s.wait_until(1704112495); s.continue_on_failure(); s.recover_on_failure(); s.abort_on_failure(); s.break_at(1704112495); s.break_at_ha('01:00:00'); s.wait_until_solar_altitude_below(91); s.wait_until_target_altitude_above(-91,1,2); s.break_if_solar_altitude_above(91); s.break_if_target_altitude_below(-91,1,2); s.resume_point(); s.evaluate('testValue=1'); s.send_message('message'); s.load_config('Config'); s.load_driver('Driver'); s.unload_driver('Driver'); s.select_imager_agent('Imager Agent'); s.select_mount_agent('Mount Agent'); s.select_guider_agent('Guider Agent'); s.select_imager_camera('CCD Imager Simulator'); s.select_filter_wheel('CCD Imager Simulator (wheel)'); s.select_focuser('CCD Imager Simulator (focuser)'); s.select_rotator('Field Rotator Simulator'); s.select_mount('Mount Simulator'); s.select_dome('Dome Simulator'); s.select_gps('GPS Simulator'); s.select_guider_camera('CCD Guider Simulator'); s.select_guider('CCD Guider Simulator (guider)'); s.select_frame_type('Light'); s.select_image_format('RAW'); s.set_frame(0,0,100,100); s.reset_frame(); s.select_camera_mode('BIN_1x1'); s.set_gain(10); s.set_offset(5); s.set_gamma(1); s.select_program('M'); s.select_aperture('f/4'); s.select_shutter('1/10'); s.select_iso('800'); s.enable_cooler(-10); s.disable_cooler(); s.enable_dithering(2,3,4); s.disable_dithering(); s.enable_meridian_flip(true,0); s.disable_meridian_flip(); s.enable_filter_offsets(); s.disable_filter_offsets(); s.set_fits_header('OBJECT','M42'); s.remove_fits_header('OBJECT'); s.select_filter('L'); s.set_directory('/tmp'); s.set_file_template('frame'); s.set_object_name('M42'); s.start_preview(0.1); s.stop_preview(); s.capture_batch(2,0.1); s.capture_batch('batch',2,0.1); s.capture_stream(2,0.1); s.capture_stream('stream',2,0.1); s.set_manual_focuser_mode(); s.set_automatic_focuser_mode(); s.focus(0.1); s.focus_ignore_failure(0.1); s.clear_focuser_selection(); s.set_focuser_position(100); s.slew(1,2); s.park(); s.unpark(); s.home(); s.enable_tracking(); s.disable_tracking(); s.dome_slew(90); s.dome_park(); s.dome_unpark(); s.dome_open(); s.dome_close(); s.enable_ha_limit(); s.disable_ha_limit(); s.enable_time_limit(); s.disable_time_limit(); s.enable_dome_slaving(); s.disable_dome_slaving(); s.make_dome_slaving_persistent(); s.make_dome_slaving_not_persistent(); s.enable_field_derotation(); s.disable_field_derotation(); s.make_field_derotation_persistent(); s.make_field_derotation_not_persistent(); s.enable_joystick_control(); s.disable_joystick_control(); s.wait_for_gps(); s.calibrate_guiding(); s.calibrate_guiding(0.1); s.start_guiding(); s.start_guiding(0.1); s.start_guiding_exposure(0.1); s.stop_guiding(); s.clear_guider_selection(); s.sync_center(0.1); s.precise_goto(0.1,1,2); s.set_rotator_angle(45); new Sequence('start').start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("publicMethods.length===106 && publicMethods.every(function(n){return publicCalls[n]>0}) && s.step>100 && s.progress>100 && s.exposure===0.8 && s.reset_loop_content_state===false"));
	ASSERT_TRUE(check_js("s.sequence.every(function(entry){var name=entry.execute.substring(0,entry.execute.indexOf('(')); return typeof indigo_sequencer[name]==='function' && Function('return function(){indigo_sequencer.'+entry.execute+'}')})"));
}

static void loop_and_timing_construction(void) {
	ASSERT_TRUE(run_js("var s=new Sequence(); s.repeat(2,function(){s.wait(0); s.repeat(2,function(){s.send_message('nested')})}); var withReset=s.sequence.length; var progress=s.progress; s.disable_reset_loop_content_state(); s.repeat(2,function(){s.wait(0)}); testLoop={withReset:withReset,progress:progress,total:s.sequence.length,steps:s.step};"));
	ASSERT_TRUE(check_js("testLoop.withReset===21 && testLoop.progress===6 && testLoop.total===26 && testLoop.steps===6"));
	ASSERT_TRUE(run_js("var z=new Sequence(); z.repeat(0,function(){z.send_message('never')}); testZero={length:z.sequence.length,step:z.step,progress:z.progress};"));
	ASSERT_TRUE(check_js("testZero.length===2 && testZero.step===1 && testZero.progress===0"));
}

static void nested_loop_execution(void) {
	ASSERT_TRUE(run_js("loopTrace=''; var s=new Sequence('loops'); s.repeat(2,function(){s.evaluate(\"loopTrace+='O'\"); s.repeat(2,function(){s.evaluate(\"loopTrace+='I'\")})}); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("loopTrace==='OIIOII'"));
	ASSERT_TRUE(snapshot(SCRIPTING, "LOOP_0") == NULL);
	ASSERT_TRUE(snapshot(SCRIPTING, "LOOP_1") == NULL);
}

static void capture_accounting(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('capture'); s.capture_batch(3,2); s.capture_batch('named',2,4); s.capture_stream(5,1); s.capture_stream('stream',2,3); testCapture={exposure:s.exposure,progress:s.progress,step:s.step,sequence:s.sequence};"));
	ASSERT_TRUE(check_js("testCapture.exposure===25 && testCapture.step===4 && testCapture.progress===14"));
	ASSERT_TRUE(check_js("testCapture.sequence[0].execute==='set_batch(3,2)' && testCapture.sequence[3].execute.indexOf('named')>=0 && testCapture.sequence[13].execute==='start_imager_process(\"STREAMING\")'"));
}

static void empty_sequence_execution(void) {
	ASSERT_TRUE(run_js("new Sequence('empty').start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	indigo_property *property = snapshot(SCRIPTING, "SEQUENCE_STATE");
	ASSERT_TRUE(property != NULL);
	ASSERT_EQ_INT(0, property->items[1].number.value);
	ASSERT_EQ_INT(0, property->items[2].number.value);
	indigo_release_property(property);
	ASSERT_TRUE(has_message("Sequence finished"));
}

static void evaluate_wait_and_messages(void) {
	ASSERT_TRUE(run_js("testSequenceValue=0; var s=new Sequence('flow'); s.disable_verbose(); s.evaluate('testSequenceValue=41'); s.enable_verbose(); s.wait(0.01); s.send_message('flow-message'); s.evaluate('testSequenceValue++'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("testSequenceValue===42"));
	ASSERT_TRUE(has_message("flow-message"));
}

static void string_arguments_and_utc_break(void) {
	ASSERT_TRUE(run_js("quotedValue=''; breakValue=''; var text='quote '+String.fromCharCode(34)+' slash '+String.fromCharCode(92); var s=new Sequence('quoted'); s.send_message(text); s.evaluate('quotedValue='+JSON.stringify(text)); s.break_at('2024-01-01 12:34:55'); s.evaluate('breakValue+=String.fromCharCode(65)'); s.resume_point(); s.evaluate('breakValue+=String.fromCharCode(66)'); testBreakStep=s.sequence[2].execute; s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("quotedValue===text && breakValue==='B' && testBreakStep==='break_at(1704112495)'"));
	ASSERT_TRUE(has_message("quote \" slash \\"));
}

static void failure_abort_policy(void) {
	ASSERT_TRUE(run_js("afterFailure=false; var s=new Sequence('abort failure'); s.select_imager_agent('Missing Agent'); s.select_frame_type('Light'); s.evaluate('afterFailure=true'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_ALERT_STATE, 5));
	ASSERT_TRUE(check_js("afterFailure===false"));
	ASSERT_TRUE(has_message("Sequence failed"));
}

static void failure_continue_policy(void) {
	ASSERT_TRUE(run_js("afterFailure=false; var s=new Sequence('continue failure'); s.continue_on_failure(); s.select_imager_agent('Missing Agent'); s.select_frame_type('Light'); s.evaluate('afterFailure=true'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("afterFailure===true"));
}

static void failure_recovery_policy(void) {
	ASSERT_TRUE(run_js("recoveryTrace=''; var s=new Sequence('recover failure'); s.recover_on_failure(); s.select_imager_agent('Missing Agent'); s.select_frame_type('Light'); s.evaluate(\"recoveryTrace+='S'\"); s.recovery_point(); s.evaluate(\"recoveryTrace+='R'\"); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("recoveryTrace==='R'"));
}

static void recovery_without_point(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('recover without point'); s.recover_on_failure(); s.select_imager_agent('Missing Agent'); s.select_frame_type('Light'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_ALERT_STATE, 5));
	ASSERT_TRUE(has_message("no recovery point found"));
}

static void pause_resume_abort_reset(void) {
	ASSERT_TRUE(run_js("pauseTrace=0; var s=new Sequence('pause'); s.wait(0.3); s.evaluate('pauseTrace=1'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_BUSY_STATE, 2));
	ASSERT_TRUE(change_switch(SCRIPTING, "AGENT_PAUSE_PROCESS", "PAUSE_WAIT", true, INDIGO_BUSY_STATE));
	indigo_usleep(500000);
	ASSERT_TRUE(check_js("pauseTrace===0"));
	ASSERT_TRUE(change_switch(SCRIPTING, "AGENT_PAUSE_PROCESS", "PAUSE_WAIT", false, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("pauseTrace===1"));
	ASSERT_TRUE(run_js("abortTrace=0; var s=new Sequence('abort'); s.wait(10); s.evaluate('abortTrace=1'); s.start();"));
	ASSERT_TRUE(change_switch(SCRIPTING, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_sequence(INDIGO_ALERT_STATE, 5));
	ASSERT_TRUE(check_js("abortTrace===0"));
	ASSERT_TRUE(change_switch(SCRIPTING, "SEQUENCE_RESET", "RESET", true, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 2));
	ASSERT_TRUE(change_switch(SCRIPTING, "AGENT_ABORT_PROCESS", "ABORT", true, INDIGO_ALERT_STATE));
}

static void past_waits_and_breaks(void) {
	ASSERT_TRUE(run_js("timeTrace=''; var s=new Sequence('past times'); s.wait_until('2024-01-01 12:34:55'); s.wait_until(1704112495); s.break_at(1704112495); s.evaluate(\"timeTrace+='A'\"); s.resume_point(); s.evaluate(\"timeTrace+='B'\"); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("timeTrace==='B'"));
	ASSERT_TRUE(has_message("Target time has passed"));
	ASSERT_TRUE(has_message("Break executed"));
}

static void simulator_imaging(void) {
	char script[4096];
	snprintf(script, sizeof(script), "var s=new Sequence('simulator imaging'); s.select_imager_camera('CCD Imager Simulator'); s.select_filter_wheel('CCD Imager Simulator (wheel)'); s.select_focuser('CCD Imager Simulator (focuser)'); s.select_frame_type('Light'); s.select_image_format('RAW'); s.select_camera_mode('BIN_1x1'); s.set_directory('%s'); s.set_file_template('seq_XXX'); s.set_object_name('Simulator'); s.set_fits_header('OBJECT','Simulator'); s.remove_fits_header('OBJECT'); s.set_frame(0,0,64,64); s.reset_frame(); s.set_gain(10); s.set_offset(5); s.set_gamma(1); s.enable_filter_offsets(); s.disable_filter_offsets(); s.enable_cooler(25); s.select_filter('1'); s.set_manual_focuser_mode(); s.set_focuser_position(1); s.set_automatic_focuser_mode(); s.clear_focuser_selection(); s.capture_batch(1,0.05); s.disable_cooler(); s.start();", test_root);
	ASSERT_TRUE(run_js(script));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 40));
	ASSERT_TRUE(has_message("Sequence finished"));
}

static void simulator_preview_and_stream(void) {
	char script[2048];
	snprintf(script, sizeof(script), "var s=new Sequence('preview'); s.select_imager_camera('CCD Imager Simulator'); s.select_image_format('RAW'); s.set_directory('%s'); s.start_preview(0.2); s.wait(0.5); s.stop_preview(); s.start();", test_root);
	ASSERT_TRUE(run_js(script));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 30));
}

static void simulator_stream(void) {
	char script[2048];
	snprintf(script, sizeof(script), "var s=new Sequence('stream'); s.select_imager_camera('CCD Imager Simulator'); s.select_image_format('RAW'); s.set_directory('%s'); s.capture_stream('stream_XXX',3,0.5); s.start();", test_root);
	ASSERT_TRUE(run_js(script));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 30));
}

static void simulator_mount_dome_gps_rotator(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('simulator mount'); s.select_mount('Mount Simulator'); s.select_dome('Dome Simulator'); s.select_gps('GPS Simulator'); s.select_rotator('Field Rotator Simulator'); s.unpark(); s.home(); s.enable_tracking(); s.slew(5,20); s.dome_unpark(); s.dome_open(); s.dome_slew(45); s.set_rotator_angle(10); s.wait_for_gps(); s.dome_close(); s.dome_park(); s.disable_tracking(); s.park(); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 60));
}

static void simulator_agent_features(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('agent features'); s.select_imager_camera('CCD Imager Simulator'); s.select_mount('Mount Simulator'); s.select_dome('Dome Simulator'); s.select_rotator('Field Rotator Simulator'); s.enable_dithering(1,1,1); s.disable_dithering(); s.enable_meridian_flip(false,0); s.disable_meridian_flip(); s.enable_ha_limit(); s.disable_ha_limit(); s.enable_time_limit(); s.disable_time_limit(); s.enable_dome_slaving(); s.disable_dome_slaving(); s.make_dome_slaving_persistent(); s.make_dome_slaving_not_persistent(); s.enable_field_derotation(); s.disable_field_derotation(); s.make_field_derotation_persistent(); s.make_field_derotation_not_persistent(); s.enable_joystick_control(); s.disable_joystick_control(); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 20));
}

// The simulator keeps a fixed RA, so its hour angle follows the real sidereal time; the HA break
// limit is therefore taken one hour behind the current HA to break at any time of day.
static void simulator_altitude_and_hour_angle(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('select mount'); s.select_mount('Mount Simulator'); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 15));
	ASSERT_TRUE(check_js("indigo_devices['Mount Agent'].AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY != null"));
	ASSERT_TRUE(run_js("altitudeTrace=''; var ha=indigo_devices['Mount Agent'].AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY.items.HA; var s=new Sequence('altitude'); s.select_mount('Mount Simulator'); s.select_gps('GPS Simulator'); s.wait_until_solar_altitude_below(91); s.wait_until_target_altitude_above(-91,5,20); s.break_if_solar_altitude_above(-91); s.evaluate(\"altitudeTrace+='S'\"); s.resume_point(); s.break_if_target_altitude_below(91,5,20); s.evaluate(\"altitudeTrace+='T'\"); s.resume_point(); s.break_at_ha(ha-1); s.evaluate(\"altitudeTrace+='H'\"); s.resume_point(); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 15));
	ASSERT_TRUE(check_js("altitudeTrace===''"));
}

static void simulator_guiding(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('simulator guiding'); s.select_guider_camera('CCD Guider Simulator'); s.select_guider('CCD Guider Simulator (guider)'); s.calibrate_guiding(0.05); s.start_guiding(0.05); s.wait(0.2); s.stop_guiding(); s.clear_guider_selection(); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 60));
}

static void custom_agent_arguments(void) {
	ASSERT_TRUE(run_js("var s=new Sequence('agents'); s.start('Imager Agent','Mount Agent','Guider Agent');"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_TRUE(check_js("indigo_sequencer.devices[2]==='Imager Agent' && indigo_sequencer.devices[3]==='Mount Agent' && indigo_sequencer.devices[4]==='Guider Agent'"));
}

static void configuration_and_driver_orchestration(void) {
	ASSERT_TRUE(run_js("var config='Config '+String.fromCharCode(34)+'A'+String.fromCharCode(92)+'B'; var driver='Driver '+String.fromCharCode(34)+'A'+String.fromCharCode(92)+'B'; var s=new Sequence('orchestration'); s.load_config(config); s.load_driver(driver); s.unload_driver(driver); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_EQ_INT(3, atomic_load(&orchestration_changes));
	indigo_property *property = snapshot("Server", "DRIVERS");
	ASSERT_TRUE(property != NULL);
	ASSERT_FALSE(property->items[0].sw.value);
	indigo_release_property(property);
}

static void astrometry_orchestration(void) {
	int before = atomic_load(&orchestration_changes);
	ASSERT_TRUE(run_js("var s=new Sequence('astrometry'); s.sync_center(0.2); s.precise_goto(0.3,5,20); s.start();"));
	ASSERT_TRUE(wait_sequence(INDIGO_OK_STATE, 5));
	ASSERT_EQ_INT(before + 5, atomic_load(&orchestration_changes));
	indigo_property *property = snapshot("Astrometry Agent", "AGENT_PLATESOLVER_GOTO_SETTINGS");
	ASSERT_TRUE(property != NULL);
	ASSERT_NEAR(5, property->items[0].number.value, 0.001);
	ASSERT_NEAR(20, property->items[1].number.value, 0.001);
	indigo_release_property(property);
}

static const indigo_test_case tests[] = {
	{ "library inventory and properties", library_inventory },
	{ "published property schema", published_property_schema },
	{ "all public constructors", all_public_constructors },
	{ "loop and timing construction", loop_and_timing_construction },
	{ "nested loop execution", nested_loop_execution },
	{ "capture accounting and overloads", capture_accounting },
	{ "empty sequence execution", empty_sequence_execution },
	{ "evaluate wait and messages", evaluate_wait_and_messages },
	{ "string arguments and UTC break", string_arguments_and_utc_break },
	{ "failure abort policy", failure_abort_policy },
	{ "failure continue policy", failure_continue_policy },
	{ "failure recovery policy", failure_recovery_policy },
	{ "recovery without point", recovery_without_point },
	{ "pause resume abort reset", pause_resume_abort_reset },
	{ "past waits and breaks", past_waits_and_breaks },
	{ "simulator imaging", simulator_imaging },
	{ "simulator preview", simulator_preview_and_stream },
	{ "simulator stream", simulator_stream },
	{ "simulator mount dome GPS rotator", simulator_mount_dome_gps_rotator },
	{ "simulator agent features", simulator_agent_features },
	{ "simulator altitude and hour angle", simulator_altitude_and_hour_angle },
	{ "simulator guiding", simulator_guiding },
	{ "custom agent arguments", custom_agent_arguments },
	{ "configuration and driver orchestration", configuration_and_driver_orchestration },
	{ "astrometry orchestration", astrometry_orchestration }
};

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	int failed = 0, executed = 0;
	for (unsigned i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc > 1 && !strstr(tests[i].name, argv[1])) {
			continue;
		}
		executed++;
		strcpy(test_root, "/tmp/indigo_sequencer_test_XXXXXX");
		if (!mkdtemp(test_root)) {
			return 1;
		}
		pid_t child = fork();
		if (child == 0) {
			alarm(120);
			int status = indigo_test_set_private_home(test_root) && setup() ? indigo_run_tests("Sequencer.js integration", tests + i, 1) : 1;
			cleanup();
			exit(status || indigo_test_failures ? 1 : 0);
		}
		int status = -1;
		if (child < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) {
			fprintf(stderr, "FAILED %s (status %d)\n", tests[i].name, status);
			failed++;
		}
		indigo_test_remove_tree(test_root);
	}
	printf("Sequencer.js: %d/%d passed (including cleanup)\n", executed - failed, executed);
	return executed ? (failed ? 1 : 0) : 2;
}
