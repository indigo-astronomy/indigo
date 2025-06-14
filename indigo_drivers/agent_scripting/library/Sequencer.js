// MARK: Sequence class

const SCRIPTING_AGENT = 0;
const CONFIGURATION_AGENT = 1;
const IMAGER_AGENT = 2;
const MOUNT_AGENT = 3;
const GUIDER_AGENT = 4;
const ASTROMETRY_AGENT = 5;
const SERVER = 6;

function Sequence(name) {
	this.name = name == undefined ? "" : name;
	this.step = 0;
	this.recovery_point_index = 0;
	this.progress = 0;
	this.exposure = 0;
	this.sequence = [];
	this.reset_loop_content_state = true;
}

Sequence.prototype.enable_reset_loop_content_state = function() {
	this.reset_loop_content_state = true;
}

Sequence.prototype.disable_reset_loop_content_state = function() {
	this.reset_loop_content_state = false;
}

Sequence.prototype.repeat = function(count, block) {
	var loop = this.step;
	var recovery_point_index_backup = this.recovery_point_index;
	var i = 0;
	this.sequence.push({ execute: 'enter_loop()', step: loop, progress: this.progress, exposure: this.exposure });
	while (i < count) {
		if (this.reset_loop_content_state) {
			this.sequence.push({ execute: 'reset_loop_content()', step: loop, progress: this.progress, exposure: this.exposure });
		}
		this.step = loop + 1;
		this.recovery_point_index = recovery_point_index_backup;
		block();
		i++;
		this.sequence.push({ execute: 'increment_loop(' + i + ')', step: loop, progress: this.progress, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'exit_loop()', step: this.step, progress: this.progress, exposure: this.exposure });
	if (count <= 0) {
		this.step++;
	}
};

Sequence.prototype.enable_verbose = function() {
	this.sequence.push({ execute: 'set_verbose(true)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_verbose = function() {
	this.sequence.push({ execute: 'set_verbose(false)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.recovery_point = function() {
	this.sequence.push({ execute: 'recovery_point(' + (++this.recovery_point_index) + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.wait = function(seconds) {
	this.sequence.push({ execute: 'wait(' + seconds + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.wait_until = function(time) {
	if(typeof time === 'string') {
		time = '"' + time + '"';
	}
	this.sequence.push({ execute: 'wait_until(' + time + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.continue_on_failure = function() {
	this.sequence.push({ execute: 'set_failure_handling(2)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.recover_on_failure = function() {
	this.sequence.push({ execute: 'set_failure_handling(1)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.abort_on_failure = function() {
	this.sequence.push({ execute: 'set_failure_handling(0)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.evaluate = function(code) {
	this.sequence.push({ execute: 'evaluate("' + code + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.send_message = function(message) {
	this.sequence.push({ execute: 'send_message("' + message + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.load_config = function(name) {
	this.sequence.push({ execute: 'load_config("' + name + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.load_driver = function(name) {
	this.sequence.push({ execute: 'load_driver("' + name + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.unload_driver = function(name) {
	this.sequence.push({ execute: 'unload_driver("' + name + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_imager_agent = function(agent) {
	this.sequence.push({ execute: 'select_imager_agent("' + agent + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_mount_agent = function(agent) {
	this.sequence.push({ execute: 'select_mount_agent("' + agent + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_guider_agent = function(agent) {
	this.sequence.push({ execute: 'select_guider_agent("' + agent + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_imager_camera = function(camera) {
	this.sequence.push({ execute: 'select_imager_camera("' + camera + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_filter_wheel = function(wheel) {
	this.sequence.push({ execute: 'select_filter_wheel("' + wheel + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_focuser = function(focuser) {
	this.sequence.push({ execute: 'select_focuser("' + focuser + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_rotator = function(rotator) {
	this.sequence.push({ execute: 'select_rotator("' + rotator + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_mount = function(mount) {
	this.sequence.push({ execute: 'select_mount("' + mount + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_dome = function(dome) {
	this.sequence.push({ execute: 'select_dome("' + dome + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_gps = function(gps) {
	this.sequence.push({ execute: 'select_gps("' + gps + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_guider_camera = function(camera) {
	this.sequence.push({ execute: 'select_guider_camera("' + camera + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_guider = function(guider) {
	this.sequence.push({ execute: 'select_guider("' + guider + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_frame_type = function(type) {
	this.sequence.push({ execute: 'select_frame_type("' + type + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_image_format = function(format) {
	this.sequence.push({ execute: 'select_image_format("' + format + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_frame = function(left, top, width, height) {
	this.sequence.push({ execute: 'set_frame(' + left + ',' + top + ',' + width + ',' + height + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.reset_frame = function() {
	this.sequence.push({ execute: 'reset_frame()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_camera_mode = function(mode) {
	this.sequence.push({ execute: 'select_camera_mode("' + mode + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_gain = function(value) {
	this.sequence.push({ execute: 'set_gain(' + value + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_offset = function(value) {
	this.sequence.push({ execute: 'set_offset(' + value + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_gamma = function(value) {
	this.sequence.push({ execute: 'set_gamma(' + value + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_program = function(program) {
	this.sequence.push({ execute: 'select_program("' + program + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_aperture = function(aperture) {
	this.sequence.push({ execute: 'select_aperture("' + aperture + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_shutter = function(shutter) {
	this.sequence.push({ execute: 'select_shutter("' + shutter + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_iso = function(iso) {
	this.sequence.push({ execute: 'select_iso("' + iso + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.enable_cooler = function(temperature) {
	this.sequence.push({ execute: 'select_cooler("ON")', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_temperature(' + temperature + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_cooler = function() {
	this.sequence.push({ execute: 'select_cooler("OFF")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.enable_dithering = function(amount, time_limit, skip_frames) {
	this.sequence.push({ execute: 'set_imager_dithering(' + skip_frames + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_guider_dithering(' + amount + ',' + time_limit + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_imager_feature("ENABLE_DITHERING", true)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_dithering = function() {
	this.sequence.push({ execute: 'set_imager_feature("ENABLE_DITHERING", false)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.enable_meridian_flip = function(use_solver, time) {
	this.sequence.push({ execute: 'set_use_solver(' + use_solver + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_pause_after_transit(' + time + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_imager_feature("PAUSE_AFTER_TRANSIT", true)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_meridian_flip = function() {
	this.sequence.push({ execute: 'set_imager_feature("PAUSE_AFTER_TRANSIT", false)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.enable_filter_offsets = function() {
	this.sequence.push({ execute: 'set_imager_feature("APPLY_FILTER_OFFSETS", true)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_filter_offsets = function() {
	this.sequence.push({ execute: 'set_imager_feature("APPLY_FILTER_OFFSETS", false)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_fits_header = function(keyword, value) {
	this.sequence.push({ execute: 'set_fits_header("' + keyword + '", "' + value +'")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.remove_fits_header = function(keyword) {
	this.sequence.push({ execute: 'remove_fits_header("' + keyword + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.select_filter = function(filter) {
	this.sequence.push({ execute: 'select_filter("' + filter + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_directory = function(directory) {
	this.sequence.push({ execute: 'set_local_mode("' + directory + '", null, null)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_file_template = function(template) {
	this.sequence.push({ execute: 'set_local_mode(null, "' + template + '", null)', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_object_name = function(name) {
	this.sequence.push({ execute: 'set_local_mode(null, null, "' + name + '")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.capture_batch = function(p1, p2, p3) {
	// If p1 is not string, treat p1 as count and p2 as exposure
	// Else treat p1 as name_template, p2 as count, and p3 as exposure
	var name_template = null;
	var count = 0;
	var exposure = 0;
	if (typeof p1 === 'string') {
		name_template = p1;
		count = p2;
		exposure = p3;
	} else {
		count = p1;
		exposure = p2;
	}
	if (arguments.length === 3 && typeof p1 === 'string') {
		this.sequence.push({ execute: 'set_local_mode(null, "' + name_template + '", null)', step: this.step, progress: this.progress++, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'set_batch(' + count + ',' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_upload_mode("BOTH")', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'capture_batch()', step: this.step++, progress: this.progress++, exposure: this.exposure });
	this.exposure += exposure * count;
};

Sequence.prototype.capture_stream = function(p1, p2, p3) {
	// If p1 is not string, treat p1 as count and p2 as exposure
	// Else treat p1 as name_template, p2 as count, and p3 as exposure
	var name_template = null;
	var count = 0;
	var exposure = 0;
	if (typeof p1 === 'string') {
		name_template = p1;
		count = p2;
		exposure = p3;
	} else {
		count = p1;
		exposure = p2;
	}
	if (arguments.length === 3 && typeof p1 === 'string') {
		this.sequence.push({ execute: 'set_local_mode(null, "' + name_template + '", null)', step: this.step, progress: this.progress++, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'set_stream(' + count + ',' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_upload_mode("BOTH")', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'capture_stream()', step: this.step++, progress: this.progress++, exposure: this.exposure });
	this.exposure += exposure * count;
};

Sequence.prototype.set_manual_focuser_mode = function() {
	this.sequence.push({ execute: 'set_focuser_mode("MANUAL")', step: this.step++, progress: this.progress++, exposure: this.exposure });
}

Sequence.prototype.set_automatic_focuser_mode = function() {
	this.sequence.push({ execute: 'set_focuser_mode("AUTOMATIC")', step: this.step++, progress: this.progress++, exposure: this.exposure });
}

Sequence.prototype.focus = function(exposure) {
	this.sequence.push({ execute: 'save_batch()', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_batch(1, ' + exposure + ', 0)', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'focus(false)', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'restore_batch()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.focus_ignore_failure = function(exposure) {
	this.sequence.push({ execute: 'save_batch()', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_batch(1, ' + exposure + ', 0)', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'focus(true)', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'restore_batch()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.clear_focuser_selection = function() {
	this.sequence.push({ execute: 'clear_focuser_selection()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_focuser_position = function(position) {
	this.sequence.push({ execute: 'set_focuser_goto()', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_focuser_position(' + position + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.park = function() {
	this.sequence.push({ execute: 'park()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.home = function() {
	this.sequence.push({ execute: 'home()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.unpark = function() {
	this.sequence.push({ execute: 'unpark()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.enable_tracking = function() {
	this.sequence.push({ execute: 'set_tracking("ON")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.disable_tracking = function() {
	this.sequence.push({ execute: 'set_tracking("OFF")', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.slew = function(ra, dec) {
	this.sequence.push({ execute: 'set_coordinates(' + ra + ',' + dec + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'slew()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.wait_for_gps = function() {
	this.sequence.push({ execute: 'wait_for_gps()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.calibrate_guiding = function(exposure) {
	if (exposure != undefined) {
		this.sequence.push({ execute: 'set_guider_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'calibrate_guiding()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

// TO BE REMOVED IN FUTURE RELEASE - USE calibrate_guiding() INSTEAD
Sequence.prototype.calibrate_guiding_exposure = function(exposure) {
	this.sequence.push({ execute: 'set_guider_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'calibrate_guiding()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.start_guiding = function(exposure) {
	if (exposure != undefined) {
		this.sequence.push({ execute: 'set_guider_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'start_guiding()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

// TO BE REMOVED IN FUTURE RELEASE - USE start_guiding() INSTEAD
Sequence.prototype.start_guiding_exposure = function(exposure) {
	this.sequence.push({ execute: 'set_guider_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'start_guiding()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.stop_guiding = function() {
	this.sequence.push({ execute: 'stop_guiding()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.clear_guider_selection = function() {
	this.sequence.push({ execute: 'clear_guider_selection()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.sync_center = function(exposure) {
	this.sequence.push({ execute: 'set_solver_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'sync_center()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.precise_goto = function(exposure, ra, dec) {
	this.sequence.push({ execute: 'set_solver_exposure(' + exposure + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_solver_target(' + ra + ', ' + dec + ')', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'precise_goto()', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.set_rotator_angle = function(value) {
	this.sequence.push({ execute: 'set_rotator_goto()', step: this.step, progress: this.progress++, exposure: this.exposure });
	this.sequence.push({ execute: 'set_rotator_angle(' + value + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
};

Sequence.prototype.start = function(imager_agent, mount_agent, guider_agent) {
	indigo_sequencer.devices[IMAGER_AGENT] = imager_agent == undefined ? "Imager Agent" : imager_agent;
	indigo_sequencer.devices[MOUNT_AGENT] = mount_agent == undefined ? "Mount Agent" : mount_agent;
	indigo_sequencer.devices[GUIDER_AGENT] = guider_agent == undefined ? "Guider Agent" : guider_agent;
	indigo_sequencer.start(this.sequence, this.name, this.progress, this.exposure);
};

// MARK: Flipper object

var indigo_flipper = {
	devices: [ "Scripting Agent", "Configuration Agent", "Imager Agent", "Mount Agent", "Guider Agent", "Astrometry Agent", "Server" ],
	waiting_for_transit: false,
	waiting_for_slew: false,
	waiting_for_sync_and_center: false,
	waiting_for_guiding: false,
	resume_guiding: false,
	use_solver: false,

	on_update: function(property) {
		if (this.waiting_for_transit)
			indigo_log("waiting_for_transit " + property.name + " → " + property.state);
		else if (this.waiting_for_slew)
			indigo_log("waiting_for_slew " + property.name + " → " + property.state);
		else if (this.waiting_for_sync_and_center)
			indigo_log("waiting_for_sync_and_center " + property.name + " → " + property.state);
		else if (this.waiting_for_guiding)
			indigo_log("waiting_for_guiding " + property.name + " → " + property.state);
		if (this.waiting_for_transit && property.device == this.devices[MOUNT_AGENT] && property.name == "AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY") {
			if (property.items.TIME_TO_TRANSIT <= 0) {
				indigo_send_message("Meridian flip started");
				this.waiting_for_transit = false;
				this.waiting_for_slew = true;
				indigo_change_switch_property(this.devices[MOUNT_AGENT], "AGENT_START_PROCESS", { SLEW: true});
			}
		} else if (this.waiting_for_slew && property.device == this.devices[MOUNT_AGENT] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				delete indigo_event_handlers.indigo_flipper;
				indigo_send_message("Meridian flip failed (due to slew failure)");
				indigo_sequencer.abort();
			} else if (property.state == "Ok") {
				this.waiting_for_slew = false;
				if (this.use_solver) {
					this.waiting_for_sync_and_center = true;
					indigo_change_switch_property(this.devices[ASTROMETRY_AGENT], "AGENT_START_PROCESS", { CENTER: true});
				} else if (this.resume_guiding) {
					this.waiting_for_guiding = true;
					indigo_change_switch_property(this.devices[GUIDER_AGENT], "AGENT_START_PROCESS", { GUIDING: true});
				} else {
					delete indigo_event_handlers.indigo_flipper;
					indigo_send_message("Meridian flip finished");
					indigo_change_switch_property(this.devices[IMAGER_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
				}
			}
		} else if (this.waiting_for_sync_and_center && property.device == this.devices[ASTROMETRY_AGENT] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				indigo_send_message("Meridian flip failed (due to sync & center failure)");
				delete indigo_event_handlers.indigo_flipper;
				indigo_sequencer.abort();
			} else if (property.state == "Ok") {
				this.waiting_for_sync_and_center = false;
				if (this.resume_guiding) {
					this.waiting_for_guiding = true;
					indigo_change_switch_property(this.devices[GUIDER_AGENT], "AGENT_START_PROCESS", { GUIDING: true});
				} else {
					delete indigo_event_handlers.indigo_flipper;
					indigo_send_message("Meridian flip finished");
					indigo_change_switch_property(this.devices[IMAGER_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
				}
			}
		} else if (this.waiting_for_guiding && property.device == this.devices[GUIDER_AGENT] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				indigo_send_message("Meridian flip failed (due to guiding failure)");
				delete indigo_event_handlers.indigo_flipper;
				indigo_sequencer.abort();
			} else if (property.state == "Busy") {
				delete indigo_event_handlers.indigo_flipper;
				indigo_send_message("Meridian flip finished");
				indigo_change_switch_property(this.devices[IMAGER_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
			}
		}
	},
	
	start: function(use_solver) {
		var guider_agent = indigo_devices[this.devices[GUIDER_AGENT]];
		if (guider_agent != null) {
			var guider_process = guider_agent.AGENT_START_PROCESS;
			if (guider_process.items.CALIBRATION_AND_GUIDING || guider_process.items.GUIDING) {
				this.resume_guiding = true;
			}
		}
		indigo_flipper.use_solver = use_solver;
		indigo_event_handlers.indigo_flipper = indigo_flipper;
		this.waiting_for_transit = true;
		this.waiting_for_slew = false;
		this.waiting_for_sync_and_center = false;
		this.waiting_for_guiding = false;
		indigo_send_message("Meridian flip waits for transit");
		indigo_log("resume_guiding = " + this.resume_guiding);
		indigo_log("use_solver = " + this.use_solver);
	}
};

// MARK: Sequencer object

var indigo_sequencer = {
	devices: [ "Scripting Agent", "Configuration Agent", "Imager Agent", "Mount Agent", "Guider Agent", "Astrometry Agent", "Server" ],
	name: "",
	sequence_state: "Ok",
	abort_state: "Ok",
	pause_state: "Ok",
	step: -1,
	progress: 0,
	progress_total: 0,
	exposure: 0,
	exposure_total: 0,
	index: -1,
	loop_level: -1,
	loop_step: [ ],
	loop_count: [ ],
	sequence: null,
	step_states: { },
	step_states_defs: { },
	wait_for_device: null,
	wait_for_property: null,
	wait_for_property_state: "Ok",
	wait_for_timer: null,
	ignore_failure: false,
	allow_busy_state: false,
	allow_same_value: false,
	allow_missing_property: false,
	skip_to_recovery_point: false,
	failure_handling: 0,
	use_solver: false,
	paused: false,
	capturing_batch: false,
	batch_exposure: 0,
	verbose: true,
	
	update_step_state: function(step, state) {
		this.step_states["" + step] = state;
		indigo_update_light_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STEP_STATE", this.step_states, "Ok");
	},
	
	on_update: function(property) {
		if (this.sequence != null) {
			if (property.device == this.devices[IMAGER_AGENT] && property.name == "AGENT_PAUSE_PROCESS" && property.state == "Busy" && property.items.PAUSE_AFTER_TRANSIT) {
				indigo_flipper.devices = this.devices;
				indigo_flipper.start(this.use_solver);
			} else if (property.device == this.wait_for_device && property.name == this.wait_for_property) {
				indigo_log("wait_for '" + property.device + " → " + property.name + "' → " + property.state);
				if (property.state == "Alert") {
					this.wait_for_device = null;
					this.wait_for_property = null;
					this.wait_for_property_state = "Ok";
					this.failure(property.name + " reports alert");
				} else if (property.state == this.wait_for_property_state) {
					var delay = 0;
					// NOTE: this avoids a race condition with reseting the star selection process in imager and guider agents !!!
					if (property.device == this.devices[MOUNT_AGENT] && property.state == "Ok" && (property.name == "AGENT_START_PROCESS" || property.name == "MOUNT_PARK" || property.name == "MOUNT_HOME")) {
						delay = 0.5;
					}
					this.wait_for_device = null;
					this.wait_for_property = null;
					this.wait_for_property_state = "Ok";
					this.ignore_failure = false;
					indigo_set_timer(indigo_sequencer_next_ok_handler, delay);
				}
			} else if (this.capturing_batch && property.device == this.wait_for_device) {
				if (property.name == "AGENT_IMAGER_STATS") {
					var remaining_exposure = property.items.EXPOSURE;
					var frame = property.items.FRAME;
					this.exposure = this.sequence[this.index].exposure + frame * this.batch_exposure - remaining_exposure;
					indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Busy");
				}
			}
		}
	},
	
	on_enumerate_properties: function(property) {
		if (property.device == null || property.device == this.devices[SCRIPTING_AGENT]) {
			if (property.name == null || property.name == "SEQUENCE_NAME") {
				indigo_define_text_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_NAME", "Sequencer", "Sequence name", { NAME: this.name }, { NAME: { label: "Name" }}, "Ok", "RO");
			}
			if (property.name == null || property.name == "SEQUENCE_STATE") {
				indigo_define_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", "Sequencer", "State", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, { STEP: { label: "Executing step", format: "%g", min: -1, max: 1000000, step: 1 }, PROGRESS: { label: "Progress", format: "%g", min: 0, max: 1000000, step: 1 }, PROGRESS_TOTAL: { label: "Progress total", format: "%g", min: 0, max: 1000000, step: 1 }, EXPOSURE: { label: "Exposure time elapsed", format: "%g", min: 0, max: 1000000, step: 1 }, EXPOSURE_TOTAL: { label: "Exposure time total", format: "%g", min: 0, max: 1000000, step: 1 }}, this.sequence_state, "RO");
			}
			if (property.name == null || property.name == "SEQUENCE_STEP_STATE") {
				indigo_define_light_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STEP_STATE", "Sequencer", "Step state", this.step_states, this.step_states_defs, "Ok");
			}
			if (property.name == null || property.name == "AGENT_ABORT_PROCESS") {
				indigo_define_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", "Sequencer", "Abort sequence", { ABORT: false }, { ABORT: { label: "Abort" }}, this.abort_state, "RW", "OneOfMany");
			}
			if (property.name == null || property.name == "AGENT_PAUSE_PROCESS") {
				indigo_define_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", "Sequencer", "Pause sequence", { PAUSE_WAIT: this.paused }, { PAUSE_WAIT: { label: "Pause (before next operation)" }}, this.pause_state, "RW", "OneOfMany");
			}
			if (property.name == null || property.name == "SEQUENCE_RESET") {
				indigo_define_switch_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_RESET", "Sequencer", "Reset sequence", { RESET: false }, { RESET: { label: "Reset" }}, "Ok", "RW", "OneOfMany");
			}
			for (var i = 0; i < this.loop_level; i++) {
				if (property.name == null || property.name == "LOOP_" + i) {
					indigo_define_number_property(this.devices[SCRIPTING_AGENT], "LOOP_" + i, "Sequencer", "Loop " + this.loop_level, { STEP: this.loop_step[i], COUNT: this.loop_count[i] }, { STEP: { label: "Loop at", format: "%g", min: 0, max: 10000, step: 1 }, COUNT: { label: "Itreations elapsed", format: "%g", min: 0, max: 10000, step: 1 }}, "Ok", "RO");
				}
			}
		}
	},
	
	on_change_property: function(property) {
		if (property.device == this.devices[SCRIPTING_AGENT]) {
			if (property.name == "AGENT_ABORT_PROCESS") {
				if (this.sequence != null) {
					if (property.items.ABORT) {
						indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Busy");
						indigo_set_timer(indigo_sequencer_abort_handler, 0);
					}
				} else {
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Alert");
					this.abort_state = "Alert";
				}
			} else if (property.name == "AGENT_PAUSE_PROCESS") {
				if (this.sequence != null) {
					if (property.items.PAUSE_WAIT) {
						if (!this.paused) {
							if (this.capturing_batch) {
								indigo_change_switch_property(this.devices[IMAGER_AGENT], "AGENT_PAUSE_PROCESS", { "PAUSE_WAIT": true });
							}
							this.paused = true;
							indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: true }, this.pause_state = "Busy");
							indigo_send_message("Sequence paused");
						}
					} else {
						if (this.paused) {
							if (this.capturing_batch) {
								indigo_change_switch_property(this.devices[IMAGER_AGENT], "AGENT_PAUSE_PROCESS", { "PAUSE_WAIT": false });
							}
							this.paused = false;
							indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
							indigo_send_message("Sequence resumed");
						}
					}
				} else {
					this.paused = false;
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Alert");
				}
			} else if (property.name == "SEQUENCE_RESET") {
				if (property.items.RESET) {
					this.step = -1;
					indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step }, this.sequence_state = "Ok");
					this.paused = false;
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
					while (this.loop_level >= 0) {
						indigo_delete_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level--);
					}
					this.step_states = {};
					this.step_states_defs = {};
					indigo_redefine_light_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STEP_STATE", "Sequencer", "Step state", this.step_states, this.step_states_defs, "Ok");
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_RESET", { RESET: false }, "Ok");
				}
			}
		}
	},
	
	abort: function() {
		this.wait_for_device = null;
		this.wait_for_property = null;
		this.wait_for_property_state = "Ok";
		this.sequence = null;
		if (this.paused) {
			this.paused = false;
			indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
		}
		for (var device in this.devices) {
			if (device == 0) {
				continue;
			}
			if (this.devices[device].startsWith("Guider Agent")) {
				var property = indigo_devices[this.devices[device]].AGENT_START_PROCESS;
				if (property != null && property.items.GUIDING) {
					continue;
				}
				indigo_change_switch_property(this.devices[device], "AGENT_ABORT_PROCESS", { "ABORT": true });
			} else if (this.devices[device].startsWith("Astrometry Agent")) {
				indigo_change_switch_property(this.devices[device], "AGENT_PLATESOLVER_ABORT", { "ABORT": true });
			} else {
				indigo_change_switch_property(this.devices[device], "AGENT_ABORT_PROCESS", { "ABORT": true });
			}
		}
		if (this.wait_for_timer != null) {
			indigo_cancel_timer(this.wait_for_timer);
			this.wait_for_timer = null;
		}
		indigo_sequencer.update_step_state(indigo_sequencer.step, "Alert");
		indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Alert", "Sequence aborted");
		indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
	},
	
	start: function(sequence, name, progress_total, exposure_total) {
		if (this.sequence != null) {
			indigo_send_message("Other sequence is executed");
		} else {
			this.index = -1;
			this.step = 0;
			this.progress = 0;
			this.progress_total = progress_total;
			this.exposure = 0;
			this.exposure_total = exposure_total;
			this.paused = false;
			this.capturing_batch = false;
			this.failure_handling = 0;
			this.skip_to_recovery_point = false;
			this.ignore_failure = false;
			indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
			indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
			indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Busy");
			this.name = name;
			indigo_update_text_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_NAME", { NAME: this.name }, "Ok");
			while (this.loop_level >= 0) {
				indigo_delete_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level--);
			}
			this.sequence = sequence;
			var last_step = -1;
			this.step_states = {};
			this.step_states_defs = {};
			for (var i = 0; i < sequence.length; i++) {
				var entry = sequence[i];
				if (entry.execute.startsWith("increment_loop(") || entry.execute.startsWith("exit_loop(")) {
					continue;
				}
				if (entry.step > last_step) {
					last_step = entry.step;
					var name = "" + last_step;
					this.step_states[name] = "Idle";
					this.step_states_defs[name] = { label: name }
				}
			}
			indigo_redefine_light_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STEP_STATE", "Sequencer", "Step state", this.step_states, this.step_states_defs, "Ok");
			indigo_send_message("Sequence started");
			indigo_set_timer(indigo_sequencer_next_handler, 0.1);
		}
	},

	execute_next: function() {
		if (this.paused) {
			indigo_set_timer(indigo_sequencer_next_handler, 0.01);
		} else if (this.sequence != null) {
			previous = this.sequence[this.index];
			nesting = 0;
			while (true) {
				current = this.sequence[++this.index];
				if (current == null || !this.skip_to_recovery_point || current.execute.startsWith("recovery_point(")) {
					break;
				}
				if (current.execute == "enter_loop()") {
					nesting++;
				}
				if (current.execute == "exit_loop()") {
					nesting--;
					if (nesting <= 0) {
						indigo_delete_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level--);
						this.loop_count.pop();
					}
				}
				if (previous.step < current.step) {
					this.update_step_state(current.step, "Idle");
				}
			}
			if (current != null) {
				this.step = current.step;
				this.progress = current.progress;
				this.exposure = current.exposure;
				this.capturing_batch = false;
				indigo_log(current.execute);
				indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Busy");
				this.update_step_state(this.step, "Busy");
				eval("indigo_sequencer." + current.execute);
			} else {
				this.progress = this.progress_total;
				this.exposure = this.exposure_total;
				this.sequence = null;
				if (this.paused) {
					this.paused = false;
					indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
				}
				if (this.skip_to_recovery_point) {
					indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Alert");
					indigo_send_message("Sequence failed, no recovery point found");
				} else {
					indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Ok");
					indigo_send_message("Sequence finished");
				}
			}
		}
	},
	
	enter_loop: function() {
		this.loop_level++;
		this.loop_count.push(0);
		this.loop_step.push(this.step);
		indigo_define_number_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level, "Sequencer", "Loop " + this.loop_step[this.loop_level], { STEP: this.step, COUNT: this.loop_count[this.loop_level] }, { STEP: { label: "Loop at", format: "%g", min: 0, max: 10000, step: 1 }, COUNT: { label: "Itreations elapsed", format: "%g", min: 0, max: 10000, step: 1 }}, "Ok", "RO");
		indigo_set_timer(indigo_sequencer_next_handler, 0.1);
	},
	
	reset_loop_content: function() {
		var nesting = 0;
		for (var i = this.index + 1; i < this.sequence.length; i++) {
			var next = this.sequence[i];
			if (nesting == 0 && next.execute.startsWith("increment_loop(")) {
				break;
			} else if (next.execute == "enter_loop()") {
				nesting++;
			} else if (next.execute == "exit_loop()") {
				nesting--;
			}
			this.step_states[next.step] = "Idle";
		}
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
	
	increment_loop: function(i) {
		indigo_update_number_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level, { COUNT: this.loop_count[this.loop_level] = i }, "Ok");
		indigo_set_timer(indigo_sequencer_next_handler, 0.1);
	},
		
	exit_loop: function() {
		indigo_delete_property(this.devices[SCRIPTING_AGENT], "LOOP_" + this.loop_level--);
		this.loop_count.pop();
		this.update_step_state(this.loop_step.pop(), "Ok");
		indigo_set_timer(indigo_sequencer_next_handler, 0.1);
	},

	set_verbose: function(value) {
		this.verbose = value;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
	},

	recovery_point: function(index) {
		if (this.skip_to_recovery_point) {
			this.skip_to_recovery_point = false;
			indigo_send_message("Recovered from failure at recovery point #" + index);
		}
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},
	
	set_switch: function(device, property_name, item, value, state) {
		var property = indigo_devices[device][property_name];

		var allow_busy_state = this.allow_busy_state;
		var allow_missing_property = this.allow_missing_property;
		var allow_same_value = this.allow_same_value;

		this.allow_busy_state = false;
		this.allow_missing_property = false;
		this.allow_same_value = false;

		if (property == null) {
			if (allow_missing_property) {
				indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
				return;
			}
			this.failure("Failed to set '" + property_name + "' on '" + device + "'");
			return;
		}
		if (property.state == "Busy") {
			if (!allow_busy_state) {
				this.failure("Failed to set '" + property.label + "', '" + device + "' is busy");
				return;
			}
		}
		var current_value = property.items[item];
		if (current_value == null) {
			var found = false;
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === item) {
					item = name;
					current_value = property.items[item];
					found = true;
					break;
				}
			}
			if (!found) {
				this.failure("Failed to set non-existent option '" +item + "' for '" + device + " → " + property.label + "'");
				return;
			}
		}
		if (current_value == value) {
			if (allow_same_value) {
				indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
			} else {
				this.warning("'" + device + " → " + property.label + " → " + property.item_defs[item].label + "' is already " + (value ? "selected" : "unselected"));
			}
			return;
		}
		var items = { };
		items[item] = value;
		this.wait_for_device = device;
		this.wait_for_property = property_name;
		this.wait_for_property_state = state != null ? state : "Ok";
		indigo_change_switch_property(device, property_name, items);
	},

	select_switch: function(device, property, item, state) {
		this.set_switch(device, property, item, true, state);
	},
	
	deselect_switch: function(device, property, item) {
		this.set_switch(device, property, item, false, state);
	},
	
	change_texts: function(device, property_name, items) {
		var property = indigo_devices[device][property_name];

		var allow_busy_state = this.allow_busy_state;
		var allow_missing_property = this.allow_missing_property;

		this.allow_busy_state = false;
		this.allow_missing_property = false;
		this.allow_same_value = false;

		if (property == null) {
			if (allow_missing_property) {
				indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
				return;
			}
			this.failure("There is no " + property_name + " on " + device);
			return;
		}
		if (property.state == "Busy") {
			if (!allow_busy_state) {
				this.failure("Failed to set '" + property.label + "', '" + device + "' is busy");
				return;
			}
		}
		var empty = true;
		for (var name in items) {
			if (items[name] == undefined) {
				delete items[name];
			} else {
				empty = false;
			}
		}
		if (empty) {
			indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
			return;
		}
		this.wait_for_device = device;
		this.wait_for_property = property_name;
		indigo_change_text_property(device, property_name, items);
	},

	change_numbers: function(device, property_name, items) {
		var property = indigo_devices[device][property_name];

		var allow_busy_state = this.allow_busy_state;
		var allow_missing_property = this.allow_missing_property;

		this.allow_busy_state = false;
		this.allow_missing_property = false;
		this.allow_same_value = false;

		if (property == null) {
			if (allow_missing_property) {
				indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
				return;
			}
			this.failure("There is no " + property_name + " on " + device);
			return;
		}
		if (property.state == "Busy") {
			if (!allow_busy_state) {
				this.failure("Failed to set '" + property.label + "', '" + device + "' is busy");
				return;
			}
		}
		var empty = true;
		for (var name in items) {
			if (items[name] == undefined) {
				delete items[name];
			} else {
				empty = false;
			}
		}
		if (empty) {
			indigo_set_timer(indigo_sequencer_next_ok_handler, 0.1);
			return;
		}
		this.wait_for_device = device;
		this.wait_for_property = property_name;
		indigo_change_number_property(device, property_name, items);
	},
	
	warning: function(message) {
		if (this.verbose) {
			indigo_send_message(message);
		}
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},
	
	failure: function(message) {
		indigo_send_message(message);
		if (this.ignore_failure || this.failure_handling == 2) {
			this.ignore_failure = false;
			this.update_step_state(this.step, "Alert");
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		} else if (this.failure_handling == 1) {
			this.skip_to_recovery_point = true;
			this.update_step_state(this.step, "Alert");
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		} else {
			this.wait_for_device = null;
			this.wait_for_property = null;
			this.wait_for_property_state = "Ok";
			this.sequence = null;
			this.update_step_state(this.step, "Alert");
			if (this.paused) {
				this.paused = false;
				indigo_update_switch_property(this.devices[SCRIPTING_AGENT], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
			}
			indigo_update_number_property(this.devices[SCRIPTING_AGENT], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.sequence_state = "Alert", "Sequence failed");
		}
	},
	
	wait: function(seconds) {
		var result = indigo_set_timer(indigo_sequencer_next_ok_handler, seconds);
		if (result >= 0) {
			this.wait_for_timer = result;
			indigo_send_message("Suspended for " + seconds + " second(s)");
		} else {
			this.failure("Can't schedule timer in " + seconds + " second(s)");
		}
	},

	wait_until: function(time) {
		var result;
		if (typeof time === "number") {
			result = indigo_set_timer_at(indigo_sequencer_next_ok_handler, time);
		} else {
			result = indigo_set_timer_at_utc(indigo_sequencer_next_ok_handler, time);
		}
		if (result >= 0) {
			this.wait_for_timer = result;
			indigo_send_message("Suspended until " + time + " UTC");
		} else {
			this.failure("Can't schedule timer at " + time + " UTC");
		}
	},

	set_failure_handling: function(failure_handling) {
		this.failure_handling = failure_handling;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},

	evaluate: function(code) {
		eval(code);
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},

	send_message: function(message) {
		indigo_send_message(message);
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},

	load_config: function(name) {
		this.select_switch(this.devices[CONFIGURATION_AGENT], "AGENT_CONFIG_LOAD", name);
	},

	load_driver: function(name) {
		this.select_switch("Server", "DRIVERS", name);
	},
	
	unload_driver: function(name) {
		this.deselect_switch("Server", "DRIVERS", name);
	},
	
	select_imager_agent: function(agent) {
		this.devices[IMAGER_AGENT] = agent;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},
		
	select_mount_agent: function(agent) {
		this.devices[MOUNT_AGENT] = agent;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},
		
	select_guider_agent: function(agent) {
		this.devices[GUIDER_AGENT] = agent;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},

	select_device: function(agent, filter_property, device) {
		if (agent == undefined)
			agent = "NONE";
		this.select_switch(agent, filter_property, device);
	},

	select_imager_camera: function(camera) {
		this.select_device(this.devices[IMAGER_AGENT], "FILTER_CCD_LIST", camera);
	},

	select_filter_wheel: function(wheel) {
		this.select_device(this.devices[IMAGER_AGENT], "FILTER_WHEEL_LIST", wheel);
	},

	select_focuser: function(focuser) {
		this.select_device(this.devices[IMAGER_AGENT], "FILTER_FOCUSER_LIST", focuser);
	},

	select_rotator: function(rotator) {
		this.select_device(this.devices[MOUNT_AGENT], "FILTER_ROTATOR_LIST", rotator);
	},

	select_mount: function(mount) {
		this.select_device(this.devices[MOUNT_AGENT], "FILTER_MOUNT_LIST", mount);
	},

	select_dome: function(dome) {
		this.select_device(this.devices[MOUNT_AGENT], "FILTER_DOME_LIST", dome);
	},

	select_gps: function(gps) {
		this.select_device(this.devices[MOUNT_AGENT], "FILTER_GPS_LIST", gps);
	},

	select_guider_camera: function(camera) {
		this.select_device(this.devices[GUIDER_AGENT], "FILTER_CCD_LIST", camera);
	},

	select_guider: function(guider) {
		this.select_device(this.devices[GUIDER_AGENT], "FILTER_GUIDER_LIST", guider);
	},

	select_frame_type: function(type) {
		this.select_switch(this.devices[IMAGER_AGENT], "CCD_FRAME_TYPE", type);
	},

	select_image_format: function(format) {
		this.select_switch(this.devices[IMAGER_AGENT], "CCD_IMAGE_FORMAT", format);
	},

	set_frame: function(left, top, width, height) {
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_FRAME", { LEFT: left, TOP: top, WIDTH: width, HEIGHT: height });
	},

	reset_frame: function() {
		// values are adjusted to max values automatically
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_FRAME", { LEFT: 0, TOP: 0, WIDTH: 100000, HEIGHT: 100000 });
	},

	select_camera_mode: function(mode) {
		this.select_switch(this.devices[IMAGER_AGENT], "CCD_MODE", mode);
	},

	set_gain: function(value) {
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_GAIN", { GAIN: value });
	},

	set_offset: function(value) {
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_OFFSET", { OFFSET: value });
	},

	set_gamma: function(value) {
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_GAMMA", { GAMMA: value });
	},

	select_program: function(program) {
		this.select_switch(this.devices[IMAGER_AGENT], "DSLR_PROGRAM", program);
	},

	select_aperture: function(aperture) {
		this.select_switch(this.devices[IMAGER_AGENT], "DSLR_APERTURE", aperture);
	},

	select_shutter: function(shutter) {
		this.select_switch(this.devices[IMAGER_AGENT], "DSLR_SHUTTER", shutter);
	},

	select_iso: function(iso) {
		this.select_switch(this.devices[IMAGER_AGENT], "DSLR_ISO", iso);
	},

	select_cooler: function(name) {
		this.select_switch(this.devices[IMAGER_AGENT], "CCD_COOLER", name);
	},
	
	set_temperature: function(temperature) {
		this.allow_busy_state = true;
		this.change_numbers(this.devices[IMAGER_AGENT], "CCD_TEMPERATURE", { TEMPERATURE: temperature });
	},
	
	set_use_solver: function(use_solver) {
		this.use_solver = use_solver;
		indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
	},
	
	set_pause_after_transit: function(time) {
		this.change_numbers(this.devices[IMAGER_AGENT], "AGENT_IMAGER_BATCH", { PAUSE_AFTER_TRANSIT: time });
	},
	
	set_imager_dithering: function(skip_frames) {
		this.change_numbers(this.devices[IMAGER_AGENT], "AGENT_IMAGER_BATCH", { FRAMES_TO_SKIP_BEFORE_DITHER: skip_frames });
	},

	set_fits_header: function(keyword, value) {
		this.change_texts(this.devices[IMAGER_AGENT], "CCD_SET_FITS_HEADER", { "KEYWORD": keyword, "VALUE": value });
	},
	
	remove_fits_header: function(keyword, value) {
		this.change_texts(this.devices[IMAGER_AGENT], "CCD_REMOVE_FITS_HEADER", { "KEYWORD": keyword });
	},
	
	set_guider_dithering: function(amount, time_limit) {
		this.change_numbers(this.devices[GUIDER_AGENT], "AGENT_GUIDER_SETTINGS", { DITHERING_MAX_AMOUNT: amount, DITHERING_SETTLE_TIME_LIMIT: time_limit});
	},

	set_imager_feature: function(name, value) {
		this.set_switch(this.devices[IMAGER_AGENT], "AGENT_PROCESS_FEATURES", name, value);
	},
	
	select_filter: function(filter) {
		this.select_switch(this.devices[IMAGER_AGENT], "AGENT_WHEEL_FILTER", filter);
	},
	
	set_local_mode: function(directory, prefix, object) {
		this.change_texts(this.devices[IMAGER_AGENT], "CCD_LOCAL_MODE", { DIR: directory, PREFIX: prefix, OBJECT: object});
	},

	save_batch: function() {
		var agent = this.devices[IMAGER_AGENT];
		var property = indigo_devices[agent].AGENT_IMAGER_BATCH;
		if (property != null) {
			this.saved_batch = property.items;
			indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
		} else {
			this.failure("Can't save batch");
		}
	},

	restore_batch: function() {
		this.change_numbers(this.devices[IMAGER_AGENT], "AGENT_IMAGER_BATCH", this.saved_batch);
	},

	set_upload_mode: function(mode) {
		this.allow_same_value = true;
		this.select_switch(this.devices[IMAGER_AGENT], "CCD_UPLOAD_MODE", mode);
	},

	set_batch: function(count, exposure) {
		this.batch_exposure = exposure;
		this.change_numbers(this.devices[IMAGER_AGENT], "AGENT_IMAGER_BATCH", { COUNT: count, EXPOSURE: exposure});
	},

	capture_batch: function() {
		this.capturing_batch = true;
		this.select_switch(this.devices[IMAGER_AGENT], "AGENT_START_PROCESS", "EXPOSURE");
	},

	capture_stream: function() {
		this.select_switch(this.devices[IMAGER_AGENT], "AGENT_START_PROCESS", "STREAMING");
	},

	set_focuser_mode: function(mode) {
		this.select_switch(this.devices[IMAGER_AGENT], "FOCUSER_MODE", mode);
	},
	
	focus: function(ignore_failure) {
		this.ignore_failure = ignore_failure;
		this.select_switch(this.devices[IMAGER_AGENT], "AGENT_START_PROCESS", "FOCUSING");
	},

	clear_focuser_selection: function() {
		this.select_switch(this.devices[IMAGER_AGENT], "AGENT_START_PROCESS", "CLEAR_SELECTION");
	},
	
	unpark: function() {
		this.select_switch(this.devices[MOUNT_AGENT], "MOUNT_PARK", "UNPARKED");
	},
	
	set_coordinates: function(ra, dec) {
		this.change_numbers(this.devices[MOUNT_AGENT], "AGENT_MOUNT_EQUATORIAL_COORDINATES", { RA: ra, DEC: dec});
	},
	
	slew: function(ra, dec) {
		this.select_switch(this.devices[MOUNT_AGENT], "AGENT_START_PROCESS", "SLEW");
	},
	
	park: function() {
		this.select_switch(this.devices[MOUNT_AGENT], "MOUNT_PARK", "PARKED");
	},
	
	home: function() {
		this.select_switch(this.devices[MOUNT_AGENT], "MOUNT_HOME", "HOME");
	},

	set_tracking: function(on_off) {
		this.select_switch(this.devices[MOUNT_AGENT], "MOUNT_TRACKING", on_off);
	},

	wait_for_gps: function() {
		var agent = this.devices[MOUNT_AGENT];
		var property = indigo_devices[agent].GPS_GEOGRAPHIC_COORDINATES;
		if (property != null) {
			if (property.state == "Busy") {
				indigo_send_message("Waiting for GPS");
				this.wait_for_device = agent;
				this.wait_for_property =  "GPS_GEOGRAPHIC_COORDINATES";
			} else {
				indigo_set_timer(indigo_sequencer_next_ok_handler, 0);
			}
		} else {
			this.failure("Can't wait for GPS");
		}
	},
	
	set_guider_exposure: function(exposure) {
		this.change_numbers(this.devices[GUIDER_AGENT], "AGENT_GUIDER_SETTINGS", { EXPOSURE: exposure });
	},

	calibrate_guiding: function() {
		this.select_switch(this.devices[GUIDER_AGENT], "AGENT_START_PROCESS", "CALIBRATION");
	},

	start_guiding: function() {
		this.allow_busy_state = true;
		this.select_switch(this.devices[GUIDER_AGENT], "AGENT_START_PROCESS", "GUIDING", "Busy");
	},

	stop_guiding: function() {
		this.select_switch(this.devices[GUIDER_AGENT], "AGENT_ABORT_PROCESS", "ABORT");
	},

	clear_guider_selection: function() {
		this.select_switch(this.devices[GUIDER_AGENT], "AGENT_START_PROCESS", "CLEAR_SELECTION");
	},
	
	set_solver_exposure: function(exposure) {
		this.change_numbers(this.devices[ASTROMETRY_AGENT], "AGENT_PLATESOLVER_EXPOSURE", { EXPOSURE: exposure });
	},

	set_solver_target: function(ra, dec) {
		this.change_numbers(this.devices[ASTROMETRY_AGENT], "AGENT_PLATESOLVER_GOTO_SETTINGS", { RA: ra, DEC: dec });
	},

	precise_goto: function() {
		this.select_switch(this.devices[ASTROMETRY_AGENT], "AGENT_START_PROCESS", "PRECISE_GOTO");
	},

	sync_center: function() {
		this.select_switch(this.devices[ASTROMETRY_AGENT], "AGENT_START_PROCESS", "CENTER");
	},

	set_rotator_goto: function() {
		this.allow_same_value = true;
		this.allow_missing_property = true;
		this.select_switch(this.devices[MOUNT_AGENT], "ROTATOR_ON_POSITION_SET", "GOTO");
	},

	set_rotator_angle: function(angle) {
		this.change_numbers(this.devices[MOUNT_AGENT], "ROTATOR_POSITION", { POSITION: angle });
	},

	set_focuser_goto: function() {
		this.allow_same_value = true;
		this.allow_missing_property = true;
		this.select_switch(this.devices[IMAGER_AGENT], "FOCUSER_ON_POSITION_SET", "GOTO");
	},

	set_focuser_position: function(position) {
		this.change_numbers( this.devices[IMAGER_AGENT], "FOCUSER_POSITION", { POSITION: position });
	}
};

// MARK: Timers callback functions

function indigo_sequencer_next_handler() {
	indigo_sequencer.wait_for_timer = null;
	indigo_sequencer.execute_next();
}

function indigo_sequencer_next_ok_handler() {
	indigo_sequencer.wait_for_timer = null;
	indigo_sequencer.update_step_state(indigo_sequencer.step, "Ok");
	indigo_sequencer.execute_next();
}

function indigo_sequencer_abort_handler() {
	indigo_sequencer.abort();
}

// MARK: Main code

if (indigo_event_handlers.indigo_sequencer == null) {
	indigo_send_message("Sequencer installed");
}
indigo_delete_property("Scripting Agent", "LOOP_0");
indigo_delete_property("Scripting Agent", "LOOP_1");
indigo_delete_property("Scripting Agent", "LOOP_2");
indigo_delete_property("Scripting Agent", "LOOP_3");
indigo_delete_property("Scripting Agent", "LOOP_4");
indigo_delete_property("Scripting Agent", "LOOP_5");
indigo_delete_property("Scripting Agent", "LOOP_6");
indigo_delete_property("Scripting Agent", "LOOP_7");
indigo_delete_property("Scripting Agent", "LOOP_8");
indigo_delete_property("Scripting Agent", "LOOP_9");
indigo_event_handlers.indigo_sequencer = indigo_sequencer;
indigo_enumerate_properties();
