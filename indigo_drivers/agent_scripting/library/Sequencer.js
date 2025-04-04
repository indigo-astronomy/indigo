function Sequence(name) {
	this.name = name == undefined ? "" : name;
	this.step = 0;
	this.progress = 0;
	this.exposure = 0;
	this.sequence = [];
}

Sequence.prototype.repeat = function(count, block) {
	var loop = this.step;
	var i = 0;
	this.sequence.push({ execute: 'enter_loop()', step: loop, progress: this.progress, exposure: this.exposure });
	while (i < count) {
		this.step = loop + 1;
		block();
		i++;
		this.sequence.push({ execute: 'increment_loop(' + i + ')', step: loop, progress: this.progress, exposure: this.exposure });
	}
	this.sequence.push({ execute: 'exit_loop()', step: this.step, progress: this.progress, exposure: this.exposure });
	if (count <= 0) {
		this.step++;
	}
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

Sequence.prototype.slew = function(ra, dec) {
	this.sequence.push({ execute: 'slew(' + ra + ',' + dec + ')', step: this.step++, progress: this.progress++, exposure: this.exposure });
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
	indigo_sequencer.devices[2] = imager_agent == undefined ? "Imager Agent" : imager_agent;
	indigo_sequencer.devices[3] = mount_agent == undefined ? "Mount Agent" : mount_agent;
	indigo_sequencer.devices[4] = guider_agent == undefined ? "Guider Agent" : guider_agent;
	indigo_sequencer.start(this.sequence, this.name, this.progress, this.exposure);
};

var indigo_flipper = {
	devices: [
		"Scripting Agent",
		"Configuration Agent",
		"Imager Agent",
		"Mount Agent",
		"Guider Agent",
		"Astrometry Agent",
		"Server"
	],

	waiting_for_transit: false,
	waiting_for_slew: false,
	waiting_for_sync_and_center: false,
	waiting_for_guiding: false,
	resume_guiding: false,
	use_solver: false,

	on_update: function(property) {
		if (this.waiting_for_transit)
			indigo_log("waiting_for_transit " + property.name + " -> " + property.state);
		else if (this.waiting_for_slew)
			indigo_log("waiting_for_slew " + property.name + " -> " + property.state);
		else if (this.waiting_for_sync_and_center)
			indigo_log("waiting_for_sync_and_center " + property.name + " -> " + property.state);
		else if (this.waiting_for_guiding)
			indigo_log("waiting_for_guiding " + property.name + " -> " + property.state);
		if (this.waiting_for_transit && property.device == this.devices[3] && property.name == "AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY") {
			if (property.items.TIME_TO_TRANSIT <= 0) {
				indigo_send_message("Meridian flip started");
				this.waiting_for_transit = false;
				this.waiting_for_slew = true;
				indigo_change_switch_property(this.devices[3], "AGENT_START_PROCESS", { SLEW: true});
			}
		} else if (this.waiting_for_slew && property.device == this.devices[3] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				delete indigo_event_handlers.indigo_flipper;
				indigo_send_message("Meridian flip failed (due to slew failure)");
				indigo_sequencer.abort();
			} else if (property.state == "Ok") {
				this.waiting_for_slew = false;
				if (this.use_solver) {
					this.waiting_for_sync_and_center = true;
					indigo_change_switch_property(this.devices[5], "AGENT_START_PROCESS", { CENTER: true});
				} else if (this.resume_guiding) {
					this.waiting_for_guiding = true;
					indigo_change_switch_property(this.devices[4], "AGENT_START_PROCESS", { GUIDING: true});
				} else {
					delete indigo_event_handlers.indigo_flipper;
					indigo_send_message("Meridian flip finished");
					indigo_change_switch_property(this.devices[2], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
				}
			}
		} else if (this.waiting_for_sync_and_center && property.device == this.devices[5] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				indigo_send_message("Meridian flip failed (due to sync & center failure)");
				delete indigo_event_handlers.indigo_flipper;
				indigo_sequencer.abort();
			} else if (property.state == "Ok") {
				this.waiting_for_sync_and_center = false;
				if (this.resume_guiding) {
					this.waiting_for_guiding = true;
					indigo_change_switch_property(this.devices[4], "AGENT_START_PROCESS", { GUIDING: true});
				} else {
					delete indigo_event_handlers.indigo_flipper;
					indigo_send_message("Meridian flip finished");
					indigo_change_switch_property(this.devices[2], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
				}
			}
		} else if (this.waiting_for_guiding && property.device == this.devices[4] && property.name == "AGENT_START_PROCESS") {
			if (property.state == "Alert") {
				indigo_send_message("Meridian flip failed (due to guiding failure)");
				delete indigo_event_handlers.indigo_flipper;
				indigo_sequencer.abort();
			} else if (property.state == "Busy") {
				delete indigo_event_handlers.indigo_flipper;
				indigo_send_message("Meridian flip finished");
				indigo_change_switch_property(this.devices[2], "AGENT_PAUSE_PROCESS", { PAUSE_AFTER_TRANSIT: false});
			}
		}
	},
	
	start: function(use_solver) {
		var guider_agent = indigo_devices[this.devices[4]];
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

var indigo_sequencer = {
	devices: [
		"Scripting Agent",
		"Configuration Agent",
		"Imager Agent",
		"Mount Agent",
		"Guider Agent",
		"Astrometry Agent",
		"Server"
	],

	name: "",
	state: "Ok",
	abort_state: "Ok",
	pause_state: "Ok",
	step: -1,
	progress: 0,
	progress_total: 0,
	exposure: 0,
	exposure_total: 0,
	index: -1,
	loop_level: -1,
	sequence: null,
	wait_for_device: null,
	wait_for_name: null,
	wait_for_item: null,
	wait_for_value: null,
	wait_for_value_tolerance: null,
	wait_for_timer: null,
	ignore_failure: false,
	use_solver: false,
	paused: false,
	capturing_batch: false,
	
	on_update: function(property) {
		if (this.sequence != null) {
			if (property.device == this.devices[2] && property.name == "AGENT_PAUSE_PROCESS" && property.state == "Busy" && property.items.PAUSE_AFTER_TRANSIT) {
				indigo_flipper.devices = this.devices;
				indigo_flipper.start(this.use_solver);
			} else if (property.device == this.wait_for_device && property.name == this.wait_for_name) {
				if (this.wait_for_item != null && this.wait_for_value != null) {
					if (this.wait_for_value_tolerance != null) {
						diff = Math.abs(property.items[this.wait_for_item] - this.wait_for_value);
						if (diff > this.wait_for_value_tolerance) {
							indigo_log("wait_for_item '" + this.wait_for_item + "' -> " + property.items[this.wait_for_item] + " (" + diff + " > " + this.wait_for_value_tolerance + ")");
							return;
						}
					} else if (property.items[this.wait_for_item] != this.wait_for_value) {
						indigo_log("wait_for_item '" + this.wait_for_item + "' -> " + property.items[this.wait_for_item]);
						return;
					}
				}
				indigo_log("wait_for '" + property.device + "', '" + property.name + "' -> " + property.state);
				if (property.state != "Busy") {
					this.wait_for_device = null;
					this.wait_for_name = null;
					this.wait_for_item = null;
					this.wait_for_value = null;
					this.wait_for_value_tolerance = null;
					if (property.state == "Ok" || this.ignore_failure) {
						this.ignore_failure = false;
						indigo_set_timer(indigo_sequencer_next_handler, 0);
					} else if (property.state == "Alert") {
						this.failure("Sequence failed");
					}
				}
			}
		}
	},
	
	on_enumerate_properties: function(property) {
		if (property.device == null || property.device == this.devices[0]) {
			if (property.name == null || property.name == "SEQUENCE_NAME") {
				indigo_define_text_property(this.devices[0], "SEQUENCE_NAME", "Sequencer", "Sequence name", { NAME: this.name }, { NAME: { label: "Name" }}, "Ok", "RO");
			}
			if (property.name == null || property.name == "SEQUENCE_STATE") {
				indigo_define_number_property(this.devices[0], "SEQUENCE_STATE", "Sequencer", "State", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, { STEP: { label: "Executing step", format: "%g", min: -1, max: 1000000, step: 1 }, PROGRESS: { label: "Progress", format: "%g", min: 0, max: 1000000, step: 1 }, PROGRESS_TOTAL: { label: "Progress total", format: "%g", min: 0, max: 1000000, step: 1 }, EXPOSURE: { label: "Exposure time elapsed", format: "%g", min: 0, max: 1000000, step: 1 }, EXPOSURE_TOTAL: { label: "Exposure time total", format: "%g", min: 0, max: 1000000, step: 1 }}, this.state, "RO");
			}
			if (property.name == null || property.name == "AGENT_ABORT_PROCESS") {
				indigo_define_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", "Sequencer", "Abort sequence", { ABORT: false }, { ABORT: { label: "Abort" }}, this.abort_state, "RW", "OneOfMany");
			}
			if (property.name == null || property.name == "AGENT_PAUSE_PROCESS") {
				indigo_define_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", "Sequencer", "Pause sequence", { PAUSE_WAIT: this.paused }, { PAUSE_WAIT: { label: "Pause (before next operation)" }}, this.pause_state, "RW", "OneOfMany");
			}
			if (property.name == null || property.name == "SEQUENCE_RESET") {
				indigo_define_switch_property(this.devices[0], "SEQUENCE_RESET", "Sequencer", "Reset sequence", { RESET: false }, { RESET: { label: "Reset" }}, "Ok", "RW", "OneOfMany");
			}
		}
	},
	
	on_change_property: function(property) {
		if (property.device == this.devices[0]) {
			if (property.name == "AGENT_ABORT_PROCESS") {
				if (this.sequence != null) {
					if (property.items.ABORT) {
						indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Busy");
						indigo_set_timer(indigo_sequencer_abort_handler, 0);
					}
				} else {
					indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Alert");
					this.abort_state = "Alert";
				}
			} else if (property.name == "AGENT_PAUSE_PROCESS") {
				if (this.sequence != null) {
					if (property.items.PAUSE_WAIT) {
						if (!this.paused) {
							if (this.capturing_batch) {
								indigo_change_switch_property(this.devices[2], "AGENT_PAUSE_PROCESS", { "PAUSE_WAIT": true });
							}
							this.paused = true;
							indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: true }, this.pause_state = "Busy");
							indigo_send_message("Sequence paused");
						}
					} else {
						if (this.paused) {
							if (this.capturing_batch) {
								indigo_change_switch_property(this.devices[2], "AGENT_PAUSE_PROCESS", { "PAUSE_WAIT": false });
							}
							this.paused = false;
							indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
							indigo_send_message("Sequence resumed");
						}
					}
				} else {
					this.paused = false;
					indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Alert");
				}
			} else if (property.name == "SEQUENCE_RESET") {
				if (property.items.RESET) {
					this.step = -1;
					indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step }, this.state = "Ok");
					this.paused = false;
					indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
					indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
					while (this.loop_level >= 0) {
						indigo_delete_property(this.devices[0], "LOOP_" + this.loop_level--);
					}
					indigo_update_switch_property(this.devices[0], "SEQUENCE_RESET", { RESET: false }, "Ok");
				}
			}
		}
	},
	
	abort: function() {
		this.wait_for_device = null;
		this.wait_for_name = null;
		this.wait_for_item = null;
		this.wait_for_value = null;
		this.wait_for_value_tolerance = null;
		this.sequence = null;
		if (this.paused) {
			this.paused = false;
			indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
		}
		for (var device in this.devices) {
			if (device != 0) {
				indigo_change_switch_property(this.devices[device], "AGENT_ABORT_PROCESS", { "ABORT": true });
			}
		}
		if (this.wait_for_timer != null) {
			indigo_cancel_timer(this.wait_for_timer);
			this.wait_for_timer = null;
		}
		indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.state = "Alert", "Sequence aborted at step " + this.step);
		indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
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
			indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
			indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, this.abort_state = "Ok");
			indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.state = "Busy");
			this.name = name;
			indigo_update_text_property(this.devices[0], "SEQUENCE_NAME", { NAME: this.name }, "Ok");
			while (this.loop_level >= 0) {
				indigo_delete_property(this.devices[0], "LOOP_" + this.loop_level--);
			}
			this.sequence = sequence;
			indigo_send_message("Sequence started");
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		}
	},

	next: function() {
		if (this.paused) {
			indigo_set_timer(indigo_sequencer_next_handler, 0.01);
		} else if (this.sequence != null) {
			current = this.sequence[++this.index];
			if (current != null) {
				this.step = current.step;
				this.progress = current.progress;
				this.exposure = current.exposure;
				this.capturing_batch = false;
				indigo_log(current.execute);
				indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.state = "Busy");
				eval("indigo_sequencer." + current.execute);
			} else {
				this.progress = this.progress_total;
				this.exposure = this.exposure_total;
				this.sequence = null;
				if (this.paused) {
					this.paused = false;
					indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
				}
				indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.state = "Ok");
				indigo_send_message("Sequence finished");
			}
		}
	},
	
	enter_loop: function() {
		this.loop_level++;
		indigo_define_number_property(this.devices[0], "LOOP_" + this.loop_level, "Sequencer", "Loop " + this.loop_level, { STEP: this.step, COUNT: 0 }, { STEP: { label: "Loop at", format: "%g", min: 0, max: 10000, step: 1 }, COUNT: { label: "Itreations elapsed", format: "%g", min: 0, max: 10000, step: 1 }}, "Ok", "RO");
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
	
	increment_loop: function(i) {
		indigo_update_number_property(this.devices[0], "LOOP_" + this.loop_level, { COUNT: i }, "Ok");
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
		
	exit_loop: function() {
		indigo_delete_property(this.devices[0], "LOOP_" + this.loop_level--);
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
	
	select_switch: function(device, property, item) {
		var items = { };
		items[item] = true;
		this.wait_for_device = device;
		this.wait_for_name = property;
		indigo_change_switch_property(device, property, items);
	},
	
	set_switch: function(device, property, item, value) {
		var items = { };
		items[item] = value;
		this.wait_for_device = device;
		this.wait_for_name = property;
		indigo_change_switch_property(device, property, items);
	},

	deselect_switch: function(device, property, item) {
		var items = { };
		items[item] = false;
		this.wait_for_device = device;
		this.wait_for_name = property;
		indigo_change_switch_property(device, property, items);
	},
	
	change_texts: function(device, property, items) {
		this.wait_for_device = device;
		this.wait_for_name = property;
		indigo_change_text_property(device, property, items);
	},

	change_numbers: function(device, property, items) {
		this.wait_for_device = device;
		this.wait_for_name = property;
		indigo_change_number_property(device, property, items);
	},
	
	warning: function(message) {
		indigo_send_message(message);
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
	
	failure: function(message) {
		this.wait_for_device = null;
		this.wait_for_name = null;
		this.wait_for_item = null;
		this.wait_for_value = null;
		this.wait_for_value_tolerance = null;
		this.sequence = null;
		indigo_send_message(message);
		if (this.paused) {
			this.paused = false;
			indigo_update_switch_property(this.devices[0], "AGENT_PAUSE_PROCESS", { PAUSE_WAIT: false }, this.pause_state = "Ok");
		}
		indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step, PROGRESS: this.progress, PROGRESS_TOTAL: this.progress_total, EXPOSURE: this.exposure, EXPOSURE_TOTAL: this.exposure_total }, this.state = "Alert", "Sequence failed at step " + this.step);
	},
	
	wait: function(seconds) {
		var result = indigo_set_timer(indigo_sequencer_next_handler, seconds);
		if (result >= 0) {
			this.wait_for_timer = result;
			indigo_send_message("Suspended for " + seconds + " seconds");
		} else {
			this.failure("Can't schedule timer in " + seconds + " seconds");
		}
	},

	wait_until: function(time) {
		var result;
		if (typeof time === "number") {
			result = indigo_set_timer_at(indigo_sequencer_next_handler, time);
		} else {
			result = indigo_set_timer_at_utc(indigo_sequencer_next_handler, time);
		}
		if (result >= 0) {
			this.wait_for_timer = result;
			indigo_send_message("Suspended until " + time + " UTC");
		} else {
			this.failure("Can't schedule timer at " + time + " UTC");
		}
	},

	evaluate: function(code) {
		eval(code);
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},

	send_message: function(message) {
		indigo_send_message(message);
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},

	load_config: function(name) {
		var agent = this.devices[1];
		var property = indigo_devices[agent].AGENT_CONFIG_LOAD;
		if (property != null && property.items[name] != null) {
			this.select_switch(agent, "AGENT_CONFIG_LOAD", name);
		} else {
			this.failure("Can't load configuration " + name);
		}
	},

	load_driver: function(name) {
		var property = indigo_devices["Server"].DRIVERS;
		if (property != null) {
			var item = property.items[name];
			if (item != null) {
				if (property.items[name]) {
					this.warning(name + " is already loaded");
				} else {
					this.select_switch("Server", "DRIVERS", name);
				}
			} else {
				this.failure("Can't load " + name);
			}
		} else {
			this.failure("Can't load drivers");
		}
	},
	
	unload_driver: function(name) {
		var property = indigo_devices["Server"].DRIVERS;
		if (property != null) {
			var item = property.items[name];
			if (item != null) {
				if (property.items[name]) {
					this.deselect_switch("Server", "DRIVERS", name);
				} else {
					this.warning(name + " is not loaded");
				}
			} else {
				this.failure("Can't unload " + name);
			}
		} else {
			this.failure("Can't unload drivers");
		}
	},
	
	select_imager_agent: function(agent) {
		this.devices[2] = agent;
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
		
	select_mount_agent: function(agent) {
		this.devices[3] = agent;
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
		
	select_guider_agent: function(agent) {
		this.devices[4] = agent;
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},

	select_imager_camera: function(camera) {
		var agent = this.devices[2];
		if (camera == undefined)
			camera = "No camera";
		var property = indigo_devices[agent].FILTER_CCD_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == camera) {
					if (property.items[name]) {
						this.warning(camera + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_CCD_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + camera);
	},

	select_filter_wheel: function(wheel) {
		var agent = this.devices[2];
		if (wheel == undefined)
			wheel = "No wheel";
		var property = indigo_devices[agent].FILTER_WHEEL_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == wheel) {
					if (property.items[name]) {
						this.warning(wheel + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_WHEEL_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + wheel);
	},

	select_focuser: function(focuser) {
		var agent = this.devices[2];
		if (focuser == undefined)
			focuser = "No focuser";
		var property = indigo_devices[agent].FILTER_FOCUSER_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == focuser) {
					if (property.items[name]) {
						this.warning(focuser + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_FOCUSER_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + focuser);
	},

	select_rotator: function(rotator) {
		var agent = this.devices[3];
		if (rotator == undefined)
			rotator = "No rotator";
		var property = indigo_devices[agent].FILTER_ROTATOR_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == rotator) {
					if (property.items[name]) {
						this.warning(rotator + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_ROTATOR_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + rotator);
	},

	select_mount: function(mount) {
		var agent = this.devices[3];
		if (mount == undefined)
			mount = "No mount";
		var property = indigo_devices[agent].FILTER_MOUNT_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == mount) {
					if (property.items[name]) {
						this.warning(mount + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_MOUNT_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select the " + mount);
	},

	select_dome: function(dome) {
		var agent = this.devices[3];
		if (dome == undefined)
			dome = "No dome";
		var property = indigo_devices[agent].FILTER_DOME_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == dome) {
					if (property.items[name]) {
						this.warning(dome + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_DOME_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select the " + dome);
	},

	select_gps: function(gps) {
		var agent = this.devices[3];
		if (gps == undefined)
			gps = "No GPS";
		var property = indigo_devices[agent].FILTER_GPS_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == gps) {
					if (property.items[name]) {
						this.warning(gps + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_GPS_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select the " + gps);
	},

	select_guider_camera: function(camera) {
		var agent = this.devices[4];
		if (camera == undefined)
			camera = "No camera";
		var property = indigo_devices[agent].FILTER_CCD_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == camera) {
					if (property.items[name]) {
						this.warning(camera + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_CCD_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + camera);
	},

	select_guider: function(guider) {
		var agent = this.devices[4];
		if (guider == undefined)
			guider = "No guider";
		var property = indigo_devices[agent].FILTER_GUIDER_LIST;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label == guider) {
					if (property.items[name]) {
						this.warning(guider + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_GUIDER_LIST", name);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + guider);
	},

	select_frame_type: function(type) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_FRAME_TYPE;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === type) {
					if (property.items[name]) {
						this.warning("Frame type '" + type + "' is already selected");
					} else {
						this.select_switch(agent, "CCD_FRAME_TYPE", name);
					}
					return;
				}
			}
			if (property.items[type] != undefined) {
				if (property.items[type]) {
					this.warning("Frame type " + type + " is already selected");
				} else {
					this.select_switch(agent, "CCD_FRAME_TYPE", type);
				}
			} else {
				this.failure("Frame type '" + type + "' is not available");
			}
		} else {
			this.failure("Can't select frame type '" + type + "'");
		}
	},

	select_image_format: function(format) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_IMAGE_FORMAT;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === format) {
					if (property.items[name]) {
						this.warning("Image format '" + format + "' is already selected");
					} else {
						this.select_switch(agent, "CCD_IMAGE_FORMAT", name);
					}
					return;
				}
			}
			if (property.items[format] != undefined) {
				if (property.items[format]) {
					this.warning("Image format " + format + " is already selected");
				} else {
					this.select_switch(agent, "CCD_IMAGE_FORMAT", format);
				}
			} else {
				this.failure("Image format '" + format + "' is not available");
			}
		} else {
			this.failure("Can't select image format '" + format + "'");
		}
	},

	select_camera_mode: function(mode) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_MODE;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === mode) {
					if (property.items[name]) {
						this.warning("Camera mode '" + mode + "' is already selected");
					} else {
						this.select_switch(agent, "CCD_MODE", name);
					}
					return;
				}
			}
			if (property.items[mode] != undefined) {
				if (property.items[mode]) {
					this.warning("Camera mode " + mode + " is already selected");
				} else {
					this.select_switch(agent, "CCD_MODE", mode);
				}
			} else {
				this.failure("Camera mode '" + mode + "' is not available");
			}
		} else {
			this.failure("Can't select camera mode '" + mode + "'");
		}
	},

	set_gain: function(value) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_GAIN;
		if (property != null  && property.items.GAIN != undefined) {
			this.change_numbers(agent, "CCD_GAIN", { GAIN: value });
		} else {
			this.failure("Can't set gain");
		}
	},

	set_offset: function(value) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_OFFSET;
		if (property != null  && property.items.OFFSET != undefined) {
			this.change_numbers(agent, "CCD_OFFSET", { OFFSET: value });
		} else {
			this.failure("Can't set offset");
		}
	},

	set_gamma: function(value) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_GAMMA;
		if (property != null  && property.items.GAMMA != undefined) {
			this.change_numbers(agent, "CCD_GAMMA", { GAMMA: value });
		} else {
			this.failure("Can't set gamma");
		}
	},

	select_program: function(program) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_PROGRAM;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === program) {
					if (property.items[name]) {
						this.warning("Program '" + program + "' is already selected");
					} else {
						this.select_switch(agent, "DSLR_PROGRAM", name);
					}
					return;
				}
			}
			if (property.items[program] != undefined) {
				if (property.items[program]) {
					this.warning("Program " + program + " is already selected");
				} else {
					this.select_switch(agent, "DSLR_PROGRAM", program);
				}
			} else {
				this.failure("Program '" + program + "' is not available");
			}
		} else {
			this.failure("Can't select program '" + program + "'");
		}
	},

	select_aperture: function(aperture) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_APERTURE;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === aperture) {
					if (property.items[name]) {
						this.warning("Aperture '" + aperture + "' is already selected");
					} else {
						this.select_switch(agent, "DSLR_APERTURE", name);
					}
					return;
				}
			}
			if (property.items[aperture] != undefined) {
				if (property.items[aperture]) {
					this.warning("Aperture " + aperture + " is already selected");
				} else {
					this.select_switch(agent, "DSLR_APERTURE", aperture);
				}
			} else {
				this.failure("Aperture '" + aperture + "' is not available");
			}
		} else {
			this.failure("Can't select aperture '" + aperture + "'");
		}
	},

	select_shutter: function(shutter) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_SHUTTER;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === shutter) {
					if (property.items[name]) {
						this.warning("Shutter '" + shutter + "' is already selected");
					} else {
						this.select_switch(agent, "DSLR_SHUTTER", name);
					}
					return;
				}
			}
			if (property.items[shutter] != undefined) {
				if (property.items[shutter]) {
					this.warning("Shutter " + shutter + " is already selected");
				} else {
					this.select_switch(agent, "DSLR_SHUTTER", shutter);
				}
			} else {
				this.failure("Shutter speed '" + shutter + "' is not available");
			}
		} else {
			this.failure("Can't select shutter speed '" + shutter + "'");
		}
	},

	select_iso: function(iso) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_ISO;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === iso) {
					if (property.items[name]) {
						this.warning("ISO '" + iso + "' is already selected");
					} else {
						this.select_switch(agent, "DSLR_ISO", name);
					}
					return;
				}
			}
			if (property.items[iso] != undefined) {
				if (property.items[iso]) {
					this.warning("ISO " + iso + " is already selected");
				} else {
					this.select_switch(agent, "DSLR_ISO", iso);
				}
			} else {
				this.failure("ISO '" + iso + "' is not available");
			}
		} else {
			this.failure("Can't select ISO '" + iso + "'");
		}
	},

	select_cooler: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_COOLER;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Cooler state " + name + " is already selected");
			} else {
				this.select_switch(agent, "CCD_COOLER", name);
			}
		} else {
			this.failure("Can't set cooler state");
		}
	},
	
	set_temperature: function(temperature) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_TEMPERATURE;
		if (property != null) {
			this.wait_for_item = "TEMPERATURE";
			this.wait_for_value = temperature;
			this.wait_for_value_tolerance = 1; // Allow large tolerance for the temperature. The property state will remain "Busy"
			                                   // until temperature reaches the driver's tolerance.
			this.change_numbers(agent, "CCD_TEMPERATURE", { TEMPERATURE: temperature });
		} else {
			this.failure("Can't set temperature");
		}
	},
	
	set_use_solver: function(use_solver) {
		this.use_solver = use_solver;
		indigo_set_timer(indigo_sequencer_next_handler, 0);
	},
	
	set_pause_after_transit: function(time) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_IMAGER_BATCH;
		if (property != null) {
			this.change_numbers(agent, "AGENT_IMAGER_BATCH", { PAUSE_AFTER_TRANSIT: time });
		} else {
			this.failure("Can't set pause after transit");
		}
	},
	
	set_imager_dithering: function(skip_frames) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_IMAGER_BATCH;
		if (property != null) {
			this.change_numbers(agent, "AGENT_IMAGER_BATCH", { FRAMES_TO_SKIP_BEFORE_DITHER: skip_frames });
		} else {
			this.failure("Can't set dithering on imager side");
		}
	},

	set_fits_header: function(keyword, value) {
		var agent = this.devices[2];
		this.change_texts(agent, "CCD_SET_FITS_HEADER", { "KEYWORD": keyword, "VALUE": value });
	},
	
	remove_fits_header: function(keyword, value) {
		var agent = this.devices[2];
		this.change_texts(agent, "CCD_REMOVE_FITS_HEADER", { "KEYWORD": keyword });
	},
	
	set_guider_dithering: function(amount, time_limit) {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_GUIDER_SETTINGS;
		if (property != null) {
			var items = { };
			if (amount != null) {
				items.DITHERING_MAX_AMOUNT = amount;
			}
			if (time_limit != null) {
				items.DITHERING_SETTLE_TIME_LIMIT = time_limit;
			}
			this.change_numbers(agent, "AGENT_GUIDER_SETTINGS", items);
		} else {
			this.failure("Can't set dithering on imager side");
		}
	},

	set_imager_feature: function(name, value) {
		var agent = this.devices[2];
		this.set_switch(agent, "AGENT_PROCESS_FEATURES", name, value);
	},
	
	select_filter: function(filter) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_WHEEL_FILTER;
		if (property != null) {
			for (var name in property.item_defs) {
				if (property.item_defs[name].label === filter) {
					if (property.items[name]) {
						this.warning("Filter '" + filter + "' is already selected");
					} else {
						this.select_switch(agent, "AGENT_WHEEL_FILTER", name);
					}
					return;
				}
			}
			if (property.items[filter] != undefined) {
				if (property.items[filter]) {
					this.warning("Filter " + filter + " is already selected");
				} else {
					this.select_switch(agent, "AGENT_WHEEL_FILTER", filter);
				}
			} else {
				this.failure("Filter '" + filter + "' is not available");
			}
		} else {
			this.failure("Can't select filter '" + filter + "'");
		}
	},
	
	set_local_mode: function(directory, prefix, object) {
		var agent = this.devices[2];
		var items = { };
		if (directory != null) {
			items.DIR = directory;
		}
		if (prefix != null) {
			if (!prefix.includes("XXX") && !prefix.includes("%"))
				prefix += "_%3S";
			items.PREFIX = prefix;
		}
		if (object != null) {
			items.OBJECT = object;
		}
		var property = indigo_devices[agent].CCD_LOCAL_MODE;
		if (property != null) {
			this.change_texts(agent, "CCD_LOCAL_MODE", items);
		} else {
			this.failure("Can't set name template");
		}
	},

	save_batch: function() {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_IMAGER_BATCH;
		if (property != null) {
			this.saved_batch = property.items;
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		} else {
			this.failure("Can't save batch");
		}
	},

	restore_batch: function() {
		var agent = this.devices[2];
		if (this.saved_batch != null) {
			this.change_numbers(agent, "AGENT_IMAGER_BATCH", this.saved_batch);
		} else {
			this.failure("Can't restore batch");
		}
	},

	set_upload_mode: function(mode) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_UPLOAD_MODE;
		if (property != null) {
			this.select_switch(agent, "CCD_UPLOAD_MODE", mode);
		} else {
			this.failure("Can't set upload mode");
		}
	},

	set_batch: function(count, exposure) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_IMAGER_BATCH;
		if (property != null) {
			var items = { };
			if (count != null)
				items.COUNT = count;
			if (exposure != null)
				items.EXPOSURE = exposure;
			this.change_numbers(agent, "AGENT_IMAGER_BATCH", items);
		} else {
			this.failure("Can't set batch");
		}
	},

	capture_batch: function() {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.capturing_batch = true;
			this.select_switch(agent, "AGENT_START_PROCESS", "EXPOSURE");
		} else {
			this.failure("Can't capture batch");
		}
	},

	capture_stream: function() {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "STREAMING");
		} else {
			this.failure("Can't capture stream");
		}
	},

	set_focuser_mode: function(mode) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "FOCUSER_MODE", mode);
		} else {
			this.failure("Can't change focuser mode");
		}
	},
	
	focus: function(ignore_failure) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.ignore_failure = ignore_failure;
			this.select_switch(agent, "AGENT_START_PROCESS", "FOCUSING");
		} else {
			this.failure("Can't focus");
		}
	},

	clear_focuser_selection: function() {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CLEAR_SELECTION");
		} else {
			this.failure("Can't clear selection");
		}
	},
	

	unpark: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].MOUNT_PARK;
		if (property != null) {
			if (property.items.PARKED) {
				this.select_switch(agent, "MOUNT_PARK", "UNPARKED");
			} else {
				this.warning("The mount is already unparked");
			}
		} else {
			this.failure("Can't unpark the mount");
		}
	},
	
	slew: function(ra, dec) {
		var agent = this.devices[3];
		var coordinates_property = indigo_devices[agent].AGENT_MOUNT_EQUATORIAL_COORDINATES;
		var process_property = indigo_devices[agent].AGENT_START_PROCESS;
		if (coordinates_property != null && process_property) {
			this.change_numbers(agent, "AGENT_MOUNT_EQUATORIAL_COORDINATES", { RA: ra, DEC: dec});
			this.select_switch(agent, "AGENT_START_PROCESS", "SLEW");
		} else {
			this.failure("Can't slew the mount");
		}
	},
	
	park: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].MOUNT_PARK;
		if (property != null) {
			if (property.items.UNPARKED) {
				this.select_switch(agent, "MOUNT_PARK", "PARKED");
			} else {
				this.warning("The mount is already parked");
			}
		} else {
			this.failure("Can't park the mount");
		}
	},
	
	home: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].MOUNT_PARK;
		if (property != null) {
			this.select_switch(agent, "MOUNT_HOME", "HOME");
		} else {
			this.failure("Can't home the mount");
		}
	},

	wait_for_gps: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].GPS_GEOGRAPHIC_COORDINATES;
		if (property != null) {
			if (property.state == "Busy") {
				indigo_send_message("Waiting for GPS");
				this.wait_for_device = agent;
				this.wait_for_name =  "GPS_GEOGRAPHIC_COORDINATES";
			} else {
				indigo_set_timer(indigo_sequencer_next_handler, 0);
			}
		} else {
			this.failure("Can't wait for GPS");
		}
	},
	
	set_guider_exposure: function(exposure) {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_GUIDER_SETTINGS;
		if (property != null) {
			this.change_numbers(agent, "AGENT_GUIDER_SETTINGS", { EXPOSURE: exposure });
		} else {
			this.failure("Can't set guider exposure");
		}
	},

	calibrate_guiding: function() {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CALIBRATION");
		} else {
			this.failure("Can't calibrate");
		}
	},

	start_guiding: function() {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			indigo_change_switch_property(agent, "AGENT_START_PROCESS", { GUIDING: true });
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		} else {
			this.failure("Can't start guiding");
		}
	},

	stop_guiding: function() {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_ABORT_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_ABORT_PROCESS", "ABORT");
		} else {
			this.failure("Can't stop guiding");
		}
	},

	clear_guider_selection: function() {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CLEAR_SELECTION");
		} else {
			this.failure("Can't clear selection");
		}
	},
	
	set_solver_exposure: function(exposure) {
		var agent = this.devices[5];
		var property = indigo_devices[agent].AGENT_PLATESOLVER_EXPOSURE;
		if (property != null) {
			this.change_numbers(agent, "AGENT_PLATESOLVER_EXPOSURE", { EXPOSURE: exposure });
		} else {
			this.failure("Can't set solver exposure");
		}
	},

	set_solver_target: function(ra, dec) {
		var agent = this.devices[5];
		var property = indigo_devices[agent].AGENT_PLATESOLVER_GOTO_SETTINGS;
		if (property != null) {
			this.change_numbers(agent, "AGENT_PLATESOLVER_GOTO_SETTINGS", { RA: ra, DEC: dec });
		} else {
			this.failure("Can't set solver target");
		}
	},

	precise_goto: function() {
		var agent = this.devices[5];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "PRECISE_GOTO");
		} else {
			this.failure("Can't initiate precise goto");
		}
	},

	sync_center: function() {
		var agent = this.devices[5];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CENTER");
		} else {
			this.failure("Can't sync and center");
		}
	},

	set_rotator_goto: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].ROTATOR_ON_POSITION_SET;
		if (property != null) {
			this.select_switch(agent, "ROTATOR_ON_POSITION_SET", "GOTO");
		} else {
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		}
	},

	set_rotator_angle: function(angle) {
		var agent = this.devices[3];
		property = indigo_devices[agent].ROTATOR_POSITION;
		if (property != null  && property.items.POSITION != undefined) {
			this.change_numbers(agent, "ROTATOR_POSITION", { POSITION: angle });
		} else {
			this.failure("Can't set rotator angle");
		}
	},

	set_focuser_goto: function() {
		var agent = this.devices[2];
		var property = indigo_devices[agent].FOCUSER_ON_POSITION_SET;
		if (property != null) {
			this.select_switch(agent, "FOCUSER_ON_POSITION_SET", "GOTO");
		} else {
			indigo_set_timer(indigo_sequencer_next_handler, 0);
		}
	},

	set_focuser_position: function(position) {
		var agent = this.devices[2];
		property = indigo_devices[agent].FOCUSER_POSITION;
		if (property != null  && property.items.POSITION != undefined) {
			this.change_numbers(agent, "FOCUSER_POSITION", { POSITION: position });
		} else {
			this.failure("Can't set focuser position");
		}
	}
};

function indigo_sequencer_next_handler() {
	indigo_sequencer.wait_for_timer = null;
	indigo_sequencer.next();
}

function indigo_sequencer_abort_handler() {
	indigo_sequencer.abort();
}

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
