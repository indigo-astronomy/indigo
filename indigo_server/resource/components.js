/*
 Copyright (c) 2017 GUIMakers, s. r. o. All rights reserved.
 You can use this software under the terms of 'INDIGO Astronomy open-source license' (see LICENSE.md).
 */

Vue.component('indigo-select-item', {
	props: {
		property: Object,
		no_value: String,
		cls: String
	},
	methods: {
		onChange: function(e) {
			var values = {};
			values[e.target.value] = true;
			changeProperty(this.property.device, this.property.name, values);
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		none_selected: function() {
			for (var i in this.property.items) {
				if (this.property.items[i].value) return false;
			}
			return true;
		}
	},
	template: `
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-100')">
			<select class="custom-select" style="cursor: pointer" :class="state()" @change="onChange">
				<template v-if="none_selected()">
					<option disabled>{{ no_value }}</option>
				</template>
				<template v-else>
					<option v-for="item in property.items" :selected="item.value" :value="item.name">
						{{ item.label }}
					</option>
				</template>
			</select>
		</div>`
});

Vue.component('indigo-edit-number', {
	props: {
		property: Object,
		enabler: Object,
		name: String,
		icon: String,
		values: Array,
		cls: String,
		ident: String,
		use_value: Boolean,
		tooltip: String
	},
	methods: {
		change: function(value) {
			var values = {};
			if (value === "Off") {
				if (this.enabler != null) {
					for (var i in this.enabler.items) {
						var item = this.enabler.items[i];
						if (item.name == "OFF" || item.name == "DISABLED") {
							if (!item.value) {
								values[item.name] = true;
								changeProperty(this.enabler.device, this.enabler.name, values);
								return;
							}
						}
					}
				} else {
					value = 0;
				}
			} else {
				if (typeof value == "string")
							value = parseFloat(value);
				if (this.enabler != null) {
					for (var i in this.enabler.items) {
						var item = this.enabler.items[i];
						if (item.name == "ON" || item.name == "ENABLED") {
							if (!item.value) {
								values[item.name] = true;
								changeProperty(this.enabler.device, this.enabler.name, values);
								values = {};
								break;
							}
						}
					}
				}
				values[this.name] = value;
				changeProperty(this.property.device, this.property.name, values);
			}
		},
		onChange: function(e) {
			this.change(e.target.value);
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null) return null;
			if (this.enabler != null) {
				for (var i in this.enabler.items) {
					var item = this.enabler.items[i];
					if (item.value && (item.name == "OFF" || item.name == "DISABLED")) return "Off";
				}
			}
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) {
					if (this.property.perm == "ro" || this.use_value)
						return item.value;
					return item.target;
				}
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-toggle="tooltip" :title="tooltip">
			<a class="input-group-prepend">
				<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
				<span v-else class="input-group-text" :class="state()">{{icon}}</span>
			</a>
			<template v-if="ident != null">
				<input v-if="property.perm == 'ro'" :id="ident" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else :id="ident" type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
			<template v-else>
				<input v-if="property.perm == 'ro'" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
			<div v-if="values != null" class="input-group-append">
				<button class="btn dropdown-toggle dropdown-toggle-split btn-outline-secondary" type="button" data-toggle="dropdown"></button>
				<div class="dropdown-menu">
					<a class="dropdown-item" href="#" v-for="value in values" @click="change(value)">{{value}}</a>
				</div>
			</div>
		</div>`
});

Vue.component('indigo-edit-number-60', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		ident: String,
		tooltip: String
	},
	methods: {
		change: function(value) {
			if (this.property == null)
				return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) {
					if (self.ident != null) {
						item.newValue = stod(value);
					} else {
						var values = {}
						values[item.name] = stod(value)
						changeProperty(this.property.device, this.property.name, values);
					}
					return;
				}
			}
		},
		onChange: function(e) {
			this.change(e.target.value);
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null)
				return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) {
					if (this.property.perm == "ro")
						return dtos(item.value)
					return dtos(item.newValue != null ? item.newValue : item.target)
				}
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-toggle="tooltip" :title="tooltip">
			<a class="input-group-prepend">
				<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
				<div v-else class="input-group-text input-label" :class="state()">{{icon}}</div>
			</a>
			<template v-if="ident != null">
				<input v-if="property.perm == 'ro'" :id="ident" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else :id="ident" type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
			<template v-else>
				<input v-if="property.perm == 'ro'" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
		</div>`
});

Vue.component('indigo-show-number', {
	props: {
		property: Object,
		enabler: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null) return null;
			if (this.enabler != null) {
				for (var i in this.enabler.items) {
					var item = this.enabler.items[i];
					if (item.value && (item.name == "OFF" || item.name == "DISABLED")) return "Off";
				}
			}
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) return item.value;
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon.startsWith('glyphicons-')" class="glyphicons" :class="icon"/>
				<small v-else class="ml-1 p-1">{{icon}}</small>
				<small class="mr-2">{{value()}}</small>
			</div>
		</div>`
});

Vue.component('indigo-show-number-60', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null)
				return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name)
					return dtos(item.value);
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon.startsWith('glyphicons-')" cclass="glyphicons" :class="icon"/>
				<small v-else class="ml-1 p-1">{{icon}}</small>
				<small class="mr-2 p-1">{{value()}}</small>
			</div>
		</div>`
});

