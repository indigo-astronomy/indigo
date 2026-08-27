/*
 Copyright (c) 2017-2025 CloudMakers, s. r. o. All rights reserved.
 You can use this software under the terms of 'INDIGO Astronomy open-source license' (see LICENSE.md).
*/

var app = Vue.createApp({
	data() {
		return {
			state: 'Connecting...',
			host: '',
			devices: { },
			dark: false,
			columns: 3,
			selectedProperty: null,
			scriptDirty: false,
			scriptSavedName: null,
			useAgent: false,
			connected: false,
			failed: false,
			message: false,
			calibrating: false,
			guiding: false,
			guiderGraphsEnabled: false,
			imagerFocuserMotionButton: null,
			imagerFocuserMotionState: "Idle",
			imagerFocuserMotionActive: false,
			currentCoordinates: null,
			targetCoordinates: null,
			objectCoordinates: null,
			geoCoordinates: null,
			zoomLevel: 4
		};
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
		scriptsProperties: function() {
			var self = this;
			function compare(a, b) {
				var aSeq = self.isSequenceScript(a);
				var bSeq = self.isSequenceScript(b);
				if (aSeq !== bSeq)
					return aSeq ? -1 : 1;
				if (a.name < b.name)
					return -1;
				if (a.name > b.name)
					return 1;
				return 0;
			}
			var result = [];
			var properties = INDIGO.devices['Scripting Agent'];
			var property;
			if ((property = properties['AGENT_SCRIPTING_ADD_SCRIPT']) != undefined)
				result.push(property);
			for (var name in properties) {
				var property = properties[name];
				if (property.name.startsWith('AGENT_SCRIPTING_SCRIPT_'))
					result.push(property);
			}
			return result.sort(compare);
		},
		newScript: function() {
			this.selectedProperty = null;
			this.scriptDirty = false;
			if (this.$refs.scriptEditor != null)
				this.$refs.scriptEditor.setProperty(null);
		},
		selectScript: function(property) {
			this.selectedProperty = property;
			this.scriptDirty = false;
		},
		scriptChanged: function() {
			this.scriptDirty = true;
		},
		scriptSaveProperty: function() {
			if (this.selectedProperty != null)
				return this.selectedProperty;
			return this.findProperty('Scripting Agent', 'AGENT_SCRIPTING_ADD_SCRIPT');
		},
		saveScript: function() {
			var property = this.scriptSaveProperty();
			if (property == null)
				return false;
			var editor = this.$refs.scriptEditor;
			if (editor == null)
				return false;
			/* Nothing was edited, so there is nothing to store. Execute and delete call this to
			   flush pending edits first and must not write the editor content back over an
			   untouched script. */
			if (!this.scriptDirty)
				return true;
			var isNew = this.selectedProperty == null;
			var values = {};
			values['NAME'] = editor.getName();
			values['SCRIPT'] = editor.getCode();
			this.scriptSavedName = isNew ? values['NAME'] : null;
			changeProperty('Scripting Agent', property.name, values);
			this.scriptDirty = false;
			return true;
		},
		scriptPropertyName: function(property) {
			if (property == null)
				return "";
			var item = property.item('NAME');
			if (item != null && item.value != null)
				return item.value;
			return property.label;
		},
		scriptDefined: function(property) {
			if (property == null)
				return;
			if (property.device != 'Scripting Agent' || !property.name.startsWith('AGENT_SCRIPTING_SCRIPT_'))
				return;
			if (this.selectedProperty != null && property.name == this.selectedProperty.name) {
				this.selectedProperty = property;
				return;
			}
			if (this.scriptSavedName == null)
				return;
			if (this.scriptPropertyName(property) == this.scriptSavedName) {
				this.selectScript(property);
				this.scriptSavedName = null;
			}
		},
		resetScript: function() {
			if (this.$refs.scriptEditor != null)
				this.$refs.scriptEditor.setProperty(this.selectedProperty);
			this.scriptDirty = false;
		},
		deleteScript: function() {
			var property = this.selectedProperty;
			if (property == null)
				return;
			if (!this.saveScript())
				return;
			var values = {};
			values[property.name] = true;
			changeProperty('Scripting Agent', 'AGENT_SCRIPTING_DELETE_SCRIPT', values);
			this.newScript();
		},
		isSequenceScript: function(property) {
			return this.scriptPropertyName(property).startsWith('#SEQUENCE');
		},
		sequenceScriptDisplayLabel: function(property) {
			var name = this.scriptPropertyName(property);
			if (name.startsWith('#SEQUENCE'))
				return name.substring(9).trimStart();
			return name;
		},
		sequenceStateProperty: function() {
			return this.findProperty('Scripting Agent', 'SEQUENCE_STATE');
		},
		sequenceRunning: function() {
			var prop = this.sequenceStateProperty();
			return prop != null && prop.state == 'Busy';
		},
		sequenceAbortProperty: function() {
			return this.findProperty('Scripting Agent', 'AGENT_ABORT_PROCESS');
		},
		sequencePauseProperty: function() {
			return this.findProperty('Scripting Agent', 'AGENT_PAUSE_PROCESS');
		},
		sequenceStateItem: function(name) {
				var prop = this.sequenceStateProperty();
				if (prop == null) return '-';
				var item = prop.item(name);
				return item != null ? item.value : '-';
			},
			sequenceControlsVisible: function() {
			return this.sequenceAbortProperty() != null || this.sequencePauseProperty() != null;
		},
		sequencePaused: function() {
			var prop = this.sequencePauseProperty();
			if (prop == null) return false;
			return prop.state == 'Busy';
		},
		abortSequence: function() {
			changeProperty('Scripting Agent', 'AGENT_ABORT_PROCESS', { ABORT: true });
		},
		togglePauseSequence: function() {
			changeProperty('Scripting Agent', 'AGENT_PAUSE_PROCESS', { PAUSE_WAIT: !this.sequencePaused() });
		},
		executeScript: function() {
			var property = this.selectedProperty;
			if (property == null)
				return;
			if (!this.saveScript())
				return;
			var values = {};
			values[property.name] = true;
			if (this.isSequenceScript(property)) {
				fetch('/Sequencer.js')
					.then(function(r) { return r.text(); })
					.then(function(text) {
						changeProperty('Scripting Agent', 'AGENT_SCRIPTING_RUN_SCRIPT', { SCRIPT: text });
						changeProperty('Scripting Agent', 'AGENT_SCRIPTING_EXECUTE_SCRIPT', values);
					})
					.catch(function(err) {
						console.error('Failed to fetch Sequencer.js:', err);
					});
			} else {
				changeProperty('Scripting Agent', 'AGENT_SCRIPTING_EXECUTE_SCRIPT', values);
			}
		},
		scriptSwitchValue: function(propertyName, property) {
			var scriptProperty = this.findProperty('Scripting Agent', propertyName);
			if (scriptProperty == null || property == null)
				return false;
			var item = scriptProperty.item(property.name);
			return item != null && item.value;
		},
		scriptOnLoadActive: function(property) {
			return this.scriptSwitchValue('AGENT_SCRIPTING_ON_LOAD_SCRIPT', property);
		},
		scriptOnUnloadActive: function(property) {
			return this.scriptSwitchValue('AGENT_SCRIPTING_ON_UNLOAD_SCRIPT', property);
		},
		toggleScriptSwitch: function(propertyName, property) {
			var scriptProperty = this.findProperty('Scripting Agent', propertyName);
			if (scriptProperty == null || property == null)
				return;
			var item = scriptProperty.item(property.name);
			if (item == null)
				return;
			var values = {};
			values[property.name] = !item.value;
			changeProperty('Scripting Agent', propertyName, values);
		},
		toggleScriptOnLoad: function(property) {
			this.toggleScriptSwitch('AGENT_SCRIPTING_ON_LOAD_SCRIPT', property);
		},
		toggleScriptOnUnload: function(property) {
			this.toggleScriptSwitch('AGENT_SCRIPTING_ON_UNLOAD_SCRIPT', property);
		}
	}
});
app.config.globalProperties.$ = $;
app.config.globalProperties.window = window;
var INDIGO = null;

