/*
 Copyright (c) 2017 GUIMakers, s. r. o. All rights reserved.
 You can use this software under the terms of 'INDIGO Astronomy open-source license' (see LICENSE.md).
*/

var INDIGO = new Vue({
	el: '#ROOT',
	data: {
		state: 'Connecting...',
		host: '',
		devices: { }
		},
	methods: {
		findProperty: function(device, name) {
			if (INDIGO == null)
				return null;
			var properties = INDIGO.devices[device];
			if (properties == null)
				return null;
			return properties[name];
		},
	}
});

function init() {
	websocket = new WebSocket(URL);
	websocket.onopen = onOpen;
	websocket.onclose = onClose;
	websocket.onmessage = onMessage;
	websocket.onerror = onError;
	setTimeout(checkState, 1000);
}

function onOpen(evt) {
	INDIGO.state = 'Connected to ' + URL.host;
	INDIGO.host = URL.host;
	$('#SUCCESS').show()
	$('#FAILURE').hide()
	enumerateProperties();
}

function onClose(evt) {
	INDIGO.devices = { };
	INDIGO.state = 'Lost connection to ' + URL.host;
	$('#SUCCESS').hide()
	$('#FAILURE').show()
	setTimeout(init, 1000);
}

function onError(evt) {
	INDIGO.state ='Error' + evt;
	$('#SUCCESS').hide()
	$('#FAILURE').show()
}

function onMessage(evt) {
	var message = JSON.parse(evt.data);
	if ((property = message["defTextVector"]) != null) {
		property.type = "text";
		processDefineProperty(property);
	} else if ((property = message["defNumberVector"]) != null) {
		property.type = "number";
		processDefineProperty(property);
	} else if ((property = message["defSwitchVector"]) != null) {
		property.type = "switch";
		processDefineProperty(property);
	} else if ((property = message["defLightVector"]) != null) {
		property.type = "light";
		processDefineProperty(property);
	} else if ((property = message["defBLOBVector"]) != null) {
		property.type = "blob";
		processDefineProperty(property);
	} else if ((property = message["setTextVector"]) != null) {
		processUpdateProperty(property);
	} else if ((property = message["setNumberVector"]) != null) {
		processUpdateProperty(property);
	} else if ((property = message["setSwitchVector"]) != null) {
		processUpdateProperty(property);
	} else if ((property = message["setLightVector"]) != null) {
		processUpdateProperty(property);
	} else if ((property = message["setBLOBVector"]) != null) {
		processUpdateProperty(property);
	} else if ((property = message["deleteProperty"]) != null) {
		processDeleteProperty(property);
	}
}

function enumerateProperties() {
	var message = { "getProperties": { "version": 0x200 } };
	doSend(JSON.stringify(message));
}

function changeProperty(deviceName, propertyName, values) {
	var items = []
	for (var name in values)
		items.push({ "name": name, "value": value = values[name] });
	var property = { "device": deviceName, "name": propertyName, "items": items };
	var message;
	if (typeof value == "string")
		message = { "newTextVector": property };
	else if (typeof value == "number")
		message = { "newNumberVector": property };
	else if (typeof value == "boolean")
		message = { "newSwitchVector": property };
	doSend(JSON.stringify(message));
}

function doSend(message) {
	websocket.send(message);
}

String.prototype.hashCode = function() {
	var hash = 0, i = 0, len = this.length;
	while ( i < len ) {
		hash  = ((hash << 5) - hash + this.charCodeAt(i++)) << 0;
	}
	return ((hash + 2147483647) + 1).toString(16);
};

function compareLabels(a, b) {
	var A = a.label.toUpperCase();
	var B = b.label.toUpperCase();
	if (A < B)
		return -1;
	if (A > B)
		return 1;
	return 0;
}

function processDefineProperty(property) {
	var device = property.device;
	property.deviceId = device.hashCode();
	var name = property.name;
	property.itemsByLabel = [];
	for (i in property.items)
		property.itemsByLabel.push(property.items[i]);
	property.itemsByLabel.sort(compareLabels);
	property.item = function(name) {
		for (i in this.items) {
			if (this.items[i].name == name)
				return this.items[i];
		}
		return null;
	}
	var properties = INDIGO.devices[device];
	if (properties == null) {
		Vue.set(INDIGO.devices, device, { [name]: property});
	} else {
		Vue.set(properties, name, property);
	}
	onDefineProperty(property);
}

function processUpdateProperty(property) {
	var device = property.device;
	var name = property.name;
	var properties = INDIGO.devices[device];
	if (properties == null) {
		return;
	} else {
		var savedProperty = properties[name];
		if (savedProperty == null) {
			return;
		} else {
			Vue.set(savedProperty, "state", property.state);
			for (var i in property.items) {
				var item = property.items[i];
				for (var s in savedProperty.items) {
					var saved = savedProperty.items[s];
					if (item.name == saved.name) {
						Vue.set(saved, "value", item.value);
						if (item.target != null)
							Vue.set(saved, "target", item.target);
					}
				}
			}
			onUpdateProperty(savedProperty);
		}
	}
}

function processDeleteProperty(property) {
	var device = property.device;
	var name = property.name;
	var properties = INDIGO.devices[device];
	if (properties == null) {
		return;
	} else if (name == null) {
		Vue.delete(INDIGO.devices, device);
	} else {
		onDeleteProperty(properties[name]);
		Vue.delete(properties, name);
	}
}

function dtos(value) {
	var d = Math.abs(value);
	var m = 60.0 * (d - Math.floor(d));
	var s = 60.0 * (m - Math.floor(m));
	if (value < 0)
		d = -d;
	d = d | 0;
	m = m | 0;
	s = s | 0;
	return d.toString() + ':' + (m < 10 ? 0 + m.toString() : m.toString()) + ':' + (s < 10 ? 0 + s.toString() : s.toString())
}

function stod(str) {
	var strs = str.split(':');
	var d = 0;
	var m = 0;
	var s = 0;
	if (strs.length > 0) {
		d = parseInt(strs[0]);
	}
	if (strs.length > 1) {
		m = parseInt(strs[1]);
	}
	if (strs.length > 2) {
		s = parseInt(strs[2]);
	}
	if (d < 0)
		return d - m / 60.0 - s / 3600.0;
	return d + m / 60.0 + s / 3600.0;
}
