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