Vue.component('indigo-show-text', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null)
				return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name)
					return item.value;
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon != null && icon.startsWith('glyphicons-')" cclass="glyphicons" :class="icon"/>
				<small v-else-if="icon != null" class="ml-1 p-1">{{icon}}</small>
				<small v-else class="ml-1 p-1"></small>
				<small class="mr-2 p-1">{{value()}}</small>
			</div>
		</div>`
});

Vue.component('indigo-edit-text', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		onChange: function(e) {
			var values = {};
			values[this.name] = e.target.value;
			changeProperty(this.property.device, this.property.name, values);
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		item: function() {
			if (this.property == null) return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) return item;
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-100')" data-toggle="tooltip" :title="tooltip">
		<div class="input-group-prepend">
			<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
			<span v-else class="input-group-text" :class="state()">{{icon}}</span>
		</div>
		<input type="text" class="form-control" :value="item().value" @change="onChange">
		</div>`
});

Vue.component('indigo-stepper', {
	props: {
		property: Object,
		name: String,
		direction: Object,
		direction_left: String,
		direction_right: String,
		cls: String,
		tooltip: String
	},
	methods: {
		left: function(value) {
			var values = {};
			values[this.direction_left] = true;
			changeProperty(this.direction.device, this.direction.name, values);
			values = {};
			if (typeof value == "string") value = parseFloat(value);
			values[this.name] = value;
			changeProperty(this.property.device, this.property.name, values);
		},
		right: function(value) {
			var values = {};
			values[this.direction_right] = true;
			changeProperty(this.direction.device, this.direction.name, values);
			values = {};
			if (typeof value == "string") value = parseFloat(value);
			values[this.name] = value;
			changeProperty(this.property.device, this.property.name, values);
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.property == null) return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) return item.value;
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-toggle="tooltip" :title="tooltip">
			<div class="input-group-prepend">
				<button class="btn glyphicons glyphicons-arrow-left" :class="state()" @click="left($($event.target).parent().next().val())" type="button"></button>
			</div>
			<input type="text" class="form-control input-right" :value="value()">
			<div class="input-group-append">
				<button class="btn glyphicons glyphicons-arrow-right" :class="state()" @click="right($($event.target).parent().prev().val())" type="button"></button>
			</div>
		</div>`
});

Vue.component('indigo-ctrl', {
	props: {
		devices: Object
	},
	methods: {
		groups: function(device) {
			var result = {};
			for (p in device) {
				var property = device[p];
				var group = result[property.group];
				if (group == null) {
					group = {};
					result[property.group] = group;
				}
				group[property.name] = property;
			}
			return result;
		},
		state: function(object) {
			if (object.state != null)
				return object.state.toLowerCase() + "-state";
			if (object.value != null)
				return object.value.toLowerCase() + "-state";
			for (p in object) {
				var property = object[p];
				if (property.name == "CONNECTION") {
					if (property.state == "Ok") {
						for (i in property.items) {
							var item = property.items[i];
							if (item.name == "CONNECTED" && item.value) {
								return "ok-state";
							}
						}
					}
					break;
				}
			}
			return "idle-state";
		},
		setSwitch: function(property, itemName, value) {
			var values = {};
			values[itemName] = value;
			changeProperty(property.device, property.name, values);
		},
		dirty: function(item) {
			if (item.newValue != null)
				return "dirty";
			return "";
		},
		format: function(item, value) {
			if (item.format != null && item.format.endsWith("m"))
				return dtos(value);
			return value;
		},
		value: function(item) {
			if (item.newValue != null)
				return item.newValue;
			var value = item.target != null ? item.target : item.value;
			if (item.format != null && item.format.endsWith("m"))
				return dtos(value);
			return value;
		},
		newValue: function(item, value) {
			Vue.set(item, 'newValue', value);
		},
		reset: function(property) {
			for (i in property.items) {
				Vue.set(property.items[i], 'newValue', null);
			}
		},
		set: function(property) {
			var values = {};
			for (i in property.items) {
				var item = property.items[i];
				var newValue = item.newValue;
				if (newValue != null) {
					if (property.type == "number") {
						if (item.format != null && item.format.endsWith("m"))
							newValue = stod(newValue);
						else
							newValue = parseFloat(newValue);
					}
				}
				values[item.name] = newValue != null ? newValue : (property.type == "number" ? item.target : item.value);
				item.newValue = null;
			}
			changeProperty(property.device, property.name, values);
		},
		openAll: function(id) {
			var header = $("#H_" + id);
			var body = $("#B_" + id);
			header.removeClass("collapsed");
			body.addClass("show");
			$(body).find("button.collapsed").removeClass("collapsed");
			$(body).find("div.collapse").addClass("show");
		},
		closeAll: function(id) {
			var body = $("#B_" + id);
			body.addClass("show");
			$(body).find("button.collapsed").addClass("collapsed");
			$(body).find("div.collapse").removeClass("show");
		},
	},
	template: `
		<div class="accordion p-1 w-100">
			<div class="card bg-transparent" v-for="(device,deviceName) in devices">
				<div class="input-group d-flex card-header p-0" :class="state(device)">
					<div class="input-group-prepend flex-grow-1">
								<button :id="'H_' + deviceName.hashCode()" class="btn p-2 collapsed collapse-button w-100" data-toggle="collapse" :data-target="'#B_' + deviceName.hashCode()" style="text-align:left;border:none;background:transparent;"><span class="icon-indicator"></span>{{deviceName}}</button>
					</div>
					<div class="input-group-append">
						<button class="btn" @click.stop="closeAll(deviceName.hashCode())" style="border:none;background:transparent;" data-toggle="tooltip" title="Collapse items">△</button>
					</div>
					<div class="input-group-append">
						<button class="btn" @click.stop="openAll(deviceName.hashCode())" style="border:none;background:transparent;" data-toggle="tooltip" title="Expand items">▽</button>
					</div>
				</div>
					<div :id="'B_' + deviceName.hashCode()" class="accordion collapse p-2 bg-transparent">
					<div class="card bg-transparent"  v-for="(group,groupName) in groups(device)">
						<div class="input-group d-flex card-header p-0">
							<div class="input-group-prepend flex-grow-1">
								<button :id="'H_' + deviceName.hashCode() + '_' + groupName.hashCode()" class="btn btn-outline-secondary p-2 collapsed collapse-button w-100" data-toggle="collapse" :data-target="'#B_' + deviceName.hashCode() + '_' + groupName.hashCode()" style="text-align:left;border:none;background:transparent;color:black"><span class="icon-indicator"></span>{{groupName}}</button>
							</div>
							<div class="input-group-append">
								<button class="btn" @click.stop="closeAll(deviceName.hashCode() + '_' + groupName.hashCode())" style="border:none;background:transparent;">△</button>
							</div>
							<div class="input-group-append">
								<button class="btn" @click.stop="openAll(deviceName.hashCode() + '_' + groupName.hashCode())" style="border:none;background:transparent">▽</button>
							</div>
						</div>
						<div :id="'B_' + deviceName.hashCode() + '_' + groupName.hashCode()" class="accordion collapse p-2">
							<div class="card" v-for="(property,name) in group">
								<button class="btn card-header p-2 collapsed collapse-button" :class="state(property)" data-toggle="collapse" :data-target="'#' + deviceName.hashCode() + '_' + groupName.hashCode() + '_' + name" style="text-align:left"><span class="icon-indicator"></span>{{property.label}}<small class="float-right">{{name}}</small></button>
								<div :id="deviceName.hashCode() + '_' + groupName.hashCode() + '_' + name" class="collapse card-block p-2 bg-light">
									<form class="m-0">
										<div v-if="property.message != null" class="alert alert-warning m-1" role="alert">
											{{property.message}}
										</div>
										<template v-if="property.type == 'text'">
											<div v-for="item in property.items" class="form-group row m-1">
												<label class="col-sm-4 col-form-label pl-0 mt-1">{{item.label}}</label>
												<input type="text" v-if="property.perm == 'ro'" readonly class="col-sm-8 form-control mt-1" :value="item.value">
												<input type="text" v-else class="col-sm-8 form-control mt-1" :class="dirty(item)" :value="value(item)" @input="newValue(item, $event.target.value)">
											</div>
											<template v-if="property.perm != 'ro'">
												<div class="float-right mt-1 mr-1">
													<button type="submit" class="btn btn-sm btn-primary ml-1" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-default ml-1" @click.prevent="reset(property)">Reset</button>
												</div>
											</template>
										</template>
										<template v-else-if="property.type == 'number'">
											<div v-for="item in property.items" class="form-group row m-1">
												<template v-if="property.perm == 'ro'">
													<label class="col-sm-9 col-form-label pl-0 mt-1">{{item.label}}</label>
													<input type="text" readonly class="col-sm-3 form-control mt-1" style="min-width: 5rem" :class="dirty(item)" :value="format(item, item.value)">
												</template>
												<template v-else>
													<label class="col-sm-5 col-form-label pl-0 mt-1">{{item.label}}</label>
													<input type="text" readonly class="col-sm-3 form-control mt-1" :value="format(item, item.value)" style="min-width: 5rem">
													<input type="text" class="col-sm-3 offset-sm-1 form-control mt-1" style="min-width: 5rem" :class="dirty(item)" :value="value(item)" @input="newValue(item, $event.target.value)">
												</template>
											</div>
											<template v-if="property.perm != 'ro'">
												<div class="float-right mt-1 mr-1">
													<button type="submit" class="btn btn-sm btn-primary ml-1" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-default ml-1" @click.prevent="reset(property)">Reset</button>
												</div>
											</template>
										</template>
										<template v-else-if="property.type == 'switch'">
											<div class="form-group row m-0">
												<div v-for="item in property.items" class="col-sm-3 p-0 m-0 pr-2" style="min-width: 15rem">
													<template v-if="property.perm == 'ro'">
														<button v-if="item.value && property.rule == 'OneOfMany'" disabled class="btn btn-sm btn-primary w-100 m-1">{{item.label}}</button>
														<button v-else disabled class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-default'" @click.prevent="setSwitch(property, item.name, !item.value)">{{item.label}}</button>
													</template>
													<template v-else>
														<button v-if="item.value && property.rule == 'OneOfMany'" disabled class="btn btn-sm btn-primary w-100 m-1">{{item.label}}</button>
														<button v-else class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-default'" @click.prevent="setSwitch(property, item.name, !item.value)">{{item.label}}</button>
												  </template>
												</div>
											</div>
										</template>
										<template v-else-if="property.type == 'light'">
											<div class="form-group row m-0">
												<div v-for="item in property.items" class="col-sm-3 p-0 m-0 pr-2" style="min-width: 15rem">
													<button disabled class="btn btn-sm w-100 m-1" :class="state(item)">{{item.label}}</button>
												</div>
											</div>
										</template>
										<template v-else-if="property.type == 'blob'">
											<div v-for="item in property.items">
												<template v-if="item.value != null && item.value.startsWith('http://')">
													<a v-if="!item.value.endsWith('.jpeg')" :href="item.value">{{item.value}}</a>
													<img v-else :src="item.value" class="img-fluid"/>
												</template>
												<template v-else-if="item.value != null">
													<a v-if="!item.value.endsWith('.jpeg')" :href="'http://' + window.location.hostname + ':' + window.location.port + item.value">{{"http://" + window.location.hostname + ":" + window.location.port + item.value}}</a>
													<img v-else :src="'http://' + window.location.hostname + ':' + window.location.port + item.value" class="img-fluid"/>
												</template>
											</div>
										</template>
										<template v-else>
											<small>{{property}}</small>
										</template>
									</form>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>`
});

Vue.component('indigo-select-multi-item', {
	props: {
		property: Object,
		label: String,
		prefix: String,
		tooltip: String
	},
	methods: {
		items: function() {
			var result = [];
			for (i in this.property.itemsByLabel) {
				var item = this.property.items[i];
				if (item.name.startsWith(this.prefix)) result.push(item);
			}
			return result;
		},
		value: function() {
			var result = null;
			for (i in this.property.itemsByLabel) {
				var item = this.property.items[i];
				if (item.value && item.name.startsWith(this.prefix)) result = result == null ? item.label : result + "; " + item.label;
			}
			return result;
		},
		state: function() {
			return this.property.state.toLowerCase() + "-state";
		},
		change: function(item) {
			var values = {};
			values[item.name] = !item.value;
			changeProperty(this.property.device, this.property.name, values);
		}
	},
	template: `
		<div class="input-group p-1" data-toggle="tooltip" :title="tooltip">
			<div class="input-group-prepend">
				<span class="input-group-text" id="inputGroup-sizing-default" style="width: 10em;" :class="state()">{{label}}</span>
			</div>
			<input readonly type="text" class="form-control" :value="value()">
			<div class="input-group-append">
				<button class="btn dropdown-toggle dropdown-toggle-split btn-outline-secondary" type="button" data-toggle="dropdown"></button>
				<div class="dropdown-menu">
					<a class="dropdown-item" :class="item.value ? 'checked' : ''" href="#" v-for="item in items()" @click="change(item)">{{item.label}}</a>
				</div>
			</div>
		</div>`
});

Vue.component('indigo-query-db', {
	props: {
		container: Object,
		result: Object
	},
	methods: {
		setTarget: function(object) {
			selectObject(object.ra, object.dec);
		},
		onChange: function(e) {
			var pattern = e.target.value.replace(" ", "").toUpperCase();
			var id = Number.parseInt(pattern);
			this.result = [];
			if (pattern != "") {
				var stars = $(this.container).children(".star");
				for (i in stars) {
					var data = stars[i].__data__;
					if (data == null) continue;
					var properties = data.properties;
					if (properties == null) continue;
					var geometry = data.geometry;
					if (geometry == null) continue;
					if (data.id == id || (properties.name != null && properties.name.toUpperCase().indexOf(pattern) >= 0)) {
						var name = properties.name;
						if (properties.desig != "") {
							if (name != "")
								name += ", ";
							name += properties.desig;
						}
						if (data.id > 0) {
							if (name != "")
								name += ", ";
							name += "HIP" + data.id;
						}
						this.result.push({ name: name, ra: deg2h(geometry.coordinates[0]), dec: geometry.coordinates[1] });
					}
				}				
				var dsos = $(this.container).children(".dso");
				for (i in dsos) {
					var data = dsos[i].__data__;
					if (data == null) continue;
					var properties = data.properties;
					if (properties == null) continue;
					var geometry = data.geometry;
					if (geometry == null) continue;
					if (data.id.toUpperCase().indexOf(pattern) >= 0 || (properties.desig != null && properties.desig.toUpperCase().indexOf(pattern) >= 0)) {
						var name = properties.name;
						if (name != "" && properties.desig != "")
							name += ", ";
						name += properties.desig;
						var properties = data.properties;
						this.result.push({ name: name, ra: deg2h(geometry.coordinates[0]), dec: geometry.coordinates[1] });
					}
				}				
			}
		}
	},
	template: `
		<div class="w-100">
			<div class="input-group p-1 w-100">
				<div class="input-group-prepend">
					<div class="input-group-text btn-svg">&#x1f50d;</div>
				</div>
				<input type="text" class="form-control" @change="onChange">			
			</div>
			<div v-if="this.result != null && this.result.length > 0" class="list-group list-group-flush p-1 mt-1 w-100" style="max-height: 10rem; overflow-y: scroll">
				<a v-for="object in this.result" href="#" class="list-group-item list-group-item-action bg-transparent" :class="INDIGO.dark ? 'text-light' : 'text-dark'" @click="setTarget(object)">{{object.name}}</a>
			</div>
		<div>
		`
});


Vue.component('indigo-wifi-setup', {
	props: {
		ap_property: Object,
		infra_property: Object
	},
	data: {
		mode: String
	},
	methods: {
		onChange: function(e) {
			self.mode = e.target.value;
			if (self.mode == "AP") {
				for (var i in this.ap_property.items) {
					var item = this.ap_property.items[i];
					if (item.name == "SSID") {
						$("#SSID").val(item.value);
					} else if (item.name == "PASSWORD") {
						$("#PASSWORD").val(item.value);
					}
				}
				$("#PASSWORD").removeAttr("placeholder");
			} else if (self.mode == "INFRA") {
				for (var i in this.infra_property.items) {
					var item = this.infra_property.items[i];
					if (item.name == "SSID") {
						$("#SSID").val(item.value);
					}
				}
				$("#PASSWORD").val("");
				$("#PASSWORD").attr("placeholder", "<value is hidden>");
			}
		},
		isAP: function() {
			for (var i in this.ap_property.items) {
				var item = this.ap_property.items[i];
				if (item.name == "SSID" && item.value) {
					self.mode = "AP";	
					return true;
				}
			}
			return false;
		},
		isInfra: function() {
			for (var i in this.infra_property.items) {
				var item = this.infra_property.items[i];
				if (item.name == "SSID" && item.value) {
					self.mode = "INFRA";	
					return true;
				}
			}
			return false;
		},
		value: function(name) {
			for (var i in this.ap_property.items) {
				var item = this.ap_property.items[i];
				if (item.name == name && item.value) {
					return item.value;
				}
			}
			for (var i in this.infra_property.items) {
				var item = this.infra_property.items[i];
				if (item.name == name && item.value) {
					return item.value;
				}
			}
			return "";
		},
		set: function() {
			var values = {};
			values["SSID"] = $("#SSID").val();
			values["PASSWORD"] = $("#PASSWORD").val();
			if (self.mode == "AP") {
				changeProperty(this.ap_property.device, this.ap_property.name, values);
			} else if (self.mode == "INFRA") {
				changeProperty(this.infra_property.device, this.infra_property.name, values);
			}
		},
		reset: function() {
			for (var i in this.ap_property.items) {
				var item = this.ap_property.items[i];
				if (item.name == "SSID") {
					if (item.value) {
						$("#MODE").val("AP")
						$("#PASSWORD").removeAttr("placeholder");
						$("#SSID").val(item.value);
					}
				} else if (item.name == "PASSWORD") {
					if (item.value) {
						$("#PASSWORD").val(item.value);
					}
				}
			}
			for (var i in this.infra_property.items) {
				var item = this.infra_property.items[i];
				if (item.name == "SSID") {
					if (item.value) {
						$("#MODE").val("INFRA")
						$("#PASSWORD").val("");
						$("#PASSWORD").attr("placeholder", "<value is hidden>");
						$("#SSID").val(item.value);
					}
				}
			}
		}
	},	
	template: `
		<div class="w-100 d-flex flex-wrap">
			<div class="w-100 p-1">
				<select id="MODE" class="custom-select ok-state" style="cursor: pointer" @change="onChange">
					<option :selected="isAP()" value="AP">Configure access point</option>
					<option :selected="isInfra()" value="INFRA">Join existing network</option>
				</select>
			</div>
			<div class="input-group p-1 w-100">
				<div class="input-group-prepend">
					<span class="input-group-text ok-state" style="width: 10em;">SSID</span>
				</div>
				<input id="SSID" type="text" class="form-control" :value="value('SSID')">
			</div>
			<div class="input-group p-1 w-100">
				<div class="input-group-prepend">
					<span class="input-group-text ok-state" style="width: 10em;">Password</span>
				</div>
				<input id="PASSWORD" v-if="isInfra()" type="text" class="form-control" value="indigosky" :value="value('PASSWORD')" placeholder="<value is hidden>">
				<input id="PASSWORD" v-else type="text" class="form-control" :value="value('PASSWORD')">
			</div>
			<div class="d-flex w-100 mt-1 p-1">
				<button type="submit" class="btn btn-sm btn-primary ml-auto mr-2" @click.prevent="set()">Submit</button>
				<button class="btn btn-sm btn-default mr-0" @click.prevent="reset()">Reset</button>
			</div>
		</div>
		`
});

Vue.component('indigo-internet-sharing', {
	props: {
		property: Object
	},
	methods: {
		setSwitch: function(property, itemName, value) {
			var values = {};
			values[itemName] = value;
			changeProperty(property.device, property.name, values);
		}
	},
	template: `
		<div class="w-100 d-flex flex-wrap p-1">
			<div v-for="item in property.items" class="col-sm-6 p-0 m-0 pr-2" style="min-width: 15rem">
				<button v-if="item.value" disabled class="btn btn-sm btn-primary w-100 m-1">Internet sharing {{item.label}}</button>
				<button v-else class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-default'" @click.prevent="setSwitch(property, item.name, !item.value)">Internet sharing {{item.label}}</button>
			</div>
		</div>
		`
});

Vue.component('indigo-shutdown', {
	props: {
		property: Object
	},
	methods: {
		shutdown: function() {
			if (confirm("Do you really want to shutdown?")) {
				var values = {};
				values["SHUTDOWN"] = true;
				changeProperty(this.property.device, this.property.name, values);
			}
		}
	},	
	template: `
		<div class="w-100 d-flex flex-wrap p-1">
			<button type="submit" class="btn btn-danger w-100" @click.prevent="shutdown()">Shutdown</button>
		</div>
		`
});

function guiSetup() {
	$('[data-toggle="tooltip"]').tooltip();
	localStorage.name = "indigo";
	if (localStorage.getItem("dark_mode")) {
		INDIGO.dark = true;
		$('body').removeClass("bg-secondary").addClass("bg-dark");
		$('input').removeClass("bg-light").addClass("bg-dark");
		$('input').removeClass("text-dark").addClass("text-light");
		$('textarea').removeClass("bg-light").addClass("bg-dark");
		$('textarea').removeClass("text-dark").addClass("text-light");
		$('div.bg-light').removeClass("bg-light").addClass("bg-secondary");
		$('canvas.bg-light').removeClass("bg-light").addClass("bg-secondary");
		if (typeof config !== 'undefined') {
			config.background.fill = "#FFF";
			config.stars.style.fill = "#000"
		}
	} else {
		INDIGO.dark = false;
		$('body').removeClass("bg-dark").addClass("bg-secondary");
		$('input').removeClass("bg-dark").addClass("bg-light");
		$('input').removeClass("text-light").addClass("text-dark");
		$('textarea').removeClass("bg-dark").addClass("bg-light");
		$('textarea').removeClass("text-light").addClass("text-dark");
		$('div.bg-secondary').removeClass("bg-secondary").addClass("bg-light");
		$('canvas.bg-secondary').removeClass("bg-secondary").addClass("bg-light");
	}
}

function setDarkMode() {
	localStorage.setItem("dark_mode", true);
	guiSetup();
	if (typeof config !== 'undefined') {
		config.background.fill = "#000";
		config.stars.style.fill = "#FFF"
		if (celestialVisible) {
			Celestial.display(config);
		}
	}
}

function setLightMode() {
	localStorage.removeItem("dark_mode");
	guiSetup();
	if (typeof config !== 'undefined') {
		config.background.fill = "#fff";
		config.stars.style.fill = "#000"
		if (celestialVisible) {
			Celestial.display(config);
		}
	}
}

