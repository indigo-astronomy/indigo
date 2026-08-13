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
		disabled: Boolean,
		displayValue: [String, Number],
		tooltip: String
	},
	methods: {
		optionLabel: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "label"))
				return option.label;
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionValue: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionIcon: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "icon"))
				return option.icon;
			return null;
		},
		optionIconCount: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "iconCount"))
				return option.iconCount;
			return 0;
		},
		optionHasIcons: function(option) {
			return this.optionIcon(option) != null && this.optionIconCount(option) > 0;
		},
		optionForValue: function(value) {
			if (this.values != null) {
				for (var i in this.values) {
					var option = this.values[i];
					if (this.optionValue(option) == value)
						return option;
				}
			}
			return value;
		},
		valueLabel: function(value) {
			if (this.values != null) {
				for (var i in this.values) {
					var option = this.values[i];
					if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value") && this.optionValue(option) == value)
						return this.optionLabel(option);
				}
			}
			return value;
		},
		change: function(option) {
			if (this.disabled)
				return;
			var value = this.optionValue(option);
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
			if (this.displayValue != null)
				return this.displayValue;
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
						return this.valueLabel(item.value);
					return this.valueLabel(item.target);
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
				<input v-else :id="ident" type="text" class="form-control input-right" :value="value()" :disabled="disabled" @change="onChange">
			</template>
			<template v-else>
				<input v-if="property.perm == 'ro'" readonly type="text" class="form-control input-right" :value="value()">
				<input v-else type="text" class="form-control input-right" :value="value()" :disabled="disabled" @change="onChange">
			</template>
			<template v-if="values != null">
				<button class="btn dropdown-toggle dropdown-toggle-split btn-outline-secondary" type="button" data-bs-toggle="dropdown" :disabled="disabled"></button>
				<div class="dropdown-menu">
					<a class="dropdown-item" href="#" v-for="value in values" @click="change(value)">{{optionLabel(value)}}</a>
				</div>
			</template>
		</div>`
});

app.component('indigo-number-dropdown', {
	props: {
		property: Object,
		name: String,
		icon: String,
		values: Array,
		cls: String,
		disabled: Boolean,
		displayValue: [String, Number],
		tooltip: String
	},
	methods: {
		optionLabel: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "label"))
				return option.label;
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionValue: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionIcon: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "icon"))
				return option.icon;
			return null;
		},
		optionIconCount: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "iconCount"))
				return option.iconCount;
			return 0;
		},
		optionHasIcons: function(option) {
			return this.optionIcon(option) != null && this.optionIconCount(option) > 0;
		},
		optionForValue: function(value) {
			if (this.values != null) {
				for (var i in this.values) {
					var option = this.values[i];
					if (this.optionValue(option) == value)
						return option;
				}
			}
			return value;
		},
		valueLabel: function(value) {
			if (this.values != null) {
				for (var i in this.values) {
					var option = this.values[i];
					if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value") && this.optionValue(option) == value)
						return this.optionLabel(option);
				}
			}
			return value;
		},
		change: function(option) {
			if (this.disabled || this.property == null || this.property.perm == "ro")
				return;
			var value = this.optionValue(option);
			if (typeof value == "string")
				value = parseFloat(value);
			var values = {};
			values[this.name] = value;
			changeProperty(this.property.device, this.property.name, values);
		},
		state: function() {
			return this.property == null ? null : this.property.state.toLowerCase() + "-state";
		},
		value: function() {
			if (this.displayValue != null)
				return this.displayValue;
			if (this.property == null)
				return null;
			for (var i in this.property.items) {
				var item = this.property.items[i];
				if (item.name == this.name) {
					if (this.property.perm == "ro")
						return this.valueLabel(item.value);
					return this.valueLabel(item.target);
				}
			}
			return null;
		},
		selectedOption: function() {
			return this.optionForValue(this.value());
		}
	},
	template: `
		<div v-if="property != null" class="dropdown p-1" :class="(cls != null ? cls : 'w-50')" data-bs-toggle="tooltip" :title="tooltip">
			<button class="btn dropdown-toggle w-100 d-flex align-items-center" :class="state()" type="button" data-bs-toggle="dropdown" :disabled="disabled || property.perm == 'ro'">
				<span v-if="icon != null && icon.startsWith('glyphicons-')" class="glyphicons me-2" :class="icon"></span>
				<span v-else-if="icon != null" class="me-2">{{icon}}</span>
				<span class="flex-grow-1 text-start indigo-icon-run">
					<template v-if="optionHasIcons(selectedOption())">
						<span v-for="n in optionIconCount(selectedOption())" class="glyphicons" :class="optionIcon(selectedOption())"></span>
					</template>
					<template v-else>{{value()}}</template>
				</span>
			</button>
			<div class="dropdown-menu">
				<a class="dropdown-item indigo-icon-run" href="#" v-for="value in values" @click.prevent="change(value)">
					<template v-if="optionHasIcons(value)">
						<span v-for="n in optionIconCount(value)" class="glyphicons" :class="optionIcon(value)"></span>
					</template>
					<template v-else>{{optionLabel(value)}}</template>
				</a>
			</div>
		</div>`
});

app.component('indigo-feature-number-dropdown', {
	props: {
		featureProperty: Object,
		numberProperty: Object,
		featureName: String,
		numberName: String,
		values: Array,
		cls: String,
		tooltip: String
	},
	methods: {
		optionLabel: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "label"))
				return option.label;
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionValue: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "value"))
				return option.value;
			return option;
		},
		optionEnabled: function(option) {
			if (option != null && typeof option == "object" && Object.prototype.hasOwnProperty.call(option, "enabled"))
				return option.enabled;
			return true;
		},
		featureItem: function() {
			if (this.featureProperty == null)
				return null;
			return this.featureProperty.item(this.featureName);
		},
		numberItem: function() {
			if (this.numberProperty == null)
				return null;
			return this.numberProperty.item(this.numberName);
		},
		ready: function() {
			return this.featureItem() != null && this.numberItem() != null;
		},
		isDisabled: function() {
			return !this.ready() || this.featureProperty.perm == "ro" || this.numberProperty.perm == "ro";
		},
		numberValue: function() {
			var item = this.numberItem();
			if (item == null)
				return null;
			if (this.numberProperty.perm == "ro")
				return item.value;
			return item.target;
		},
		sameNumber: function(left, right) {
			return Math.abs(Number(left) - Number(right)) < 0.000001;
		},
		selectedOption: function() {
			if (!this.ready())
				return null;
			var enabled = this.featureItem().value;
			var numberValue = this.numberValue();
			if (this.values != null) {
				for (var i in this.values) {
					var option = this.values[i];
					if (!enabled && !this.optionEnabled(option))
						return option;
					if (enabled && this.optionEnabled(option) && this.sameNumber(this.optionValue(option), numberValue))
						return option;
				}
			}
			return null;
		},
		value: function() {
			var option = this.selectedOption();
			if (option != null)
				return this.optionLabel(option);
			if (this.ready())
				return this.numberValue();
			return null;
		},
		change: function(option) {
			if (this.isDisabled())
				return;
			var value = this.optionValue(option);
			if (typeof value == "string")
				value = parseFloat(value);
			var featureValues = {};
			featureValues[this.featureName] = this.optionEnabled(option);
			changeProperty(this.featureProperty.device, this.featureProperty.name, featureValues);
			var numberValues = {};
			numberValues[this.numberName] = value;
			changeProperty(this.numberProperty.device, this.numberProperty.name, numberValues);
		},
		state: function() {
			if (!this.ready())
				return null;
			if (this.featureProperty.state == "Alert" || this.numberProperty.state == "Alert")
				return "alert-state";
			if (this.featureProperty.state == "Busy" || this.numberProperty.state == "Busy")
				return "busy-state";
			return this.numberProperty.state.toLowerCase() + "-state";
		}
	},
	template: `
		<div v-if="ready()" class="dropdown p-1" :class="(cls != null ? cls : 'w-50')" data-bs-toggle="tooltip" :title="tooltip">
			<button class="btn dropdown-toggle w-100 d-flex align-items-center" :class="state()" type="button" data-bs-toggle="dropdown" :disabled="isDisabled()">
				<span class="flex-grow-1 text-start text-truncate">{{value()}}</span>
			</button>
			<div class="dropdown-menu">
				<a class="dropdown-item" href="#" v-for="value in values" @click.prevent="change(value)">{{optionLabel(value)}}</a>
			</div>
		</div>`
});

app.component('indigo-autofocus-graph', {
	props: {
		startProperty: Object,
		statsProperty: Object,
		estimatorProperty: Object,
		cameraSelected: Boolean,
		focuserSelected: Boolean
	},
	data: function() {
		return {
			focusing: false,
			baselineSampleKey: null,
			lastSampleKey: null,
			currentSampleValue: null,
			samples: []
		};
	},
	watch: {
		startProperty: {
			handler: function() {
				this.onStartProcessUpdate();
			},
			deep: true,
			immediate: true
		},
		statsProperty: {
			handler: function() {
				this.onStatsUpdate();
			},
			deep: true
		},
		estimatorProperty: {
			handler: function() {
				this.reset();
			},
			deep: true
		}
	},
	methods: {
		itemValue: function(property, name) {
			if (property == null)
				return null;
			var item = property.item(name);
			if (item == null)
				return null;
			return Number(item.value);
		},
		estimatorIs: function(name) {
			if (this.estimatorProperty == null)
				return false;
			var item = this.estimatorProperty.item(name);
			return item != null && item.value;
		},
		metricName: function() {
			if (this.estimatorIs("RMS_CONTRAST"))
				return "RMS_CONTRAST";
			if (this.estimatorIs("BAHTINOV"))
				return "BAHTINOV_ERROR";
			return "HFD";
		},
		focusProcessActive: function() {
			if (this.startProperty == null || this.startProperty.state != "Busy")
				return false;
			var item = this.startProperty.item("FOCUSING");
			return item != null && item.value;
		},
		onStartProcessUpdate: function() {
			var active = this.focusProcessActive();
			if (active && !this.focusing)
				this.reset();
			this.focusing = active;
		},
		onStatsUpdate: function() {
			this.onStartProcessUpdate();
			if (!this.focusing || !this.cameraSelected || !this.focuserSelected || this.statsProperty == null)
				return;
			var metricName = this.metricName();
			var position = this.itemValue(this.statsProperty, "FOCUS_POSITION");
			var metric = this.itemValue(this.statsProperty, metricName);
			if (!isFinite(position) || !isFinite(metric))
				return;
			if (metricName == "BAHTINOV_ERROR") {
				if (metric < 0)
					return;
			} else if (metric <= 0) {
				return;
			}
			var key = metricName + ":" + position + ":" + metric;
			if (key == this.lastSampleKey || key == this.baselineSampleKey)
				return;
			this.lastSampleKey = key;
			var sample = {
				position: position,
				metric: metric
			};
			this.currentSampleValue = sample;
			for (var i in this.samples) {
				if (this.samples[i].position == position) {
					this.samples.splice(i, 1, sample);
					return;
				}
			}
			this.samples.push(sample);
			if (this.samples.length > 200)
				this.samples.shift();
		},
		reset: function() {
			this.samples = [];
			this.lastSampleKey = null;
			this.baselineSampleKey = null;
			this.currentSampleValue = null;
			if (this.statsProperty != null) {
				var metricName = this.metricName();
				var position = this.itemValue(this.statsProperty, "FOCUS_POSITION");
				var metric = this.itemValue(this.statsProperty, metricName);
				if (isFinite(position) && isFinite(metric))
					this.baselineSampleKey = metricName + ":" + position + ":" + metric;
			}
		},
		visible: function() {
			return this.cameraSelected && this.focuserSelected && this.samples.length > 0;
		},
		sortedSamples: function() {
			return this.samples.slice().sort(function(a, b) {
				return a.position - b.position;
			});
		},
		graphWidth: function() {
			return 256;
		},
		graphHeight: function() {
			return 128;
		},
		plotLeft: function() {
			return 8;
		},
		plotRight: function() {
			return this.graphWidth() - 8;
		},
		plotTop: function() {
			return 20;
		},
		plotBottom: function() {
			return this.graphHeight() - 20;
		},
		bounds: function() {
			var samples = this.sortedSamples();
			var xMin = samples[0].position;
			var xMax = samples[0].position;
			var yMin = samples[0].metric;
			var yMax = samples[0].metric;
			for (var i in samples) {
				var sample = samples[i];
				xMin = Math.min(xMin, sample.position);
				xMax = Math.max(xMax, sample.position);
				yMin = Math.min(yMin, sample.metric);
				yMax = Math.max(yMax, sample.metric);
			}
			if (xMin == xMax) {
				xMin -= 1;
				xMax += 1;
			}
			if (yMin == yMax) {
				var padding = Math.max(Math.abs(yMin) * 0.1, 1);
				yMin -= padding;
				yMax += padding;
			}
			return {
				xMin: xMin,
				xMax: xMax,
				yMin: yMin,
				yMax: yMax
			};
		},
		sampleX: function(sample) {
			var bounds = this.bounds();
			return this.plotLeft() + (sample.position - bounds.xMin) * (this.plotRight() - this.plotLeft()) / (bounds.xMax - bounds.xMin);
		},
		sampleY: function(sample) {
			var bounds = this.bounds();
			return this.plotBottom() - (sample.metric - bounds.yMin) * (this.plotBottom() - this.plotTop()) / (bounds.yMax - bounds.yMin);
		},
		linePath: function() {
			var path = "";
			var samples = this.sortedSamples();
			for (var i in samples) {
				var sample = samples[i];
				path += (path == "" ? "M " : " L ") + this.sampleX(sample) + " " + this.sampleY(sample);
			}
			return path;
		},
		currentSample: function() {
			return this.currentSampleValue;
		},
		bestSample: function() {
			if (this.samples.length == 0)
				return null;
			var best = this.samples[0];
			for (var i in this.samples) {
				var sample = this.samples[i];
				if (this.metricName() == "RMS_CONTRAST") {
					if (sample.metric > best.metric)
						best = sample;
				} else if (sample.metric < best.metric) {
					best = sample;
				}
			}
			return best;
		},
		viewBox: function() {
			return "0 0 " + this.graphWidth() + " " + this.graphHeight();
		}
	},
	template: `
		<div v-if="visible()" class="indigo-autofocus-graph">
			<svg :viewBox="viewBox()" preserveAspectRatio="none">
				<line class="indigo-autofocus-axis" :x1="plotLeft()" :y1="plotBottom()" :x2="plotRight()" :y2="plotBottom()"></line>
				<line v-for="(sample, index) in sortedSamples()" :key="'sample-' + index" class="indigo-autofocus-sample" :x1="sampleX(sample)" :y1="plotBottom()" :x2="sampleX(sample)" :y2="sampleY(sample)"></line>
				<path class="indigo-autofocus-line" :d="linePath()"></path>
				<template v-if="bestSample() != null">
					<circle class="indigo-autofocus-best" :cx="sampleX(bestSample())" :cy="sampleY(bestSample())" r="4"></circle>
				</template>
				<template v-if="currentSample() != null">
					<circle class="indigo-autofocus-current" :cx="sampleX(currentSample())" :cy="sampleY(currentSample())" r="4"></circle>
				</template>
			</svg>
		</div>`
});

app.component('indigo-star-selection-overlay', {
	props: {
		selectionProperty: Object,
		estimatorProperty: Object,
		cameraSelected: Boolean
	},
	data: function() {
		return {
			imageBox: {
				left: 0,
				top: 0,
				width: 0,
				height: 0,
				naturalWidth: 0,
				naturalHeight: 0
			}
		};
	},
	mounted: function() {
		this.imageElement = this.$el.parentElement == null ? null : this.$el.parentElement.querySelector("#image");
		this.updateImageMetricsHandler = () => this.updateImageMetrics();
		if (this.imageElement != null)
			this.imageElement.addEventListener("load", this.updateImageMetricsHandler);
		window.addEventListener("resize", this.updateImageMetricsHandler);
		if (typeof ResizeObserver != "undefined" && this.imageElement != null) {
			this.resizeObserver = new ResizeObserver(this.updateImageMetricsHandler);
			this.resizeObserver.observe(this.imageElement);
		}
		this.updateImageMetrics();
	},
	beforeUnmount: function() {
		if (this.imageElement != null && this.updateImageMetricsHandler != null)
			this.imageElement.removeEventListener("load", this.updateImageMetricsHandler);
		if (this.updateImageMetricsHandler != null)
			window.removeEventListener("resize", this.updateImageMetricsHandler);
		if (this.resizeObserver != null)
			this.resizeObserver.disconnect();
	},
	methods: {
		item: function(name) {
			if (this.selectionProperty == null)
				return null;
			if (typeof this.selectionProperty.item == "function")
				return this.selectionProperty.item(name);
			for (var i in this.selectionProperty.items) {
				var item = this.selectionProperty.items[i];
				if (item.name == name)
					return item;
			}
			return null;
		},
		itemValue: function(name) {
			var item = this.item(name);
			if (item == null)
				return null;
			return Number(item.value);
		},
		estimatorIs: function(name) {
			if (this.estimatorProperty == null)
				return false;
			var item = this.estimatorProperty.item(name);
			return item != null && item.value;
		},
		updateImageMetrics: function() {
			if (this.imageElement == null || this.$el.parentElement == null)
				return;
			var imageRect = this.imageElement.getBoundingClientRect();
			var parentRect = this.$el.parentElement.getBoundingClientRect();
			var imageBox = {
				left: imageRect.left - parentRect.left,
				top: imageRect.top - parentRect.top,
				width: imageRect.width,
				height: imageRect.height,
				naturalWidth: this.imageElement.naturalWidth,
				naturalHeight: this.imageElement.naturalHeight
			};
			if (
				this.imageBox.left != imageBox.left ||
				this.imageBox.top != imageBox.top ||
				this.imageBox.width != imageBox.width ||
				this.imageBox.height != imageBox.height ||
				this.imageBox.naturalWidth != imageBox.naturalWidth ||
				this.imageBox.naturalHeight != imageBox.naturalHeight
			) {
				this.imageBox = imageBox;
			}
		},
		visible: function() {
			return this.cameraSelected && this.selectionProperty != null && this.imageBox.width > 0 && this.imageBox.height > 0 && this.imageBox.naturalWidth > 0 && this.imageBox.naturalHeight > 0 && this.markers().length > 0;
		},
		overlayStyle: function() {
			return {
				left: this.imageBox.left + "px",
				top: this.imageBox.top + "px",
				width: this.imageBox.width + "px",
				height: this.imageBox.height + "px"
			};
		},
		markers: function() {
			var markers = [];
			if (this.selectionProperty == null)
				return markers;
			var count = this.itemValue("COUNT");
			var radius = this.itemValue("RADIUS");
			if (this.estimatorIs("BAHTINOV"))
				return markers;
			if (!this.estimatorIs("U_CURVE"))
				count = 1;
			if (!isFinite(count) || count < 1)
				count = 1;
			count = Math.min(Math.floor(count), 8);
			if (!isFinite(radius) || radius <= 0)
				radius = 1;
			for (var i = 0; i < count; i++) {
				var suffix = i == 0 ? "" : "_" + (i + 1);
				var x = this.itemValue("X" + suffix);
				var y = this.itemValue("Y" + suffix);
				if (isFinite(x) && isFinite(y) && x > 0 && y > 0) {
					markers.push({
						x: x,
						y: y,
						radius: radius
					});
				}
			}
			return markers;
		},
		markerStyle: function(marker) {
			var scaleX = this.imageBox.width / this.imageBox.naturalWidth;
			var scaleY = this.imageBox.height / this.imageBox.naturalHeight;
			var radius = marker.radius * Math.min(scaleX, scaleY);
			var size = 2 * radius;
			return {
				left: marker.x * scaleX + "px",
				top: marker.y * scaleY + "px",
				width: size + "px",
				height: size + "px"
			};
		}
	},
	template: `
		<div class="indigo-star-selection-overlay" :style="overlayStyle()">
			<template v-if="visible()">
				<span v-for="(marker, index) in markers()" :key="'star-selection-' + index" class="indigo-star-selection-marker" :style="markerStyle(marker)"></span>
			</template>
		</div>`
});

app.component('indigo-bahtinov-spikes-overlay', {
	props: {
		spikesProperty: Object,
		estimatorProperty: Object,
		cameraSelected: Boolean
	},
	data: function() {
		return {
			imageBox: {
				left: 0,
				top: 0,
				width: 0,
				height: 0,
				naturalWidth: 0,
				naturalHeight: 0
			}
		};
	},
	mounted: function() {
		this.imageElement = this.$el.parentElement == null ? null : this.$el.parentElement.querySelector("#image");
		this.updateImageMetricsHandler = () => this.updateImageMetrics();
		if (this.imageElement != null)
			this.imageElement.addEventListener("load", this.updateImageMetricsHandler);
		window.addEventListener("resize", this.updateImageMetricsHandler);
		if (typeof ResizeObserver != "undefined" && this.imageElement != null) {
			this.resizeObserver = new ResizeObserver(this.updateImageMetricsHandler);
			this.resizeObserver.observe(this.imageElement);
		}
		this.updateImageMetrics();
	},
	beforeUnmount: function() {
		if (this.imageElement != null && this.updateImageMetricsHandler != null)
			this.imageElement.removeEventListener("load", this.updateImageMetricsHandler);
		if (this.updateImageMetricsHandler != null)
			window.removeEventListener("resize", this.updateImageMetricsHandler);
		if (this.resizeObserver != null)
			this.resizeObserver.disconnect();
	},
	methods: {
		item: function(property, name) {
			if (property == null)
				return null;
			if (typeof property.item == "function")
				return property.item(name);
			for (var i in property.items) {
				var item = property.items[i];
				if (item.name == name)
					return item;
			}
			return null;
		},
		itemValue: function(property, name) {
			var item = this.item(property, name);
			if (item == null)
				return null;
			return Number(item.value);
		},
		estimatorIs: function(name) {
			var item = this.item(this.estimatorProperty, name);
			return item != null && item.value;
		},
		updateImageMetrics: function() {
			if (this.imageElement == null || this.$el.parentElement == null)
				return;
			var imageRect = this.imageElement.getBoundingClientRect();
			var parentRect = this.$el.parentElement.getBoundingClientRect();
			var imageBox = {
				left: imageRect.left - parentRect.left,
				top: imageRect.top - parentRect.top,
				width: imageRect.width,
				height: imageRect.height,
				naturalWidth: this.imageElement.naturalWidth,
				naturalHeight: this.imageElement.naturalHeight
			};
			if (
				this.imageBox.left != imageBox.left ||
				this.imageBox.top != imageBox.top ||
				this.imageBox.width != imageBox.width ||
				this.imageBox.height != imageBox.height ||
				this.imageBox.naturalWidth != imageBox.naturalWidth ||
				this.imageBox.naturalHeight != imageBox.naturalHeight
			) {
				this.imageBox = imageBox;
			}
		},
		overlayStyle: function() {
			return {
				left: this.imageBox.left + "px",
				top: this.imageBox.top + "px",
				width: this.imageBox.width + "px",
				height: this.imageBox.height + "px"
			};
		},
		viewBox: function() {
			return "0 0 " + this.imageBox.naturalWidth + " " + this.imageBox.naturalHeight;
		},
		visible: function() {
			return this.cameraSelected && this.estimatorIs("BAHTINOV") && this.spikesProperty != null && this.imageBox.width > 0 && this.imageBox.height > 0 && this.imageBox.naturalWidth > 0 && this.imageBox.naturalHeight > 0 && this.spikes().length > 0;
		},
		addPoint: function(points, x, y) {
			var width = this.imageBox.naturalWidth;
			var height = this.imageBox.naturalHeight;
			var epsilon = 0.001;
			if (x < -epsilon || x > width + epsilon || y < -epsilon || y > height + epsilon)
				return;
			x = Math.max(0, Math.min(width, x));
			y = Math.max(0, Math.min(height, y));
			for (var i in points) {
				if (Math.abs(points[i].x - x) < epsilon && Math.abs(points[i].y - y) < epsilon)
					return;
			}
			points.push({
				x: x,
				y: y
			});
		},
		lineSegment: function(rho, theta) {
			var width = this.imageBox.naturalWidth;
			var height = this.imageBox.naturalHeight;
			var cos = Math.cos(theta);
			var sin = Math.sin(theta);
			var epsilon = 0.000001;
			var points = [];
			if (Math.abs(sin) > epsilon) {
				this.addPoint(points, 0, rho / sin);
				this.addPoint(points, width, (rho - width * cos) / sin);
			}
			if (Math.abs(cos) > epsilon) {
				this.addPoint(points, rho / cos, 0);
				this.addPoint(points, (rho - height * sin) / cos, height);
			}
			if (points.length < 2)
				return null;
			var best = null;
			for (var i = 0; i < points.length - 1; i++) {
				for (var j = i + 1; j < points.length; j++) {
					var dx = points[i].x - points[j].x;
					var dy = points[i].y - points[j].y;
					var distance = dx * dx + dy * dy;
					if (best == null || distance > best.distance) {
						best = {
							x1: points[i].x,
							y1: points[i].y,
							x2: points[j].x,
							y2: points[j].y,
							distance: distance
						};
					}
				}
			}
			return best;
		},
		spikes: function() {
			var spikes = [];
			for (var i = 1; i <= 3; i++) {
				var rho = this.itemValue(this.spikesProperty, "RHO_" + i);
				var theta = this.itemValue(this.spikesProperty, "THETA_" + i);
				if (!isFinite(rho) || !isFinite(theta) || (rho == 0 && theta == 0))
					continue;
				var segment = this.lineSegment(rho, theta);
				if (segment != null)
					spikes.push(segment);
			}
			return spikes;
		}
	},
	template: `
		<div class="indigo-bahtinov-spikes-overlay" :style="overlayStyle()">
			<svg v-if="visible()" :viewBox="viewBox()" preserveAspectRatio="none">
				<line v-for="(spike, index) in spikes()" :key="'bahtinov-spike-' + index" class="indigo-bahtinov-spike" :x1="spike.x1" :y1="spike.y1" :x2="spike.x2" :y2="spike.y2"></line>
			</svg>
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
		formatValue: function(value) {
			if (typeof value != "number")
				return value;
			return Number(value.toPrecision(4)).toString();
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
				if (item.name == this.name) return this.formatValue(item.value);
			}
			return null;
		}
	},
	template: `
		<div v-if="property != null" class="p-1" :class="(cls != null ? cls : 'w-25')" data-bs-toggle="tooltip" :title="tooltip">
			<div class="indigo-chip indigo-readonly-chip w-100 d-flex align-items-center">
				<span v-if="icon.startsWith('glyphicons-')" class="indigo-chip-icon glyphicons" :class="icon"></span>
				<span v-else class="indigo-chip-icon">{{icon}}</span>
				<span class="indigo-chip-value">{{value()}}</span>
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
			<div class="indigo-chip indigo-readonly-chip w-100 d-flex align-items-center">
				<span v-if="icon.startsWith('glyphicons-')" class="indigo-chip-icon glyphicons" :class="icon"></span>
				<span v-else class="indigo-chip-icon">{{icon}}</span>
				<span class="indigo-chip-value">{{value()}}</span>
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
			<div class="indigo-chip indigo-readonly-chip w-100 d-flex align-items-center">
				<span v-if="icon != null && icon.startsWith('glyphicons-')" class="indigo-chip-icon glyphicons" :class="icon"></span>
				<span v-else-if="icon != null" class="indigo-chip-icon">{{icon}}</span>
				<span v-else class="indigo-chip-icon"></span>
				<span class="indigo-chip-value">{{value()}}</span>
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
												<div class="float-end d-flex gap-1 mt-1">
													<button type="submit" class="btn btn-sm btn-primary" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-outline-secondary" @click.prevent="reset(property)">Reset</button>
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
												<div class="float-end d-flex gap-1 mt-1">
													<button type="submit" class="btn btn-sm btn-primary" @click.prevent="set(property)">Submit</button>
													<button class="btn btn-sm btn-outline-secondary" @click.prevent="reset(property)">Reset</button>
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

var indigoStarCatalog = [];
var indigoDsoCatalog = [];
var indigoCatalogsLoaded = false;
var indigoCatalogPromise = null;

function indigoCatalogDeg2h(ra) {
	return ra < 0 ? ra / 15 + 24 : ra / 15;
}

function indigoCatalogString(value) {
	return value == null ? "" : String(value);
}

function indigoCatalogNormalize(value) {
	return indigoCatalogString(value).replace(/\s/g, "").toUpperCase();
}

function indigoCatalogCoordinates(feature) {
	if (feature == null || feature.geometry == null || feature.geometry.type != "Point")
		return null;
	if (feature.geometry.coordinates == null || feature.geometry.coordinates.length < 2)
		return null;
	return feature.geometry.coordinates;
}

function indigoCatalogLoad(url, mapper) {
	return fetch(url)
		.then(function(response) {
			if (!response.ok)
				throw new Error(url + " returned " + response.status);
			return response.json();
		})
		.then(function(geojson) {
			var result = [];
			var features = geojson.features || [];
			for (var i = 0; i < features.length; i++) {
				var object = mapper(features[i]);
				if (object != null)
					result.push(object);
			}
			return result;
		})
		.catch(function(error) {
			console.log("Failed to load " + url + ": " + error);
			return [];
		});
}

function indigoLoadCelestialCatalogs() {
	if (indigoCatalogPromise != null)
		return indigoCatalogPromise;
	indigoCatalogPromise = Promise.all([
		indigoCatalogLoad("/data/stars.json", function(feature) {
			var coordinates = indigoCatalogCoordinates(feature);
			if (coordinates == null)
				return null;
			var properties = feature.properties || {};
			var id = feature.id;
			var name = indigoCatalogString(properties.name);
			var desig = indigoCatalogString(properties.desig);
			var label = name;
			if (desig != "") {
				if (label != "")
					label += ", ";
				label += desig;
			}
			if (Number(id) > 0) {
				if (label != "")
					label += ", ";
				label += "HIP" + id;
			}
			if (label == "")
				label = "HIP" + id;
			return {
				name: label,
				ra: indigoCatalogDeg2h(coordinates[0]),
				raDeg: coordinates[0],
				dec: coordinates[1],
				search: [
					id,
					"HIP" + id,
					name,
					desig
				].map(indigoCatalogNormalize).join(" ")
			};
		}),
		indigoCatalogLoad("/data/dsos.json", function(feature) {
			var coordinates = indigoCatalogCoordinates(feature);
			if (coordinates == null)
				return null;
			var properties = feature.properties || {};
			var id = indigoCatalogString(feature.id);
			var name = indigoCatalogString(properties.name);
			var desig = indigoCatalogString(properties.desig);
			var label = name;
			if (label != "" && desig != "")
				label += ", ";
			label += desig;
			if (label == "")
				label = id;
			return {
				name: label,
				ra: indigoCatalogDeg2h(coordinates[0]),
				raDeg: coordinates[0],
				dec: coordinates[1],
				search: [
					id,
					name,
					desig
				].map(indigoCatalogNormalize).join(" ")
			};
		})
	]).then(function(catalogs) {
		indigoStarCatalog = catalogs[0];
		indigoDsoCatalog = catalogs[1];
		indigoCatalogsLoaded = true;
	});
	return indigoCatalogPromise;
}

app.component('indigo-query-db', {
	props: {
		dark: {
			type: Boolean,
			default: false
		}
	},
	data() {
		return {
			query: "",
			loading: false,
			result: []
		};
	},
	mounted: function() {
		this.loadCatalogs();
	},
	methods: {
		loadCatalogs: function() {
			if (indigoCatalogsLoaded)
				return;
			var self = this;
			this.loading = true;
			indigoLoadCelestialCatalogs().then(function() {
				self.loading = false;
				self.search();
			});
		},
		setTarget: function(object) {
			if (window.selectObject != null)
				window.selectObject(object);
		},
		onChange: function(e) {
			this.query = e.target.value;
			this.search();
		},
		search: function() {
			var pattern = indigoCatalogNormalize(this.query);
			this.result = [];
			if (pattern == "")
				return;
			if (!indigoCatalogsLoaded) {
				this.loadCatalogs();
				return;
			}
			for (var i = 0; i < indigoStarCatalog.length; i++) {
				var star = indigoStarCatalog[i];
				if (star.search.indexOf(pattern) >= 0)
					this.result.push(star);
			}
			for (var j = 0; j < indigoDsoCatalog.length; j++) {
				var dso = indigoDsoCatalog[j];
				if (dso.search.indexOf(pattern) >= 0)
					this.result.push(dso);
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


app.component('indigo-navbar', {
	props: {
		active: String,
		title: String,
		icon: String,
		expand: {
			type: String,
			default: "sm"
		}
	},
	data: function() {
		return {
			appLinks: [
				{ id: "imager", href: "imager.html", icon: "imager.png", title: "Imager" },
				{ id: "mount", href: "mount.html", icon: "mount.png", title: "Mount" },
				{ id: "guider", href: "guider.html", icon: "guider.png", title: "Guider" },
				{ id: "script", href: "script.html", icon: "script.png", title: "Script" }
			]
		};
	},
	computed: {
		navbarClass: function() {
			return "navbar navbar-expand-" + this.expand + " navbar-light";
		},
		features: function() {
			if (this.$root == null || this.$root.devices == null)
				return null;
			var properties = this.$root.devices["Server"];
			if (properties == null)
				return null;
			return properties["FEATURES"];
		},
		webApps: function() {
			if (this.features == null)
				return false;
			var item = this.features.item("WEB_APPS");
			return item != null && item.value;
		},
		activePage: function() {
			var pages = [
				{ id: "mng", icon: "mng.png", title: "Server Manager" },
				{ id: "ctrl", icon: "ctrl.png", title: "Control Panel" }
			].concat(this.appLinks);
			for (var i = 0; i < pages.length; i++) {
				if (pages[i].id == this.active)
					return pages[i];
			}
			return null;
		},
		brandIcon: function() {
			if (this.icon != null && this.icon != "")
				return this.icon;
			if (this.activePage != null)
				return this.activePage.icon;
			return "";
		},
		brandTitle: function() {
			if (this.title != null && this.title != "")
				return this.title;
			if (this.activePage != null)
				return this.activePage.title;
			return "";
		}
	},
	template: `
		<nav :class="navbarClass">
			<a class="navbar-brand text-white" href="#">
				<img :src="brandIcon" width="40" height="40" class="d-inline-block align-middle" alt=""/>
				<h4 class="title">{{brandTitle}}</h4>
			</a>
			<button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarContent">
				<span class="navbar-toggler-icon"></span>
			</button>
			<div id="navbarContent" class="collapse navbar-collapse m-0">
				<template v-if="features != null">
					<a class="nav-link ms-auto" href="mng.html" data-bs-toggle="tooltip" title="Server Manager">
						<img src="mng.png" width="40" height="40" class="align-middle" alt=""/>
					</a>
					<a class="nav-link" href="ctrl.html" data-bs-toggle="tooltip" title="Control Panel">
						<img src="ctrl.png" width="40" height="40" class="align-middle" alt=""/>
					</a>
					<template v-if="webApps">
						<a v-for="link in appLinks" :key="link.id" class="nav-link" :href="link.href" data-bs-toggle="tooltip" :title="link.title">
							<img :src="link.icon" width="40" height="40" class="d-inline-block align-middle" alt=""/>
						</a>
					</template>
				</template>
			</div>
		</nav>
		`
});


app.component('indigo-status-bar', {
	props: {
		columnsToggle: {
			type: Boolean,
			default: false
		}
	},
	computed: {
		serverInfo: function() {
			if (this.$root == null || this.$root.findProperty == null)
				return null;
			return this.$root.findProperty("Server", "INFO");
		},
		serverVersion: function() {
			if (this.serverInfo == null)
				return "";
			var item = this.serverInfo.item("VERSION");
			return item == null ? "" : item.value;
		},
		serverService: function() {
			if (this.serverInfo == null)
				return "";
			var item = this.serverInfo.item("SERVICE");
			return item == null ? "" : item.value;
		}
	},
	methods: {
		setDark: function() {
			setDarkMode();
		},
		setLight: function() {
			setLightMode();
		},
		setColumns: function(columns) {
			var fromClass = columns == 1 ? "col-md-4" : "col-md-12";
			var toClass = columns == 1 ? "col-md-12" : "col-md-4";
			document.querySelectorAll("div." + fromClass).forEach(function(element) {
				element.classList.remove(fromClass);
				element.classList.add(toClass);
			});
			this.$root.columns = columns;
		}
	},
	template: `
		<div v-show="$root.connected" class="alert alert-success alert-dismissible fade show m-1 mt-2" role="alert">
			{{ $root.state }}
			<span v-if="serverInfo != null" class="float-end">
				INDIGO Server {{serverVersion}} at {{serverService}}
			</span>
		</div>
		<div v-show="$root.failed" class="alert alert-danger alert-dismissible fade show m-1 mt-2" role="alert">
			{{ $root.state }}
		</div>
		<div v-show="$root.message" class="alert alert-warning alert-dismissible fade show m-1 mt-2" role="alert">
			{{ $root.state }}
		</div>
		<div class="alert alert-info show m-1 mt-2" role="alert">
			Copyright &copy; 2019-2025, The INDIGO Initiative. All rights reserved.
			<a v-if="$root.dark" href="#" class="float-end" @click.prevent="setLight">Switch to light appearance</a>
			<a v-else href="#" class="float-end" @click.prevent="setDark">Switch to dark appearance</a>
			<a v-if="columnsToggle && $root.columns == 3" href="#" class="float-end indigo-status-bar-action" @click.prevent="setColumns(1)">Switch to 1 column</a>
			<a v-else-if="columnsToggle" href="#" class="float-end indigo-status-bar-action" @click.prevent="setColumns(3)">Switch to 3 columns</a>
		</div>
		`
});


app.component('indigo-status-button', {
	props: {
		property: Object,
		activeItem: String,
		activeState: {
			type: String,
			default: "Ok"
		},
		busyItem: String,
		busyStatusClass: {
			type: String,
			default: "busy-state"
		},
		statusClass: String,
		action: Function,
		activeAction: Function,
		inactiveAction: Function,
		busyAction: Function,
		alertAction: Function,
		disabled: Boolean
	},
	computed: {
		stateClass: function() {
			if (this.statusClass != null && this.statusClass != "")
				return this.statusClass;
			if (this.property == null)
				return "idle-state";
			if (this.property.state == this.activeState && this.activeItemValue())
				return "ok-state";
			if (this.property.state == "Busy" && this.busyItemValue())
				return this.busyStatusClass;
			if (this.property.state == "Alert")
				return "alert-state";
			return "idle-state";
		}
	},
	methods: {
		itemValue: function(name) {
			if (this.property == null)
				return false;
			if (name == null || name == "")
				return true;
			var item = this.property.item(name);
			return item != null && item.value;
		},
		activeItemValue: function() {
			return this.itemValue(this.activeItem);
		},
		busyItemValue: function() {
			if (this.busyItem == null || this.busyItem == "")
				return this.property != null;
			return this.itemValue(this.busyItem);
		},
		runAction: function(event) {
			var action = this.action;
			if (this.property != null && this.property.state == "Alert" && this.alertAction != null) {
				action = this.alertAction;
			} else if (this.property != null && this.property.state == "Busy" && this.busyAction != null) {
				action = this.busyAction;
			} else if (this.activeItemValue()) {
				if (this.activeAction != null)
					action = this.activeAction;
			} else if (this.inactiveAction != null) {
				action = this.inactiveAction;
			}
			if (action != null)
				action(event);
		}
	},
	template: `
		<button class="btn btn-svg" :class="stateClass" :disabled="disabled" @click="runAction"><slot></slot></button>
		`
});


app.component('indigo-guider-graph', {
	data: function() {
		return {
			raDrift: [],
			decDrift: [],
			raCorr: [],
			decCorr: [],
			rmse: [],
			paintPending: false,
			resizeHandler: null
		};
	},
	watch: {
		raDrift: {
			handler: function() {
				this.schedulePaint();
			},
			deep: true
		},
		decDrift: {
			handler: function() {
				this.schedulePaint();
			},
			deep: true
		},
		raCorr: {
			handler: function() {
				this.schedulePaint();
			},
			deep: true
		},
		decCorr: {
			handler: function() {
				this.schedulePaint();
			},
			deep: true
		},
		rmse: {
			handler: function() {
				this.schedulePaint();
			},
			deep: true
		}
	},
	mounted: function() {
		this.resizeHandler = this.paintGraphs.bind(this);
		window.addEventListener("resize", this.resizeHandler);
		this.paintGraphs();
		guiSetup();
	},
	beforeUnmount: function() {
		if (this.resizeHandler != null)
			window.removeEventListener("resize", this.resizeHandler);
	},
	methods: {
		push: function(driftRa, driftDec, corrRa, corrDec, rmseValue) {
			this.trimData();
			this.raDrift.push(driftRa);
			this.decDrift.push(driftDec);
			this.raCorr.push(corrRa);
			this.decCorr.push(corrDec);
			this.rmse.push(rmseValue);
		},
		clear: function() {
			this.raDrift = [];
			this.decDrift = [];
			this.raCorr = [];
			this.decCorr = [];
			this.rmse = [];
		},
		trimData: function() {
			while (this.raDrift.length >= 200)
				this.raDrift.shift();
			while (this.decDrift.length >= 200)
				this.decDrift.shift();
			while (this.raCorr.length >= 200)
				this.raCorr.shift();
			while (this.decCorr.length >= 200)
				this.decCorr.shift();
			while (this.rmse.length >= 200)
				this.rmse.shift();
		},
		schedulePaint: function() {
			if (this.paintPending)
				return;
			this.paintPending = true;
			var self = this;
			this.$nextTick(function() {
				self.paintPending = false;
				self.paintGraphs();
			});
		},
		paintGraphs: function() {
			this.paintGraph(this.$refs.driftCanvas, this.raDrift, this.decDrift, this.rmse, false);
			this.paintGraph(this.$refs.corrCanvas, this.raCorr, this.decCorr, this.rmse, true);
		},
		paintGraph: function(canvas, ra, dec, rmse, pulse) {
			if (canvas == null)
				return;
			canvas.width = canvas.offsetWidth;
			canvas.height = canvas.offsetHeight;
			var width = canvas.width;
			var height = canvas.height;
			if (width == 0 || height == 0 || ra.length == 0 || dec.length == 0)
				return;
			var height2 = height / 2;
			var ctx = canvas.getContext("2d");
			var maxValue = 0;
			for (var i in ra) {
				var value = Math.abs(ra[i]);
				if (maxValue < value)
					maxValue = value;
			}
			for (var j in dec) {
				var decValue = Math.abs(dec[j]);
				if (maxValue < decValue)
					maxValue = decValue;
			}
			if (maxValue == 0) {
				maxValue = 0.1;
			} else if (maxValue < 0.5) {
				maxValue = Math.ceil(maxValue * 10) / 10;
			} else if (maxValue < 1) {
				maxValue = 1;
			} else if (maxValue > 10) {
				maxValue = 10;
			} else {
				maxValue = Math.ceil(maxValue);
			}
			var yScale = (height2 - 5.0) / maxValue;
			var xScale = width / 200;
			ctx.save();
			ctx.strokeStyle = "#AAA";
			var path = new Path2D();
			path.moveTo(0, height2);
			path.lineTo(width, height2);
			ctx.stroke(path);
			path = new Path2D();
			if (maxValue >= 1) {
				for (var r = 1; r <= maxValue; r++) {
					var rr = r * yScale;
					path.moveTo(0, height2 - rr);
					path.lineTo(width, height2 - rr);
					path.moveTo(0, height2 + rr);
					path.lineTo(width, height2 + rr);
				}
			} else {
				ctx.setLineDash([1.0, 5.0]);
				for (var rrIndex = 1; rrIndex <= 10 * maxValue; rrIndex++) {
					var subpixel = rrIndex * yScale / 10;
					path.moveTo(0, height2 - subpixel);
					path.lineTo(width, height2 - subpixel);
					path.moveTo(0, height2 + subpixel);
					path.lineTo(width, height2 + subpixel);
				}
			}
			ctx.stroke(path);
			ctx.restore();

			ctx.save();
			ctx.fillStyle = "rgba(100, 100, 100, 0.3)";
			path = new Path2D();
			var rmseX = 0;
			for (var k in rmse) {
				if (rmse[k])
					path.rect(rmseX, 0, xScale, height);
				rmseX += xScale;
			}
			ctx.fill(path);
			ctx.restore();

			ctx.save();
			ctx.strokeStyle = "#00F";
			path = new Path2D();
			var x = 0;
			var y = height2 + ra[0] * yScale;
			path.moveTo(x, y);
			for (var m in ra) {
				y = height2 + ra[m] * yScale;
				path.lineTo(x, y);
				x += xScale;
				if (pulse)
					path.lineTo(x, y);
			}
			ctx.stroke(path);
			ctx.restore();

			ctx.save();
			ctx.strokeStyle = "#F00";
			path = new Path2D();
			x = 0;
			y = height2 + dec[0] * yScale;
			path.moveTo(x, y);
			for (var n in dec) {
				y = height2 + dec[n] * yScale;
				path.lineTo(x, y);
				x += xScale;
				if (pulse)
					path.lineTo(x, y);
			}
			ctx.stroke(path);
			ctx.restore();
		}
	},
	template: `
		<div>
			<div class="card p-1 m-1 bg-light">
				<div class="card-body d-flex">
					<canvas id="graph_drift" ref="driftCanvas" class="card p-0 m-0 bg-light" data-bs-toggle="tooltip" title="Drift"></canvas>
				</div>
			</div>
			<div class="card p-1 m-1 mt-2 bg-light">
				<div class="card-body d-flex">
					<canvas id="graph_corr" ref="corrCanvas" class="card p-0 m-0 bg-light" data-bs-toggle="tooltip" title="Corrections"></canvas>
				</div>
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
			<div class="d-flex w-100 mt-1 p-1 indigo-command-row">
				<button type="submit" class="btn btn-sm btn-primary ms-auto" @click.prevent="set()">Submit</button>
				<button class="btn btn-sm btn-outline-secondary" @click.prevent="reset()">Reset</button>
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
		resize: function() {
			var map = this.$refs.map;
			if (map == null)
				return;
			map.style.maxHeight = "";
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
			if (!indigoCatalogsLoaded) {
				var self = this;
				indigoLoadCelestialCatalogs().then(function() {
					self.selectNearestObject(coordinates);
				});
				return;
			}
			this.selectNearestObject(coordinates);
		},
		selectNearestObject: function(coordinates) {
			var bestX = 0;
			var bestY = 0;
			var dist = Math.pow(coordinates[0] - bestX, 2) + Math.pow(coordinates[1] - bestY, 2);
			var catalogs = [ indigoStarCatalog, indigoDsoCatalog ];
			for (var i = 0; i < catalogs.length; i++) {
				for (var j = 0; j < catalogs[i].length; j++) {
					var object = catalogs[i][j];
					var d = Math.pow(coordinates[0] - object.raDeg, 2) + Math.pow(coordinates[1] - object.dec, 2);
					if (d < dist) {
						dist = d;
						bestX = object.raDeg;
						bestY = object.dec;
					}
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
			<div id="map" ref="map" class="position-relative indigo-sky-map"></div>
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
