var onOpen = function (evt) {
	writeOK('CONNECTED to ' + URL);
	enumerateProperties();
}

var onClose = function (evt) {
	writeOK('DISCONNECTED from ' + URL);
}

var onError = function (evt) {
	writeAlert('ERROR' + evt.data);
}

var onMessage = function (evt) {
	var message = JSON.parse(evt.data);
	if ((property = message["defTextVector"]) != null) {
		processDefineProperty(property);
		defineProperty(property);
	} else if ((property = message["defNumberVector"]) != null) {
		processDefineProperty(property);
		defineProperty(property);
	} else if ((property = message["defSwitchVector"]) != null) {
		processDefineProperty(property);
		defineProperty(property);
	} else if ((property = message["defLightVector"]) != null) {
		processDefineProperty(property);
		defineProperty(property);
	} else if ((property = message["setTextVector"]) != null) {
		processUpdateProperty(property);
		updateProperty(property);
	} else if ((property = message["setNumberVector"]) != null) {
		processUpdateProperty(property);
		updateProperty(property);
	} else if ((property = message["setSwitchVector"]) != null) {
		processUpdateProperty(property);
		updateProperty(property);
	} else if ((property = message["setLightVector"]) != null) {
		processUpdateProperty(property);
		updateProperty(property);
	} else if ((property = message["deleteProperty"]) != null) {
		processDeleteProperty(property);
		deleteProperty(property);
	}
}

var devices = {};

var defineProperty = function (property) {
	writeProperty('DEFINE:', property);
}

var updateProperty = function (property) {
	writeProperty('UPDATE:', property);
}

var deleteProperty = function (property) {
	writeProperty('DELETE:', property);
}

var output;

function init() {
	console.log("init()");
	output = document.getElementById("output");
	websocket = new WebSocket(URL);
	websocket.onopen = onOpen;
	websocket.onclose = onClose;
	websocket.onmessage = onMessage;
	websocket.onerror = onError;
}

function writeToScreen(message) {
	var pre = document.createElement("pre");
	pre.style.wordWrap = "break-word";
	pre.innerHTML = message;
	output.appendChild(pre);
	pre.scrollIntoView(false);
}

function writeProperty(text, property) {
	writeToScreen('<div class="'+property.state+'">' + text + ' ' + JSON.stringify(property, null, 2) + '</div>');
}


function writeAlert(text) {
	writeToScreen('<div class="Alert">' + text + '</div>');
}

function writeWarning(text) {
	writeToScreen('<div class="Busy">' + text + '</div>');
}

function writeOK(text) {
	writeToScreen('<div class="Ok">' + text + '</div>');
}

function doSend(message) {
	websocket.send(message);
}

function processDefineProperty(property) {
	var device = property.device;
	var name = property.name;
	console.log("defineProperty("+device+" → "+name+")");
	var properties = devices[device];
	var values = {};
	for (var i in property.items) {
		var item = property.items[i];
		if (item.target != null)
			delete item["target"];
		values[item.name] = item.value;
	}
	property.values = values;
	if (properties == null) {
		devices[device] = { [name]: property};
	} else {
		properties[name] = property;
	}
}

function processUpdateProperty(property) {
	var device = property.device;
	var name = property.name;
	console.log("updateProperty("+device+" → "+name+")");
	var values = {};
	for (var i in property.items) {
		var item = property.items[i];
		values[item.name] = item.value;
	}
	property.values = values;
	var properties = devices[device];
	if (properties == null) {
		console.log("undefined device "+device);
	} else {
		var savedProperty = properties[name];
		if (savedProperty == null) {
			console.log("undefined property "+name);
		} else {
			for (var item in property.values) {
				savedProperty.values[item] = property.values[item];
			}
		}
	}
}

function processDeleteProperty(property) {
	var device = property.device;
	var name = property.name;
	console.log("deleteProperty("+device+" → "+name+")");
	var properties = devices[device];
	if (properties == null) {
		console.log("undefined device "+device);
	} else if (name == null) {
		delete devices[device];
	} else {
		delete properties[name];
	}
}

function enumerateProperties() {
	var message = { "getProperties": { "version": 0x200 } };
	writeToScreen('<span class="Ok">ENUMERATE: ' + JSON.stringify(message.getProperties, null, 2) + '</span>');
	doSend(JSON.stringify(message));
}

function changeProperty(deviceName, propertyName, values) {
	var items = []
	for (var name in values)
		items.push({ "name": name, "value": value = values[name] });
	property = { "device": deviceName, "name": propertyName, "items": items };
	writeToScreen('<span class="Ok">CHANGE: ' + JSON.stringify(property, null, 2) + '</span>');
	if (typeof value == "string")
		message = { "newTextVector": property };
	else if (typeof value == "number")
		message = { "newNumberVector": property };
	else if (typeof value == "boolean")
		message = { "newSwitchVector": property };
	doSend(JSON.stringify(message));
}
