var indigo_devices = { };
var indigo_event_handlers = { };
var indigo_timers = [ ];

function indigo_call_handlers(event, device_name, value, check_devices) {
	for (handler_name in indigo_event_handlers) {
		var handler = indigo_event_handlers[handler_name];
		if (check_devices && handler.devices && handler.devices.indexOf(device_name) == -1)
			continue;
		if (handler[event]) {
			handler[event](value);
		}
	}
}

function indigo_change_property(device, property, items) {
	for (var name in items) {
		var value = items[name];
		if (typeof value == "string")
			indigo_change_text_property(device, property, items);
		else if (typeof value == "number")
			indigo_change_number_property(device, property, items);
		else if (typeof value == "boolean")
			indigo_change_switch_property(device, property, items);
		break;
	}
}

function indigo_log_with_property(message, property) {
	indigo_log(message + " property '"+property.device+"'."+property.name+", state = "+property.state+", perm = "+property.perm);
	for (name in property.items) {
		var value = property.items[name];
		if (typeof value == "string")
			value = "'"+value+"'";
		if (typeof value == "object") {
			var str = "[";
			var next = false;
			for (field in value) {
				var subvalue = value[field];
				if (next)
					str += ", ";
				next = true;
				if (typeof subvalue == "string")
					subvalue = "'"+subvalue+"'";
				str += field + ": " + subvalue;
			}
			value = str + "]";
		}
		indigo_log("  "+name+" = "+value+"");
	}
	if (property.message)
		indigo_log("  message: "+message);
}

function indigo_error_with_property(message, property) {
	indigo_error(message + " property '"+property.device+"'."+property.name+", state = "+property.state+", perm = "+property.perm);
	for (name in property.items) {
		var value = property.items[name];
		if (typeof value == "string")
			value = "'"+value+"'";
		if (typeof value == "object") {
			var str = "[";
			var next = false;
			for (field in value) {
				var subvalue = value[field];
				if (next)
					str += ", ";
				next = true;
				if (typeof subvalue == "string")
					subvalue = "'"+subvalue+"'";
				str += field + ": " + subvalue;
			}
			value = str + "]";
		}
		indigo_error("  "+name+" = "+value+"");
	}
	if (property.message)
		indigo_error("  message: "+message);
}

function indigo_on_define_property(device_name, property_name, property_label, items, item_defs, state, perm, message) {
	var property = { device: device_name, name: property_name, label: property_label, items: items, item_defs: item_defs, state: state, perm: perm, message: message };
	var properties = indigo_devices[device_name];
	if (properties == null) {
		indigo_devices[device_name] = { [property_name]: property };
	} else {
		properties[property_name] = property;
	}
	property.change = function(items) {
		indigo_change_property(this.device, this.name, items);
	}
	indigo_call_handlers("on_define", device_name, property, true);
}

function indigo_on_update_property(device_name, property_name, items, state, message) {
	var properties = indigo_devices[device_name];
	if (properties == null) {
		return;
	} else {
		var saved_property = properties[property_name];
		if (saved_property == null) {
			return;
		} else {
			saved_property.state = state;
			for (var item_name in items) {
				var item_value = items[item_name];
				for (var saved_item_name in saved_property.items) {
					if (item_name == saved_item_name) {
						saved_property.items[saved_item_name] = item_value;
					}
				}
			}
		}
	}
	saved_property.message = message;
	indigo_call_handlers("on_update", device_name, saved_property, true);
}

function indigo_on_delete_property(device_name, property_name, message) {
	var properties = indigo_devices[device_name];
	if (properties == null) {
		return;
	} else if (property_name == null) {
		for (property_name in properties) {
			var property = properties[property_name];
			property.message = message;
			indigo_call_handlers("on_delete", device_name, property, true);
		}
		delete indigo_devices[device_name];
	} else {
		var property = properties[property_name];
		if (property) {
			property.message = message;
			indigo_call_handlers("on_delete", device_name, property, true);
			delete properties[property_name];
		}
	}
}

function indigo_on_send_message(device_name, message) {
	indigo_call_handlers("on_message", device_name, device_name+": "+message, true);
}

function indigo_on_enumerate_properties(device_name, property_name) {
	var property = { };
	if (device_name)
		property.device = device_name;
	if (property_name)
		property.name = property_name;
	indigo_call_handlers("on_enumerate_properties", device_name, property, false);
}

function indigo_on_change_property(device_name, property_name, items, state) {
	var property = { device: device_name, name: property_name, items: items, state: state };
	indigo_call_handlers("on_change_property", device_name, property, false);
}

indigo_log("Hight level scripting API installed");
