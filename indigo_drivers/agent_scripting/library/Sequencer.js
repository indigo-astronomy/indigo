function Sequence() {
	this.step = 0
	this.sequence = [];
}

Sequence.prototype.wait = function(seconds) {
	this.sequence.push({ execute: 'wait(' + seconds + ')', step: this.step++ });
};

Sequence.prototype.evaluate = function(code) {
	this.sequence.push({ execute: 'evaluate("' + code + '")', step: this.step++ });
};

Sequence.prototype.send_message = function(message) {
	this.sequence.push({ execute: 'send_message("' + message + '")', step: this.step++ });
};

Sequence.prototype.load_config = function(name) {
	this.sequence.push({ execute: 'load_config("' + name + '")', step: this.step++ });
};

Sequence.prototype.load_driver = function(name) {
	this.sequence.push({ execute: 'load_driver("' + name + '")', step: this.step++ });
};

Sequence.prototype.unload_driver = function(name) {
	this.sequence.push({ execute: 'unload_driver("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_imager_agent = function(agent) {
	this.sequence.push({ execute: 'select_imager_agent("' + agent + '")', step: this.step++ });
};

Sequence.prototype.select_mount_agent = function(agent) {
	this.sequence.push({ execute: 'select_mount_agent("' + agent + '")', step: this.step++ });
};

Sequence.prototype.select_guider_agent = function(agent) {
	this.sequence.push({ execute: 'select_guider_agent("' + agent + '")', step: this.step++ });
};

Sequence.prototype.select_imager_camera = function(camera) {
	this.sequence.push({ execute: 'select_imager_camera("' + camera + '")', step: this.step++ });
};

Sequence.prototype.select_filter_wheel = function(wheel) {
	this.sequence.push({ execute: 'select_filter_wheel("' + wheel + '")', step: this.step++ });
};

Sequence.prototype.select_focuser = function(focuser) {
	this.sequence.push({ execute: 'select_focuser("' + focuser + '")', step: this.step++ });
};

Sequence.prototype.select_rotator = function(rotator) {
	this.sequence.push({ execute: 'select_rotator("' + rotator + '")', step: this.step++ });
};

Sequence.prototype.select_mount = function(mount) {
	this.sequence.push({ execute: 'select_mount("' + mount + '")', step: this.step++ });
};

Sequence.prototype.select_dome = function(dome) {
	this.sequence.push({ execute: 'select_dome("' + dome + '")', step: this.step++ });
};

Sequence.prototype.select_gps = function(gps) {
	this.sequence.push({ execute: 'select_gps("' + gps + '")', step: this.step++ });
};

Sequence.prototype.select_guider_camera = function(camera) {
	this.sequence.push({ execute: 'select_guider_camera("' + camera + '")', step: this.step++ });
};

Sequence.prototype.select_guider = function(guider) {
	this.sequence.push({ execute: 'select_guider("' + guider + '")', step: this.step++ });
};

Sequence.prototype.select_frame_type = function(name) {
	this.sequence.push({ execute: 'select_frame_type("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_camera_mode = function(name) {
	this.sequence.push({ execute: 'select_camera_mode("' + name + '")', step: this.step++ });
};

Sequence.prototype.set_gain = function(value) {
	this.sequence.push({ execute: 'set_gain(' + value + ')', step: this.step++ });
};

Sequence.prototype.set_offset = function(value) {
	this.sequence.push({ execute: 'set_offset(' + value + ')', step: this.step++ });
};

Sequence.prototype.set_gamma = function(value) {
	this.sequence.push({ execute: 'set_gamma(' + value + ')', step: this.step++ });
};

Sequence.prototype.select_program = function(name) {
	this.sequence.push({ execute: 'select_program("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_aperture = function(name) {
	this.sequence.push({ execute: 'select_aperture("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_shutter = function(name) {
	this.sequence.push({ execute: 'select_shutter("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_iso = function(name) {
	this.sequence.push({ execute: 'select_iso("' + name + '")', step: this.step++ });
};

Sequence.prototype.select_cooler = function(name) {
	this.sequence.push({ execute: 'select_cooler("' + name + '")', step: this.step++ });
};

Sequence.prototype.set_temperature = function(value) {
	this.sequence.push({ execute: 'set_temperature(' + value + ')', step: this.step++ });
};

Sequence.prototype.set_dithering = function(aggressivity, time_limit, skip_frames) {
	this.sequence.push({ execute: 'set_dithering(' + aggressivity + ',' + time_limit + ',' + skip_frames + ')', step: this.step++ });
};

Sequence.prototype.select_filter = function(name) {
	this.sequence.push({ execute: 'select_filter("' + name + '")', step: this.step++ });
};

Sequence.prototype.set_directory = function(directory) {
	this.sequence.push({ execute: 'set_local_mode("' + directory + '", null)', step: this.step++ });
};

Sequence.prototype.capture_batch = function(name_template, count, exposure) {
	this.sequence.push({ execute: 'set_local_mode(null, "' + name_template + '")', step: this.step });
	this.sequence.push({ execute: 'set_batch(' + count + ',' + exposure + ')', step: this.step });
	this.sequence.push({ execute: 'set_upload_mode("BOTH")', step: this.step });
	this.sequence.push({ execute: 'capture_batch()', step: this.step++ });
};

Sequence.prototype.capture_stream = function(name_template, count, exposure) {
	this.sequence.push({ execute: 'set_local_mode(null, "' + name_template + '")', step: this.step });
	this.sequence.push({ execute: 'set_batch(' + count + ',' + exposure + ', 0)', step: this.step });
	this.sequence.push({ execute: 'set_upload_mode("BOTH")', step: this.step });
	this.sequence.push({ execute: 'capture_stream()', step: this.step++ });
};

Sequence.prototype.focus = function(exposure) {
	this.sequence.push({ execute: 'save_batch()', step: this.step++ });
	this.sequence.push({ execute: 'set_batch(1,' + exposure + ', 0)', step: this.step });
	this.sequence.push({ execute: 'focus()', step: this.step });
	this.sequence.push({ execute: 'restore_batch()', step: this.step++ });
};

Sequence.prototype.park = function() {
	this.sequence.push({ execute: 'park()', step: this.step++ });
};

Sequence.prototype.unpark = function() {
	this.sequence.push({ execute: 'unpark()', step: this.step++ });
};

Sequence.prototype.slew = function(ra, dec) {
	this.sequence.push({ execute: 'slew(' + ra + ',' + dec + ')', step: this.step++ });
};

Sequence.prototype.wait_for_gps = function() {
	this.sequence.push({ execute: 'wait_for_gps()', step: this.step++ });
};

Sequence.prototype.calibrate_guiding = function() {
	this.sequence.push({ execute: 'calibrate_guiding()', step: this.step++ });
};

Sequence.prototype.start_guiding = function() {
	this.sequence.push({ execute: 'start_guiding()', step: this.step++ });
};

Sequence.prototype.stop_guiding = function() {
	this.sequence.push({ execute: 'stop_guiding()', step: this.step++ });
};

Sequence.prototype.sync_center = function(exposure) {
	this.sequence.push({ execute: 'set_solver_exposure(' + exposure + ')', step: this.step });
	this.sequence.push({ execute: 'sync_center()', step: this.step++ });
};

Sequence.prototype.start = function(imager_agent, mount_agent, guider_agent) {
	sequencer.devices[2] = imager_agent == undefined ? "Imager Agent" : imager_agent;
	sequencer.devices[3] = mount_agent == undefined ? "Mount Agent" : mount_agent;
	sequencer.devices[4] = guider_agent == undefined ? "Guider Agent" : guider_agent;
	sequencer.start(this.sequence);
};

var sequencer = {
	devices: [
		"Scripting Agent",
		"Configuration Agent",
		"Imager Agent",
		"Mount Agent",
		"Guider Agent",
		"Astrometry Agent",
		"Server"
	],

	state: "Ok",
	step: -1,
	index: -1,
	sequence: null,
	wait_for_device: null,
	wait_for_name: null,
	wait_for_item: null,
	wait_for_value: null,
	wait_for_timer: null,
	
	on_update: function(property) {
		if (property.device == this.wait_for_device && property.name == this.wait_for_name) {
			if (this.wait_for_item != null && this.wait_for_value != null) {
			indigo_log("wait_for_item '" + this.wait_for_item + "' -> " + property.items[this.wait_for_item]);
				if (property.items[this.wait_for_item] != this.wait_for_value) {
					return;
				}
			}
			indigo_log("wait_for '" + property.device + "', '" + property.name + "' -> " + property.state);
			if (property.state != "Busy") {
				this.wait_for_device = null;
				this.wait_for_name = null;
				this.wait_for_item = null;
				this.wait_for_value = null;
				if (property.state == "Ok") {
					indigo_set_timer(sequencer_next_handler, 0);
				} else if (property.state == "Alert") {
					this.failure("Sequence failed");
				}
			}
		}
	},
	
	on_enumerate_properties: function(property) {
		if (property.device == null || property.device == this.devices[0]) {
			if (property.name == null || property.name == "SEQUENCE_STATE") {
				indigo_define_number_property(this.devices[0], "SEQUENCE_STATE", "Sequencer", "State", { STEP: this.step }, { STEP: { label: "Executing step", format: "%g", min: -1, max: 10000, step: 1 }}, this.state, "RO");
			}
			if (property.name == null || property.name == "AGENT_ABORT_PROCESS") {
				indigo_define_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", "Sequencer", "Abort sequence", { ABORT: false }, { ABORT: { label: "Abort" }}, this.state, "RW", "OneOfMany");
			}
		}
	},
	
	on_change_property: function(property) {
		if (property.device == this.devices[0]) {
			if (property.name == "AGENT_ABORT_PROCESS") {
				if (property.items.ABORT) {
					indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, "Busy");
					indigo_set_timer(sequencer_abort_handler, 0);
				}
			}
		}
	},
	
	abort: function() {
		for (var device in this.devices) {
			if (device != 0) {
				this.select_switch(this.devices[device], "AGENT_ABORT_PROCESS", "ABORT");
			}
		}
		if (this.wait_for_timer != null) {
			indigo_cancel_timer(this.wait_for_timer);
			this.wait_for_timer = null;
		}
		this.failure("Sequence aborted");
		indigo_update_switch_property(this.devices[0], "AGENT_ABORT_PROCESS", { ABORT: false }, "Ok");
	},
	
	start: function(sequence) {
		if (sequencer.sequence != null) {
			indigo_send_message("Other sequence is executed");
		} else {
			sequencer.index = -1;
			sequencer.sequence = sequence;
			indigo_send_message("Sequence started");
			indigo_set_timer(sequencer_next_handler, 0);
		}
	},

	next: function() {
		current = this.sequence[++this.index];
		if (current != null) {
			this.step = current.step;
			this.state = "Busy";
			indigo_log(current.execute);
			eval("sequencer." + current.execute);
		} else {
			this.step = -1;
			this.state = "Ok";
			sequencer.sequence = null;
			indigo_send_message("Sequence finished");
		}
		indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step }, this.state);
	},
	
	select_switch: function(device, property, item) {
		var items = { };
		items[item] = true;
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
		indigo_set_timer(sequencer_next_handler, 0);
	},
	
	failure: function(message) {
		//this.step = -1;
		this.state = "Alert";
		this.wait_for_device = null;
		this.wait_for_name = null;
		this.wait_for_item = null;
		this.wait_for_value = null;
		this.sequence = null;
		indigo_update_number_property(this.devices[0], "SEQUENCE_STATE", { STEP: this.step }, this.state, message);
	},
	
	wait: function(seconds) {
		indigo_send_message("Suspened for " + seconds + " seconds");
		this.wait_for_timer = indigo_set_timer(sequencer_next_handler, seconds);
	},
		
	evaluate: function(code) {
		eval(code);
		indigo_set_timer(sequencer_next_handler, 0);
	},

	send_message: function(message) {
		indigo_send_message(message);
		indigo_set_timer(sequencer_next_handler, 0);
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
		indigo_set_timer(sequencer_next_handler, 0);
	},
		
	select_mount_agent: function(agent) {
		this.devices[3] = agent;
		indigo_set_timer(sequencer_next_handler, 0);
	},
		
	select_guider_agent: function(agent) {
		this.devices[4] = agent;
		indigo_set_timer(sequencer_next_handler, 0);
	},

	select_imager_camera: function(camera) {
		var agent = this.devices[2];
		if (camera == undefined)
			camera = "NONE";
		var property = indigo_devices[agent].FILTER_CCD_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == camera) {
					if (property.items[key]) {
						this.warning(camera + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_CCD_LIST", camera);
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
			wheel = "NONE";
		var property = indigo_devices[agent].FILTER_WHEEL_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == wheel) {
					if (property.items[key]) {
						this.warning(wheel + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_WHEEL_LIST", wheel);
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
			focuser = "NONE";
		var property = indigo_devices[agent].FILTER_FOCUSER_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == focuser) {
					if (property.items[key]) {
						this.warning(focuser + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_FOCUSER_LIST", focuser);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + focuser);
	},
	
	select_rotator: function(rotator) {
		var agent = this.devices[2];
		if (rotator == undefined)
			rotator = "NONE";
		var property = indigo_devices[agent].FILTER_ROTATOR_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == rotator) {
					if (property.items[key]) {
						this.warning(rotator + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_ROTATOR_LIST", rotator);
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
			mount = "NONE";
		var property = indigo_devices[agent].FILTER_MOUNT_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == mount) {
					if (property.items[key]) {
						this.warning(mount + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_MOUNT_LIST", mount);
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
			dome = "NONE";
		var property = indigo_devices[agent].FILTER_DOME_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == dome) {
					if (property.items[key]) {
						this.warning(dome + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_DOME_LIST", dome);
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
			gps = "NONE";
		var property = indigo_devices[agent].FILTER_GPS_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == gps) {
					if (property.items[key]) {
						this.warning(gps + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_GPS_LIST", gps);
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
			camera = "NONE";
		var property = indigo_devices[agent].FILTER_CCD_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == camera) {
					if (property.items[key]) {
						this.warning(camera + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_CCD_LIST", camera);
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
			guider = "NONE";
		var property = indigo_devices[agent].FILTER_GUIDER_LIST;
		if (property != null) {
			for (var key in property.items) {
				if (key == guider) {
					if (property.items[key]) {
						this.warning(guider + " is already selected");
					} else {
						this.select_switch(agent, "FILTER_GUIDER_LIST", guider);
					}
					return;
				}
			}
		}
		this.failure("Can't select " + guider);
	},
	
	select_frame_type: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_FRAME_TYPE;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Frame type " + name + " is already selected");
			} else {
				this.select_switch(agent, "CCD_FRAME_TYPE", name);
			}
		} else {
			this.failure("Can't select frame type");
		}
	},

	select_camera_mode: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].CCD_MODE;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Camera mode " + name + " is already selected");
			} else {
				this.select_switch(agent, "CCD_MODE", name);
			}
		} else {
			this.failure("Can't select camera mode");
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

	select_program: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_PROGRAM;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Program " + name + " is already selected");
			} else {
				this.select_switch(agent, "DSLR_PROGRAM", name);
			}
		} else {
			this.failure("Can't select program");
		}
	},

	select_aperture: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_APERTURE;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Aperture " + name + " is already selected");
			} else {
				this.select_switch(agent, "DSLR_APERTURE", name);
			}
		} else {
			this.failure("Can't select aperture");
		}
	},

	select_shutter: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_SHUTTER;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("Shutter " + name + " is already selected");
			} else {
				this.select_switch(agent, "DSLR_SHUTTER", name);
			}
		} else {
			this.failure("Can't select shutter");
		}
	},

	select_iso: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].DSLR_ISO;
		if (property != null && property.items[name] != undefined) {
			if (property.items[name]) {
				this.warning("ISO " + name + " is already selected");
			} else {
				this.select_switch(agent, "DSLR_ISO", name);
			}
		} else {
			this.failure("Can't select iso");
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
			this.change_numbers(agent, "CCD_TEMPERATURE", { TEMPERATURE: temperature });
		} else {
			this.failure("Can't set temperature");
		}
	},
	
	set_dithering: function(aggressivity, time_limit, skip_frames) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_IMAGER_DITHERING;
		if (property != null) {
			var items = { AGGRESSIVITY: aggressivity };
			if (time_limit != null)
				items.TIME_LIMIT = time_limit;
			if (skip_frames != null)
				items.SKIP_FRAMES = skip_frames;
			this.change_numbers(agent, "AGENT_IMAGER_DITHERING", items);
		} else {
			this.failure("Can't set dithering");
		}
	},

	select_filter: function(name) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_WHEEL_FILTER;
		if (property != null && property.items[name] != null) {
			if (property.items[name]) {
				this.warning("Filter " + name + " is already selected");
			} else {
				this.select_switch(agent, "AGENT_WHEEL_FILTER", name);
			}
		} else {
			this.failure("Can't select filter");
		}
	},
	
	set_local_mode: function(directory, prefix) {
		var agent = this.devices[2];
		var items = { }
		if (directory != null) {
			items.DIR = directory
		}
		if (prefix != null) {
			if (!prefix.includes("XXX") && !prefix.includes("%"))
				prefix += "_%3S";
			items.PREFIX = prefix;
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
			indigo_set_timer(sequencer_next_handler, 0);
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

	focus: function(exposure) {
		var agent = this.devices[2];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "FOCUSING");
		} else {
			this.failure("Can't focus");
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
	
	wait_for_gps: function() {
		var agent = this.devices[3];
		var property = indigo_devices[agent].GPS_GEOGRAPHIC_COORDINATES;
		if (property != null) {
			if (property.state == "Busy") {
				indigo_send_message("Waiting for GPS");
				this.wait_for_device = agent;
				this.wait_for_name =  "GPS_GEOGRAPHIC_COORDINATES";
			} else {
				indigo_set_timer(sequencer_next_handler, 0);
			}
		} else {
			this.failure("Can't wait for GPS");
		}
	},
	
	calibrate_guiding: function(exposure) {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CALIBRATION_AND_GUIDING");
		} else {
			this.failure("Can't calibrate");
		}
	},

	start_guiding: function(exposure) {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "GUIDING");
		} else {
			this.failure("Can't start guiding");
		}
	},

	stop_guiding: function(exposure) {
		var agent = this.devices[4];
		var property = indigo_devices[agent].AGENT_ABORT_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_ABORT_PROCESS", "ABORT");
		} else {
			this.failure("Can't stop guiding");
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

	sync_center: function() {
		var agent = this.devices[5];
		var property = indigo_devices[agent].AGENT_START_PROCESS;
		if (property != null) {
			this.select_switch(agent, "AGENT_START_PROCESS", "CENTER");
		} else {
			this.failure("Can't sync and center");
		}
	}
};

function sequencer_next_handler() {
	sequencer.wait_for_timer = null;
	sequencer.next();
}

function sequencer_abort_handler() {
	sequencer.abort();
}

indigo_delete_property("Scripting Agent", "SEQUENCE_STATE");
indigo_delete_property("Scripting Agent", "AGENT_ABORT_PROCESS");
indigo_event_handlers.sequencer = sequencer;
indigo_enumerate_properties();
indigo_send_message("Sequencer installed");
