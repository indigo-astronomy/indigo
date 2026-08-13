/*
 Copyright (c) 2017 GUIMakers, s. r. o. All rights reserved.
 You can use this software under the terms of 'INDIGO Astronomy open-source license' (see LICENSE.md).
 */

app.component('indigo-select-item', {
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
			<select class="form-select" style="cursor: pointer" :class="state()" @change="onChange">
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

app.component('indigo-edit-number', {
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-bs-toggle="tooltip" :title="tooltip">
			<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
			<span v-else class="input-group-text" :class="state()">{{icon}}</span>
			<template v-if="ident != null">
				<input v-if="property.perm == 'ro'" :id="ident" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else :id="ident" type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
			<template v-else>
				<input v-if="property.perm == 'ro'" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else type="text" class="form-control input-right" :value="value()" @change="onChange">
			</template>
			<template v-if="values != null">
				<button class="btn dropdown-toggle dropdown-toggle-split btn-outline-secondary" type="button" data-bs-toggle="dropdown"></button>
				<div class="dropdown-menu">
					<a class="dropdown-item" href="#" v-for="value in values" @click="change(value)">{{value}}</a>
				</div>
			</template>
		</div>`
});

app.component('indigo-edit-number-60', {
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
					if (this.ident != null) {
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-bs-toggle="tooltip" :title="tooltip">
			<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
			<div v-else class="input-group-text input-label" :class="state()">{{icon}}</div>
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

app.component('indigo-show-number', {
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-bs-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon.startsWith('glyphicons-')" class="glyphicons" :class="icon"/>
				<small v-else class="ms-1 p-1">{{icon}}</small>
				<small class="me-2">{{value()}}</small>
			</div>
		</div>`
});

app.component('indigo-show-number-60', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		state: function() {
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-bs-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon.startsWith('glyphicons-')" class="glyphicons" :class="icon"/>
				<small v-else class="ms-1 p-1">{{icon}}</small>
				<small class="me-2 p-1">{{value()}}</small>
			</div>
		</div>`
});

app.component('indigo-show-text', {
	props: {
		property: Object,
		name: String,
		icon: String,
		cls: String,
		tooltip: String
	},
	methods: {
		state: function() {
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-bs-toggle="tooltip" :title="tooltip">
			<div class="badge p-0 w-100 d-flex justify-content-between align-items-center" :class="state()">
				<small v-if="icon != null && icon.startsWith('glyphicons-')" cclass="glyphicons" :class="icon"/>
				<small v-else-if="icon != null" class="ms-1 p-1">{{icon}}</small>
				<small v-else class="ms-1 p-1"></small>
				<small class="me-2 p-1">{{value()}}</small>
			</div>
		</div>`
});

app.component('indigo-edit-text', {
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-100')" data-bs-toggle="tooltip" :title="tooltip">
			<span v-if="icon.startsWith('glyphicons-')" class="input-group-text glyphicons" :class="icon + ' ' + state()"></span>
			<span v-else class="input-group-text" :class="state()">{{icon}}</span>
			<input type="text" class="form-control" :value="item().value" @change="onChange">
		</div>`
});

app.component('indigo-stepper', {
	props: {
		property: Object,
		name: String,
		direction: Object,
		direction_left: String,
		direction_right: String,
		cls: String,
		tooltip: String
	},
	data: function() {
		return {
			localValue: null
		};
	},
	watch: {
		property: {
			handler: function() {
				this.localValue = this.value();
			},
			deep: true,
			immediate: true
		},
		name: function() {
			this.localValue = this.value();
		}
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
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
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
		<div v-if="property != null" class="input-group p-1" :class="(cls != null ? cls : 'w-50')" data-bs-toggle="tooltip" :title="tooltip">
			<button class="btn glyphicons glyphicons-arrow-left" :class="state()" @click="left(localValue)" type="button"></button>
			<input type="text" class="form-control input-right" v-model="localValue">
			<button class="btn glyphicons glyphicons-arrow-right" :class="state()" @click="right(localValue)" type="button"></button>
		</div>`
});

app.component('indigo-ctrl', {
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
			item.newValue = value;
		},
		reset: function(property) {
			for (i in property.items) {
				property.items[i].newValue = null;
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
		isAbsoluteUrl: function(value) {
			return value.startsWith('http:') || value.startsWith('https:');
		},
		isImage: function(value) {
			return value.endsWith('.jpeg');
		},
		localUrl: function(value) {
			return window.location.protocol + '//' + window.location.host + value;
		},
		openAll: function(id) {
			var body = document.getElementById("B_" + id);
			if (body == null)
				return;
			bootstrap.Collapse.getOrCreateInstance(body, { toggle: false }).show();
			body.querySelectorAll(".collapse").forEach(function(el) {
				bootstrap.Collapse.getOrCreateInstance(el, { toggle: false }).show();
			});
		},
		closeAll: function(id) {
			var body = document.getElementById("B_" + id);
			if (body == null)
				return;
			bootstrap.Collapse.getOrCreateInstance(body, { toggle: false }).show();
			body.querySelectorAll(".collapse").forEach(function(el) {
				bootstrap.Collapse.getOrCreateInstance(el, { toggle: false }).hide();
			});
		},
	},
	template: `
		<div class="accordion p-1 w-100">
			<div class="card bg-transparent mb-2" v-for="(device,deviceName) in devices">
				<div class="d-flex card-header p-0" :class="state(device)">
					<button :id="'H_' + deviceName.hashCode()" class="flex-grow-1 btn p-2 collapsed collapse-button" data-bs-toggle="collapse" :data-bs-target="'#B_' + deviceName.hashCode()" style="text-align:left;border:none;background:transparent;"><span class="icon-indicator"></span>{{deviceName}}</button>
					<button class="btn" @click.stop="closeAll(deviceName.hashCode())" style="border:none;background:transparent;" data-bs-toggle="tooltip" title="Collapse items">△</button>
					<button class="btn" @click.stop="openAll(deviceName.hashCode())" style="border:none;background:transparent;" data-bs-toggle="tooltip" title="Expand items">▽</button>
				</div>
					<div :id="'B_' + deviceName.hashCode()" class="accordion collapse p-2 bg-transparent">
					<div class="card bg-transparent mb-2" v-for="(group,groupName) in groups(device)">
						<div class="d-flex card-header p-0">
							<button :id="'H_' + deviceName.hashCode() + '_' + groupName.hashCode()" class="flex-grow-1 btn btn-outline-secondary p-2 collapsed collapse-button" data-bs-toggle="collapse" :data-bs-target="'#B_' + deviceName.hashCode() + '_' + groupName.hashCode()" style="text-align:left;border:none;background:transparent;color:black"><span class="icon-indicator"></span>{{groupName}}</button>
							<button class="btn" @click.stop="closeAll(deviceName.hashCode() + '_' + groupName.hashCode())" style="border:none;background:transparent;">△</button>
							<button class="btn" @click.stop="openAll(deviceName.hashCode() + '_' + groupName.hashCode())" style="border:none;background:transparent">▽</button>
						</div>
						<div :id="'B_' + deviceName.hashCode() + '_' + groupName.hashCode()" class="accordion collapse p-2">
							<div class="card mb-1" v-for="(property,name) in group">
								<button class="btn card-header p-2 collapsed collapse-button" :class="state(property)" data-bs-toggle="collapse" :data-bs-target="'#' + deviceName.hashCode() + '_' + groupName.hashCode() + '_' + name" style="text-align:left"><span class="icon-indicator"></span>{{property.label}}<small class="float-end">{{name}}</small></button>
								<div :id="deviceName.hashCode() + '_' + groupName.hashCode() + '_' + name" class="collapse card-body p-2 bg-light">
									<form class="m-0">
										<div v-if="property.message != null" class="alert alert-warning m-1" role="alert">
											{{property.message}}
										</div>
										<template v-if="property.type == 'text'">
											<div v-for="item in property.items" class="d-flex align-items-center gap-2 m-1">
												<label class="flex-grow-1 text-truncate mb-0">{{item.label}}</label>
												<input type="text" v-if="property.perm == 'ro'" readonly class="form-control flex-shrink-0" style="width:12rem" :value="item.value">
												<input type="text" v-else class="form-control flex-shrink-0" style="width:12rem" :class="dirty(item)" :value="value(item)" @input="newValue(item, $event.target.value)">
											</div>
											<template v-if="property.perm != 'ro'">
												<div class="float-end mt-1 me-1">
													<button type="submit" class="btn btn-sm btn-primary ms-1" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-outline-secondary ms-1" @click.prevent="reset(property)">Reset</button>
												</div>
											</template>
										</template>
										<template v-else-if="property.type == 'number'">
											<div v-for="item in property.items" class="d-flex align-items-center gap-1 m-1">
												<label class="flex-grow-1 text-truncate mb-0">{{item.label}}</label>
												<input type="text" v-if="property.perm == 'ro'" readonly class="form-control text-end flex-shrink-0" style="width:7rem" :class="dirty(item)" :value="format(item, item.value)">
												<template v-else>
													<input type="text" readonly class="form-control text-end flex-shrink-0" style="width:7rem" :value="format(item, item.value)">
													<input type="text" class="form-control text-end flex-shrink-0" style="width:7rem" :class="dirty(item)" :value="value(item)" @input="newValue(item, $event.target.value)">
												</template>
											</div>
											<template v-if="property.perm != 'ro'">
												<div class="float-end mt-1 me-1">
													<button type="submit" class="btn btn-sm btn-primary ms-1" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-outline-secondary ms-1" @click.prevent="reset(property)">Reset</button>
												</div>
											</template>
										</template>
										<template v-else-if="property.type == 'switch'">
											<div class="form-group row m-0">
												<div v-for="item in property.items" class="col-sm-3 p-0 m-0 pe-2" style="min-width: 15rem">
													<template v-if="property.perm == 'ro'">
														<button v-if="item.value && property.rule == 'OneOfMany'" disabled class="btn btn-sm btn-primary w-100 m-1">{{item.label}}</button>
														<button v-else disabled class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-outline-secondary'" @click.prevent="setSwitch(property, item.name, !item.value)">{{item.label}}</button>
													</template>
													<template v-else>
														<button v-if="item.value && property.rule == 'OneOfMany'" disabled class="btn btn-sm btn-primary w-100 m-1">{{item.label}}</button>
														<button v-else class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-outline-secondary'" @click.prevent="setSwitch(property, item.name, !item.value)">{{item.label}}</button>
												  </template>
												</div>
											</div>
										</template>
										<template v-else-if="property.type == 'light'">
											<div class="form-group row m-0">
												<div v-for="item in property.items" class="col-sm-3 p-0 m-0 pe-2" style="min-width: 15rem">
													<button disabled class="btn btn-sm w-100 m-1" :class="state(item)">{{item.label}}</button>
												</div>
											</div>
										</template>
										<template v-else-if="property.type == 'blob'">
											<div v-for="item in property.items">
												<template v-if="item.value != null && isAbsoluteUrl(item.value)">
													<a v-if="!isImage(item.value)" :href="item.value">{{item.value}}</a>
													<img v-else :src="item.value" class="img-fluid"/>
												</template>
												<template v-else-if="item.value != null">
													<a v-if="!isImage(item.value)" :href="localUrl(item.value)">{{localUrl(item.value)}}</a>
													<img v-else :src="localUrl(item.value)" class="img-fluid"/>
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

app.component('indigo-select-multi-item', {
	props: {
		property: Object,
		label: String,
		prefix: String,
		tooltip: String
	},
	methods: {
		items: function() {
			var result = [];
			if (this.property != null) {
				for (var i in this.property.items) {
					var item = this.property.items[i];
					if (item.name.startsWith(this.prefix)) result.push(item);
				}
			}
			return result;
		},
		value: function() {
			var result = null;
			if (this.property != null) {
				for (var i in this.property.items) {
					var item = this.property.items[i];
					if (item.value && item.name.startsWith(this.prefix)) result = result == null ? item.label : result + "; " + item.label;
				}
			}
			return result;
		},
		state: function() {
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
		},
		change: function(item) {
			var values = {};
			values[item.name] = !item.value;
			changeProperty(this.property.device, this.property.name, values);
		}
	},
	template: `
		<div class="input-group p-1" data-bs-toggle="tooltip" :title="tooltip">
			<span class="input-group-text" style="width: 10em;" :class="state()">{{label}}</span>
			<input readonly type="text" class="form-control" :value="value()">
			<button class="btn dropdown-toggle dropdown-toggle-split btn-outline-secondary" type="button" data-bs-toggle="dropdown"></button>
			<div class="dropdown-menu">
				<a class="dropdown-item" :class="item.value ? 'checked' : ''" href="#" v-for="item in items()" @click="change(item)"><span class="checkmark">{{item.value ? '✓' : ''}}</span>{{item.label}}</a>
			</div>
		</div>`
});

app.component('indigo-query-db', {
	props: {
		container: Object,
		dark: {
			type: Boolean,
			default: false
		}
	},
	data() {
		return {
			result: []
		};
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
				<div class="input-group-text btn-svg">&#x1f50d;</div>
				<input type="text" class="form-control" @change="onChange">
			</div>
			<div v-if="result != null && result.length > 0" class="list-group list-group-flush p-1 mt-1 w-100" style="max-height: 10rem; overflow-y: scroll">
				<a v-for="object in result" href="#" class="list-group-item list-group-item-action bg-transparent" :class="dark ? 'text-light' : 'text-dark'" @click="setTarget(object)">{{object.name}}</a>
			</div>
		</div>
		`
});


app.component('indigo-wifi-setup', {
	props: {
		ap_property: Object,
		infra_property: Object
	},
	data: function() {
		return {
			mode: "",
			ssid: "",
			password: ""
		};
	},
	watch: {
		ap_property: {
			handler: function() {
				this.reset();
			},
			deep: true,
			immediate: true
		},
		infra_property: {
			handler: function() {
				this.reset();
			},
			deep: true
		}
	},
	methods: {
		propertyValue: function(property, name) {
			if (property == null)
				return "";
			for (var i in property.items) {
				var item = property.items[i];
				if (item.name == name && item.value)
					return item.value;
			}
			return "";
		},
		loadModeValues: function() {
			if (this.mode == "AP") {
				this.ssid = this.propertyValue(this.ap_property, "SSID");
				this.password = this.propertyValue(this.ap_property, "PASSWORD");
			} else if (this.mode == "INFRA") {
				this.ssid = this.propertyValue(this.infra_property, "SSID");
				this.password = "";
			}
		},
		onChange: function() {
			this.loadModeValues();
		},
		set: function() {
			var values = {};
			values["SSID"] = this.ssid;
			values["PASSWORD"] = this.password;
			if (this.mode == "AP") {
				changeProperty(this.ap_property.device, this.ap_property.name, values);
			} else if (this.mode == "INFRA") {
				changeProperty(this.infra_property.device, this.infra_property.name, values);
			}
		},
		reset: function() {
			if (this.propertyValue(this.infra_property, "SSID") != "") {
				this.mode = "INFRA";
			} else if (this.propertyValue(this.ap_property, "SSID") != "") {
				this.mode = "AP";
			} else if (this.mode == "") {
				this.mode = "AP";
			}
			this.loadModeValues();
		}
	},	
	template: `
		<div class="w-100 d-flex flex-wrap">
			<div class="w-100 p-1">
				<select id="MODE" class="form-select ok-state" style="cursor: pointer" v-model="mode" @change="onChange">
					<option value="AP">Configure access point</option>
					<option value="INFRA">Join existing network</option>
				</select>
			</div>
			<div class="input-group p-1 w-100">
				<span class="input-group-text ok-state" style="width: 10em;">SSID</span>
				<input id="SSID" type="text" class="form-control" v-model="ssid">
			</div>
			<div class="input-group p-1 w-100">
				<span class="input-group-text ok-state" style="width: 10em;">Password</span>
				<input id="PASSWORD" type="text" class="form-control" v-model="password" :placeholder="mode == 'INFRA' ? '<value is hidden>' : ''">
			</div>
			<div class="d-flex w-100 mt-1 p-1">
				<button type="submit" class="btn btn-sm btn-primary ms-auto me-2" @click.prevent="set()">Submit</button>
				<button class="btn btn-sm btn-outline-secondary me-0" @click.prevent="reset()">Reset</button>
			</div>
		</div>
		`
});

app.component('indigo-sky-map', {
	props: {
		currentCoordinates: Array,
		targetCoordinates: Array,
		objectCoordinates: Array,
		geoCoordinates: Object,
		zoomLevel: {
			type: Number,
			default: 4
		},
		dark: {
			type: Boolean,
			default: false
		}
	},
	emits: [ 'select-object' ],
	data: function() {
		return {
			celestialConfig: null,
			initialized: false,
			localZoomLevel: this.zoomLevel,
			follow: 0,
			zooms: [ 500, 750, 1000, 1500, 2048, 2500 ],
			canvas: null,
			canvasClickHandler: null,
			resizeHandler: null
		};
	},
	mounted: function() {
		this.celestialConfig = this.createConfig();
		this.canvasClickHandler = this.canvasClick.bind(this);
		this.resizeHandler = this.resize.bind(this);
		window.addEventListener("resize", this.resizeHandler);
		this.displayMap();
	},
	beforeUnmount: function() {
		window.removeEventListener("resize", this.resizeHandler);
		if (this.canvas != null)
			this.canvas.removeEventListener("mousedown", this.canvasClickHandler);
		if (window.indigoSkyMapComponent == this)
			window.indigoSkyMapComponent = null;
	},
	watch: {
		currentCoordinates: {
			handler: function() {
				this.redrawMap();
			},
			deep: true
		},
		targetCoordinates: {
			handler: function() {
				this.redrawMap();
			},
			deep: true
		},
		objectCoordinates: {
			handler: function() {
				this.redrawMap();
			},
			deep: true
		},
		geoCoordinates: {
			handler: function() {
				this.redrawMap();
			},
			deep: true
		},
		zoomLevel: function(value) {
			if (value == this.localZoomLevel)
				return;
			this.localZoomLevel = value;
			this.displayMap();
		},
		dark: function() {
			this.displayMap();
		}
	},
	methods: {
		createConfig: function() {
			return {
				width: 2048,
				projection: "stereographic",
				transform: "equatorial",
				interactive: false,
				controls: false,
				follow: "zenith",
				background: { fill: "#fff", stroke: "#fff", opacity: 1, width: 1 },
				container: "map",
				datapath: "/data/",
				stars: {
					colors: true,
					proper: true,
					propernamelimit: 2,
					propernamestyle: { fill: "#999", font: "13px -apple-system, 'Segoe UI', 'Helvetica Neue', Arial, sans-serif", align: "right", baseline: "bottom" },
					style: { fill: "#000", opacity: 1 },
					size: 5,
					data: 'stars.json'
				},
				dsos: {
					show: true,
					names: true,
					desig: true,
					limit: 8,
					namelimit: 5,
					data: 'dsos.json',
				},
				constellations: {
					show: true,
					names: true,
					desig: true,
					lines: true,
					bounds: false,
					linestyle: { stroke: "#ccc", width: 1, opacity: 0.6 }
				},
				planets: {
					show: true,
					style: { fill: "#f00", font: "bold 17px 'Lucida Sans Unicode', Consolas, sans-serif", align: "center", baseline: "middle" },
					data: 'planets.json',
				},
				mw: {
					style: { fill:"#996", opacity: 0.1 }
				}
			};
		},
		applyTheme: function() {
			if (this.dark) {
				this.celestialConfig.background.fill = "#000";
				this.celestialConfig.background.stroke = "#000";
				this.celestialConfig.stars.style.fill = "#FFF";
			} else {
				this.celestialConfig.background.fill = "#fff";
				this.celestialConfig.background.stroke = "#fff";
				this.celestialConfig.stars.style.fill = "#000";
			}
		},
		applyZoomConfig: function() {
			this.celestialConfig.width = this.zooms[this.localZoomLevel];
			switch (this.localZoomLevel) {
				case 5:
				case 4:
					this.celestialConfig.stars.limit = 6;
					this.celestialConfig.stars.proper = true;
					this.celestialConfig.stars.propernamelimit = 2;
					this.celestialConfig.constellations.names = true;
					this.celestialConfig.dsos.names = true;
					this.celestialConfig.dsos.limit = 6;
					this.celestialConfig.dsos.namelimit = 4;
					break;
				case 3:
				case 2:
					this.celestialConfig.stars.limit = 4;
					this.celestialConfig.stars.proper = true;
					this.celestialConfig.stars.propernamelimit = 1.5;
					this.celestialConfig.constellations.names = true;
					this.celestialConfig.dsos.names = true;
					this.celestialConfig.dsos.limit = 5;
					this.celestialConfig.dsos.namelimit = 4;
					break;
				case 1:
				case 0:
					this.celestialConfig.stars.limit = 3;
					this.celestialConfig.stars.proper = false;
					this.celestialConfig.constellations.names = false;
					this.celestialConfig.dsos.names = false;
					break;
			}
		},
		updateCenter: function() {
			if (typeof Celestial === "undefined" || this.geoCoordinates == null)
				return;
			var latitude = this.geoCoordinates.latitude;
			var longitude = this.geoCoordinates.longitude;
			var pos = [ latitude, longitude ];
			this.celestialConfig.geopos = pos;
			this.celestialConfig.center = Celestial.getPoint(Celestial.horizontal.inverse(new Date(), [90, 0], pos), this.celestialConfig.transform);
		},
		addMarker: function() {
			window.indigoSkyMapComponent = this;
			if (window.indigoSkyMapMarkerAdded)
				return;
			Celestial.add({
				type: "marker",
				callback: function() {
				},
				redraw: function(error, json) {
					if (window.indigoSkyMapComponent != null)
						window.indigoSkyMapComponent.markerRedraw(error, json);
				}
			});
			window.indigoSkyMapMarkerAdded = true;
		},
		displayMap: function() {
			if (typeof Celestial === "undefined")
				return;
			if (this.celestialConfig == null)
				this.celestialConfig = this.createConfig();
			var firstDisplay = !this.initialized;
			this.addMarker();
			this.resize();
			this.applyTheme();
			this.applyZoomConfig();
			this.updateCenter();
			Celestial.display(this.celestialConfig);
			this.initialized = true;
			var self = this;
			this.$nextTick(function() {
				self.bindCanvas();
				self.exposeContainer();
				if (firstDisplay)
					self.centerInitialView();
				guiSetup();
			});
		},
		redrawMap: function() {
			if (typeof Celestial === "undefined")
				return;
			if (!this.initialized) {
				this.displayMap();
				return;
			}
			this.updateCenter();
			if (this.celestialConfig.center != null)
				Celestial.rotate({ center: this.celestialConfig.center });
			if (Celestial.redraw != null)
				Celestial.redraw();
			this.scrollMovingMount();
		},
		bindCanvas: function() {
			var map = this.$refs.map;
			if (map == null)
				return;
			var canvas = map.querySelector("canvas");
			if (canvas == null || canvas == this.canvas)
				return;
			if (this.canvas != null)
				this.canvas.removeEventListener("mousedown", this.canvasClickHandler);
			this.canvas = canvas;
			this.canvas.addEventListener("mousedown", this.canvasClickHandler, false);
		},
		exposeContainer: function() {
			if (typeof INDIGO !== "undefined" && Celestial.container != null)
				INDIGO.db = Celestial.container[0];
		},
		resize: function() {
			var map = this.$refs.map;
			if (map == null)
				return;
			if (window.innerWidth > 750) {
				if (window.innerHeight > 650)
					map.style.maxHeight = (window.innerHeight - 180) + "px";
				else
					map.style.maxHeight = "470px";
			} else {
				map.style.maxHeight = map.clientWidth + "px";
			}
		},
		centerInitialView: function() {
			var map = this.$refs.map;
			if (map == null)
				return;
			map.scrollLeft = (this.celestialConfig.width - map.clientWidth) / 2;
			map.scrollTop = (this.celestialConfig.width - map.clientWidth) / 2;
		},
		scrollToCoordinates: function(coordinates) {
			var map = this.$refs.map;
			if (map == null || coordinates == null || typeof Celestial === "undefined" || Celestial.mapProjection == null)
				return;
			var point = Celestial.mapProjection(coordinates);
			map.scrollLeft = point[0] - map.clientWidth / 2;
			map.scrollTop = point[1] - map.clientWidth / 2;
		},
		scrollMovingMount: function() {
			if (typeof INDIGO === "undefined" || INDIGO.findProperty == null)
				return;
			var eqCoordinates = INDIGO.findProperty("Mount Agent", "MOUNT_EQUATORIAL_COORDINATES");
			if (eqCoordinates != null && eqCoordinates.state == "Busy")
				this.scrollToCoordinates(this.currentCoordinates);
		},
		zoomIn: function() {
			if (this.localZoomLevel >= this.zooms.length - 1)
				return;
			this.setZoom(this.localZoomLevel + 1);
		},
		zoomOut: function() {
			if (this.localZoomLevel <= 0)
				return;
			this.setZoom(this.localZoomLevel - 1);
		},
		setZoom: function(value) {
			this.localZoomLevel = value;
			if (this.$root != null)
				this.$root.zoomLevel = value;
			this.displayMap();
		},
		centerMarker: function() {
			if (this.follow == 0) {
				this.scrollToCoordinates(this.currentCoordinates);
				this.follow = 1;
			} else {
				this.scrollToCoordinates(this.objectCoordinates);
				this.follow = 0;
			}
			this.redrawMap();
		},
		canvasClick: function(e) {
			if (typeof Celestial === "undefined" || Celestial.mapProjection == null)
				return;
			var coordinates = Celestial.mapProjection.invert([ e.offsetX, e.offsetY ]);
			var bestX = 0;
			var bestY = 0;
			var dist = Math.pow(coordinates[0] - bestX, 2) + Math.pow(coordinates[1] - bestY, 2);
			var paths = this.$refs.map.querySelectorAll("container path");
			for (var i = 0; i < paths.length; i++) {
				var data = paths[i].__data__;
				if (data == null) continue;
				var geometry = data.geometry;
				if (geometry == null) continue;
				if (geometry.type != "Point") continue;
				var d = Math.pow(coordinates[0] - geometry.coordinates[0], 2) + Math.pow(coordinates[1] - geometry.coordinates[1], 2);
				if (d < dist) {
					dist = d;
					bestX = geometry.coordinates[0];
					bestY = geometry.coordinates[1];
				}
			}
			this.$emit('select-object', { ra: this.deg2h(bestX), dec: bestY });
		},
		deg2h: function(ra) {
			return ra < 0 ? ra / 15 + 24 : ra / 15;
		},
		markerRedraw: function() {
			if (typeof Celestial === "undefined" || Celestial.context == null || Celestial.mapProjection == null)
				return;
			if (this.currentCoordinates != null) {
				Celestial.setStyle({ stroke: "#ff0000", width: 1 });
				var point = Celestial.mapProjection(this.currentCoordinates);
				Celestial.context.beginPath();
				Celestial.context.arc(point[0], point[1], 10, 0, 2 * Math.PI);
				Celestial.context.closePath();
				Celestial.context.stroke();
			}
			if (this.targetCoordinates != null) {
				var target = Celestial.mapProjection(this.targetCoordinates);
				Celestial.context.beginPath();
				Celestial.setStyle({ stroke: "#0000ff", width: 1 });
				Celestial.context.moveTo(target[0] - 15, target[1]);
				Celestial.context.lineTo(target[0] - 5, target[1]);
				Celestial.context.moveTo(target[0] + 5, target[1]);
				Celestial.context.lineTo(target[0] + 15, target[1]);
				Celestial.context.moveTo(target[0], target[1] - 15);
				Celestial.context.lineTo(target[0], target[1] - 5);
				Celestial.context.moveTo(target[0], target[1] + 5);
				Celestial.context.lineTo(target[0], target[1] + 15);
				Celestial.context.closePath();
				Celestial.context.stroke();
			}
			if (this.objectCoordinates != null) {
				var object = Celestial.mapProjection(this.objectCoordinates);
				Celestial.context.beginPath();
				Celestial.setStyle({ stroke: "#00a000", width: 1 });
				Celestial.context.moveTo(object[0] - 10, object[1] - 10);
				Celestial.context.lineTo(object[0] - 3, object[1] - 3);
				Celestial.context.moveTo(object[0] + 3, object[1] + 3);
				Celestial.context.lineTo(object[0] + 10, object[1] + 10);
				Celestial.context.moveTo(object[0] + 10, object[1] - 10);
				Celestial.context.lineTo(object[0] + 3, object[1] - 3);
				Celestial.context.moveTo(object[0] - 3, object[1] + 3);
				Celestial.context.lineTo(object[0] - 10, object[1] + 10);
				Celestial.context.closePath();
				Celestial.context.stroke();
			}
		}
	},
	template: `
		<div class="position-relative">
			<div id="map" ref="map" class="position-relative" style="overflow: scroll;"></div>
			<div v-if="initialized" class="position-absolute d-flex">
				<button class="btn btn-svg idle-state m-1" :disabled="localZoomLevel >= zooms.length - 1" @click.prevent="zoomIn" data-bs-toggle="tooltip" title="Zoom In">
					<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
						<path d="M27,14v4a1,1,0,0,1-1,1H19v7a1,1,0,0,1-1,1H14a1,1,0,0,1-1-1V19H6a1,1,0,0,1-1-1V14a1,1,0,0,1,1-1h7V6a1,1,0,0,1,1-1h4a1,1,0,0,1,1,1v7h7A1,1,0,0,1,27,14Z"/>
					</svg>
				</button>
				<button class="btn btn-svg idle-state m-1" :disabled="localZoomLevel <= 0" @click.prevent="zoomOut" data-bs-toggle="tooltip" title="Zoom Out">
					<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
						<path d="M26,14v4a1,1,0,0,1-1,1H7a1,1,0,0,1-1-1V14a1,1,0,0,1,1-1H25A1,1,0,0,1,26,14Z"/>
					</svg>
				</button>
				<button class="btn btn-svg idle-state m-1" @click.prevent="centerMarker" data-bs-toggle="tooltip" title="Center at marker">
					<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
						<path d="M16,4a8.9999,8.9999,0,0,0-9,9c0,6,6.7583,13.07764,8.16156,14.63135a1.13778,1.13778,0,0,0,1.67688,0C18.2417,26.07764,25,19,25,13A8.9999,8.9999,0,0,0,16,4Zm0,14a5,5,0,1,1,5-5A5.00013,5.00013,0,0,1,16,18Z"/>
					</svg>
				</button>
			</div>
		</div>
		`
});

app.component('indigo-internet-sharing', {
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
			<div v-for="item in property.items" class="col-sm-6 p-0 m-0 pe-2" style="min-width: 15rem">
				<button v-if="item.value" disabled class="btn btn-sm btn-primary w-100 m-1">Internet sharing {{item.label}}</button>
				<button v-else class="btn btn-sm w-100 m-1" :class="item.value ? 'btn-primary' : 'btn-outline-secondary'" @click.prevent="setSwitch(property, item.name, !item.value)">Internet sharing {{item.label}}</button>
			</div>
		</div>
		`
});

app.component('indigo-shutdown', {
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

INDIGO = app.mount('#ROOT');

function guiSetup() {
	document.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(function(el) {
		bootstrap.Tooltip.getOrCreateInstance(el);
	});
	localStorage.name = "indigo";
	if (localStorage.getItem("dark_mode")) {
		document.documentElement.setAttribute("data-theme", "dark");
		INDIGO.dark = true;
	} else {
		document.documentElement.removeAttribute("data-theme");
		INDIGO.dark = false;
	}
}

function setDarkMode() {
	localStorage.setItem("dark_mode", true);
	document.documentElement.setAttribute("data-theme", "dark");
	INDIGO.dark = true;
}

function setLightMode() {
	localStorage.removeItem("dark_mode");
	document.documentElement.removeAttribute("data-theme");
	INDIGO.dark = false;
}