function setState(msg) {
	var d = new Date();
	var h = d.getHours().toString().padStart(2, '0');
	var m = d.getMinutes().toString().padStart(2, '0');
	var s = d.getSeconds().toString().padStart(2, '0');
	INDIGO.state = '[' + h + ':' + m + ':' + s + '] ' + msg;
}

function init() {
	websocket = new WebSocket(indigoURL);
	websocket.onopen = onOpen;
	websocket.onclose = onClose;
	websocket.onmessage = onMessage;
	websocket.onerror = onError;
	setTimeout(checkState, 1000);
}

function onOpen(evt) {
	setState('Connected to ' + indigoURL.host);
	INDIGO.host = indigoURL.host;
	INDIGO.connected = true;
	INDIGO.failed = false;
	enumerateProperties();
}

function onClose(evt) {
	INDIGO.devices = { };
	setState('Lost connection to ' + indigoURL.host);
	INDIGO.connected = false;
	INDIGO.failed = true;
	INDIGO.message = false;
	setTimeout(init, 1000);
}

function onError(evt) {
	setState('Error' + evt);
	INDIGO.connected = false;
	INDIGO.failed = true;
	INDIGO.message = false;
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
	} else if ((msg = message["message"]) != null) {
		setState(msg);
		INDIGO.connected = false;
		INDIGO.failed = false;
		INDIGO.message = true;
	}
}

function enumerateProperties() {
	var message = { "getProperties": { "version": 0x200, "client": "WebGUI" } };
	doSend(JSON.stringify(message));
}

function changeProperty(deviceName, propertyName, values) {
	var dev = INDIGO.devices[deviceName];
	if (dev == null) {
		return;
	}
	var prop = dev[propertyName];
	if (prop == null) {
		return;
	}
	var hints = prop.hints;
	if (hints != null) {
		hints = hints.split(";")
		for (var i in hints) {
			var hint = hints[i];
			if (hint.startsWith("warn_on_change:")) {
				if (confirm(hint.substring(16, hint.length - 1))) {
					break;
				} else {
					return;
				}
			}
		}
	}
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
		INDIGO.devices[device] = { [name]: property };
	} else {
		properties[name] = property;
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
			savedProperty.state = property.state;
			savedProperty.message = property.message;
			if (property.message != null) {
				setState(property.message);
				INDIGO.connected = false;
				INDIGO.failed = false;
				INDIGO.message = true;
			}
			for (var i in property.items) {
				var item = property.items[i];
				for (var s in savedProperty.items) {
					var saved = savedProperty.items[s];
					if (item.name == saved.name) {
						saved.value = item.value;
						if (item.target != null)
							saved.target = item.target;
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
		delete INDIGO.devices[device];
	} else {
		onDeleteProperty(properties[name]);
		delete properties[name];
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

function timestamp() {
	 function pad(number) {
		 if (number < 10) {
			 return '0' + number;
		 }
		 return number;
	 }
	var d = new Date();
	var o = d.getTimezoneOffset();
	return d.getFullYear() +
	'-' + pad(d.getMonth() + 1) +
	'-' + pad(d.getDate()) +
	'T' + pad(d.getHours()) +
	':' + pad(d.getMinutes()) +
	':' + pad(d.getSeconds()) +
	(o >= 0 ? '-' + pad(o / 60) : '+' + pad(-o / 60)) + '00';
}
