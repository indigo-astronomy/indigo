// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** libcamera implementation of the bridge used by indigo_ccd_libcamera
 \file indigo_ccd_libcamera_bridge.cpp

 Built against libcamera when Makefile.inc finds it through pkg-config
 (HAVE_LIBCAMERA), otherwise as a stub that reports no cameras, so the driver
 still builds on Linux hosts without libcamera development files.
 */

#include <stdio.h>
#include <string.h>

#include "indigo_ccd_libcamera_bridge.h"

#ifdef HAVE_LIBCAMERA

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/dma-buf.h>

#include <libcamera/libcamera.h>
#include <libcamera/logging.h>
#include <libcamera/version.h>

using namespace libcamera;

// A frame whose metadata does not report the requested exposure was started before the new controls
// took effect, and the first frames after start may be flagged as errors. Such frames are dropped,
// but only this many of them, so a sensor that quantizes or clamps the exposure differently than its
// ControlInfo says still delivers a frame.
#define LCAM_MAX_DISCARDED_FRAMES	16

// Exposures longer than this start the sensor with the shortest exposure, see lcam_start().
#define LCAM_SHORT_START_THRESHOLD	0.2

typedef struct {
	PixelFormat format;
	unsigned bit_depth;
	unsigned container;				// 8 or 16 bit samples, or 10/12 for MIPI CSI-2 packed data
} lcam_format;

struct lcam_camera {
	std::shared_ptr<Camera> camera;
	lcam_properties properties;
	std::vector<lcam_format> mode_formats;
	std::vector<Size> mode_sizes;
	std::unique_ptr<CameraConfiguration> configuration;
	std::unique_ptr<FrameBufferAllocator> allocator;
	std::vector<std::unique_ptr<Request>> requests;
	std::map<const FrameBuffer *, std::pair<void *, size_t>> mappings;
	Stream *stream = nullptr;
	lcam_format format = {};
	unsigned stride = 0;
	bool running = false, streaming = false;
	int32_t exposure_us = 0, start_exposure_us = 0;
	int discarded = 0;
	std::mutex mutex;
	std::deque<Request *> completed;
	std::vector<uint8_t> row;

	void request_completed(Request *request) {
		std::lock_guard<std::mutex> lock(mutex);
		completed.push_back(request);
	}
};

static std::unique_ptr<CameraManager> manager;
static std::mutex manager_mutex;

static void copy_error(char *error, size_t error_size, const char *message) {
	if (error && error_size) {
		snprintf(error, error_size, "%s", message);
	}
}

// Raw formats are named S<bayer><depth>[_CSI2P] for colour and R<depth>[_CSI2P] for mono sensors.
// Unpacked formats store one sample per 16-bit word, aligned to the least significant bit; the
// 16-bit formats of the Raspberry Pi 5 front end are passed on as 16-bit data. Compressed formats (PISP_COMP) cannot be decoded here and are skipped.
static bool parse_format(const PixelFormat &format, lcam_format *result, char *bayer) {
	std::string name = format.toString();
	const char *s = name.c_str();
	char pattern[8] = "";
	if (s[0] == 'S' && strlen(s) > 5 && strchr("RGB", s[1]) && strchr("RGB", s[2]) && strchr("RGB", s[3]) && strchr("RGB", s[4])) {
		memcpy(pattern, s + 1, 4);
		pattern[4] = 0;
		s += 5;
	} else if (s[0] == 'R' && s[1] >= '0' && s[1] <= '9') {
		s += 1;
	} else {
		return false;
	}
	char *end = nullptr;
	long depth = strtol(s, &end, 10);
	if (end == s || (depth != 8 && depth != 10 && depth != 12 && depth != 14 && depth != 16)) {
		return false;
	}
	result->format = format;
	result->bit_depth = (unsigned)depth;
	if (*end == 0) {
		result->container = depth == 8 ? 8 : 16;
	} else if (!strcmp(end, "_CSI2P") && (depth == 10 || depth == 12)) {
		result->container = (unsigned)depth;
	} else if (!strcmp(end, "_CSI2P") && depth == 8) {
		result->container = 8;
	} else {
		return false;
	}
	if (bayer) {
		strcpy(bayer, pattern);
	}
	return true;
}

// Prefer the format with the most significant bits; at equal depth prefer unpacked samples, which
// need no conversion.
static bool better_format(const lcam_format &a, const lcam_format &b) {
	if (a.bit_depth != b.bit_depth) {
		return a.bit_depth > b.bit_depth;
	}
	return a.container == 16 && b.container != 16;
}

static void unmap_buffers(lcam_camera *camera) {
	for (auto &mapping : camera->mappings) {
		munmap(mapping.second.first, mapping.second.second);
	}
	camera->mappings.clear();
}

static void release_capture(lcam_camera *camera) {
	if (camera->running) {
		camera->camera->stop();
		camera->running = false;
	}
	camera->camera->requestCompleted.disconnect(camera);
	{
		std::lock_guard<std::mutex> lock(camera->mutex);
		camera->completed.clear();
	}
	camera->requests.clear();
	unmap_buffers(camera);
	if (camera->allocator && camera->stream) {
		camera->allocator->free(camera->stream);
	}
	camera->allocator.reset();
	camera->stream = nullptr;
}

static bool configure(lcam_camera *camera, int mode, char *error, size_t error_size) {
	// A raw-only configuration is enough for the Raspberry Pi pipeline handlers; pipelines that insist
	// on a processed stream get a small viewfinder stream next to the raw one, which is never read.
	const std::vector<std::vector<StreamRole>> role_sets = { { StreamRole::Raw }, { StreamRole::Raw, StreamRole::Viewfinder } };
	for (const auto &roles : role_sets) {
		std::unique_ptr<CameraConfiguration> configuration = camera->camera->generateConfiguration(roles);
		if (!configuration) {
			continue;
		}
		StreamConfiguration &raw = configuration->at(0);
		raw.pixelFormat = camera->mode_formats[mode].format;
		raw.size = camera->mode_sizes[mode];
		raw.bufferCount = 2;
		if (roles.size() > 1) {
			configuration->at(1).size = Size(320, 240);
		}
		CameraConfiguration::Status status = configuration->validate();
		if (status == CameraConfiguration::Invalid || raw.pixelFormat != camera->mode_formats[mode].format || raw.size != camera->mode_sizes[mode]) {
			continue;
		}
		if (camera->camera->configure(configuration.get()) < 0) {
			continue;
		}
		camera->configuration = std::move(configuration);
		camera->stream = camera->configuration->at(0).stream();
		camera->stride = camera->configuration->at(0).stride;
		camera->format = camera->mode_formats[mode];
		return true;
	}
	copy_error(error, error_size, "Raw stream configuration rejected");
	return false;
}

static bool allocate(lcam_camera *camera, char *error, size_t error_size) {
	camera->allocator = std::make_unique<FrameBufferAllocator>(camera->camera);
	if (camera->allocator->allocate(camera->stream) <= 0) {
		copy_error(error, error_size, "Frame buffer allocation failed");
		return false;
	}
	for (const std::unique_ptr<FrameBuffer> &buffer : camera->allocator->buffers(camera->stream)) {
		const FrameBuffer::Plane &plane = buffer->planes()[0];
		size_t length = plane.offset + plane.length;
		void *memory = mmap(nullptr, length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
		if (memory == MAP_FAILED) {
			copy_error(error, error_size, "Frame buffer mapping failed");
			return false;
		}
		camera->mappings[buffer.get()] = std::make_pair(memory, length);
		std::unique_ptr<Request> request = camera->camera->createRequest();
		if (!request || request->addBuffer(camera->stream, buffer.get()) < 0) {
			copy_error(error, error_size, "Request creation failed");
			return false;
		}
		camera->requests.push_back(std::move(request));
	}
	return !camera->requests.empty();
}

static inline unsigned sample(const lcam_camera *camera, const uint8_t *row, unsigned x) {
	switch (camera->format.container) {
		case 8:
			return row[x];
		case 10: {
			const uint8_t *group = row + (x / 4) * 5;
			unsigned index = x % 4;
			return (group[index] << 2) | ((group[4] >> (index * 2)) & 0x3);
		}
		case 12: {
			const uint8_t *group = row + (x / 2) * 3;
			unsigned index = x % 2;
			return (group[index] << 4) | ((group[2] >> (index * 4)) & 0xF);
		}
		default:
			return row[2 * x] | (row[2 * x + 1] << 8);
	}
}

extern "C" bool lcam_initialize(void) {
	std::lock_guard<std::mutex> lock(manager_mutex);
	if (manager) {
		return true;
	}
	logSetLevel("*", "ERROR");
	std::unique_ptr<CameraManager> candidate = std::make_unique<CameraManager>();
	if (candidate->start() < 0) {
		return false;
	}
	manager = std::move(candidate);
	return true;
}

extern "C" void lcam_terminate(void) {
	std::lock_guard<std::mutex> lock(manager_mutex);
	if (manager) {
		manager->stop();
		manager.reset();
	}
}

extern "C" int lcam_list(lcam_camera_info *list, int size) {
	std::lock_guard<std::mutex> lock(manager_mutex);
	if (!manager) {
		return 0;
	}
	int count = 0;
	for (const std::shared_ptr<Camera> &camera : manager->cameras()) {
		if (count >= size) {
			break;
		}
		lcam_camera_info *info = list + count++;
		snprintf(info->id, sizeof(info->id), "%s", camera->id().c_str());
		const auto model = camera->properties().get(properties::Model);
		snprintf(info->model, sizeof(info->model), "%s", model ? std::string(*model).c_str() : "");
	}
	return count;
}

extern "C" lcam_camera *lcam_open(const char *id, lcam_properties *properties, char *error, size_t error_size) {
	std::shared_ptr<Camera> device;
	{
		std::lock_guard<std::mutex> lock(manager_mutex);
		if (manager) {
			device = manager->get(id);
		}
	}
	if (!device) {
		copy_error(error, error_size, "Camera not found");
		return nullptr;
	}
	if (device->acquire() < 0) {
		copy_error(error, error_size, "Camera is used by another application");
		return nullptr;
	}
	lcam_camera *camera = new lcam_camera();
	camera->camera = device;
	lcam_properties *result = &camera->properties;
	const ControlList &list = device->properties();
	// PixelArraySize was replaced by PixelArrayArea in libcamera 0.7; the active areas exist in every version.
	if (const auto areas = list.get(properties::PixelArrayActiveAreas); areas && !areas->empty()) {
		result->pixel_array_width = (*areas)[0].width;
		result->pixel_array_height = (*areas)[0].height;
	}
	if (const auto cell = list.get(properties::UnitCellSize)) {
		result->pixel_width = cell->width / 1000.0;
		result->pixel_height = cell->height / 1000.0;
	}
	const ControlInfoMap &controls = device->controls();
	auto exposure = controls.find(&controls::ExposureTime);
	auto gain = controls.find(&controls::AnalogueGain);
	if (exposure != controls.end()) {
		result->exposure_min = exposure->second.min().get<int32_t>() / 1e6;
		result->exposure_max = exposure->second.max().get<int32_t>() / 1e6;
	}
	if (gain != controls.end()) {
		result->gain_min = gain->second.min().get<float>();
		result->gain_max = gain->second.max().get<float>();
		result->gain_default = gain->second.def().isNone() ? result->gain_min : gain->second.def().get<float>();
	}
	for (int i = 0; i < LCAM_MAX_MODES; i++) {
		result->modes[i].exposure_min = result->exposure_min;
		result->modes[i].exposure_max = result->exposure_max;
	}
	std::unique_ptr<CameraConfiguration> configuration = device->generateConfiguration({ StreamRole::Raw });
	if (configuration && !configuration->empty()) {
		const StreamFormats &formats = configuration->at(0).formats();
		std::vector<std::pair<Size, lcam_format>> modes;
		for (const PixelFormat &format : formats.pixelformats()) {
			lcam_format parsed;
			char bayer[8];
			if (!parse_format(format, &parsed, bayer)) {
				continue;
			}
			strcpy(result->bayer, bayer);
			for (const Size &size : formats.sizes(format)) {
				auto existing = std::find_if(modes.begin(), modes.end(), [&](const std::pair<Size, lcam_format> &mode) { return mode.first == size; });
				if (existing == modes.end()) {
					modes.emplace_back(size, parsed);
				} else if (better_format(parsed, existing->second)) {
					existing->second = parsed;
				}
			}
		}
		std::sort(modes.begin(), modes.end(), [](const std::pair<Size, lcam_format> &a, const std::pair<Size, lcam_format> &b) { return a.first.width * a.first.height > b.first.width * b.first.height; });
		for (const auto &mode : modes) {
			if (result->mode_count >= LCAM_MAX_MODES) {
				break;
			}
			lcam_mode *item = result->modes + result->mode_count++;
			item->width = mode.first.width;
			item->height = mode.first.height;
			item->bit_depth = mode.second.bit_depth;
			unsigned binning = result->pixel_array_width && item->width ? (unsigned)lround((double)result->pixel_array_width / item->width) : 1;
			item->binning = binning >= 2 && result->pixel_array_height && item->height * binning <= result->pixel_array_height + binning ? binning : 1;
			camera->mode_sizes.push_back(mode.first);
			camera->mode_formats.push_back(mode.second);
		}
	}
	// The control limits a camera reports before configure() belong to no particular mode (the
	// Raspberry Pi pipeline reports 1 us .. 66 ms for an OV5647 that exposes up to 3 s at full
	// resolution and 4.6 s at 640x480), so every mode is configured once to read its own limits.
	for (int i = 0; i < result->mode_count; i++) {
		if (!configure(camera, i, nullptr, 0)) {
			continue;
		}
		const ControlInfoMap &configured = device->controls();
		auto mode_exposure = configured.find(&controls::ExposureTime);
		if (mode_exposure != configured.end()) {
			result->modes[i].exposure_min = mode_exposure->second.min().get<int32_t>() / 1e6;
			result->modes[i].exposure_max = mode_exposure->second.max().get<int32_t>() / 1e6;
		}
		auto mode_gain = configured.find(&controls::AnalogueGain);
		if (mode_gain != configured.end()) {
			result->gain_min = mode_gain->second.min().get<float>();
			result->gain_max = mode_gain->second.max().get<float>();
		}
	}
	camera->configuration.reset();
	camera->stream = nullptr;
	if (result->mode_count > 0 && result->modes[0].exposure_max > result->modes[0].exposure_min) {
		result->exposure_min = result->modes[0].exposure_min;
		result->exposure_max = result->modes[0].exposure_max;
	}
	if (result->mode_count == 0) {
		copy_error(error, error_size, "Camera has no supported RAW format");
		device->release();
		delete camera;
		return nullptr;
	}
	if (result->pixel_array_width == 0 || result->pixel_array_height == 0) {
		result->pixel_array_width = result->modes[0].width;
		result->pixel_array_height = result->modes[0].height;
	}
	*properties = *result;
	return camera;
}

extern "C" void lcam_close(lcam_camera *camera) {
	if (camera) {
		release_capture(camera);
		camera->configuration.reset();
		camera->camera->release();
		delete camera;
	}
}

static void set_exposure(const ControlInfoMap &info, ControlList &controls, int32_t exposure_us) {
	if (info.find(&controls::ExposureTime) != info.end()) {
		controls.set(controls::ExposureTime, exposure_us);
	}
	auto duration = info.find(&controls::FrameDurationLimits);
	if (duration != info.end()) {
		// The frame must be long enough for the exposure; let the pipeline choose the shortest such frame.
		int64_t maximum = duration->second.max().get<int64_t>();
		int64_t minimum = std::min<int64_t>(std::max<int64_t>(exposure_us, duration->second.min().get<int64_t>()), maximum);
		controls.set(controls::FrameDurationLimits, { minimum, maximum });
	}
}

extern "C" bool lcam_start(lcam_camera *camera, int mode, double exposure, double gain, bool streaming, char *error, size_t error_size) {
	release_capture(camera);
	if (mode < 0 || mode >= camera->properties.mode_count) {
		copy_error(error, error_size, "Invalid mode");
		return false;
	}
	if (!configure(camera, mode, error, error_size) || !allocate(camera, error, error_size)) {
		release_capture(camera);
		return false;
	}
	const ControlInfoMap &info = camera->camera->controls();
	ControlList controls(controls::controls);
	camera->exposure_us = (int32_t)std::lround(exposure * 1e6);
#if LIBCAMERA_VERSION_MAJOR > 0 || LIBCAMERA_VERSION_MINOR >= 5
	// libcamera 0.5 replaced AeEnable with separate manual modes for exposure and gain.
	if (info.find(&controls::ExposureTimeMode) != info.end()) {
		controls.set(controls::ExposureTimeMode, controls::ExposureTimeModeManual);
	}
	if (info.find(&controls::AnalogueGainMode) != info.end()) {
		controls.set(controls::AnalogueGainMode, controls::AnalogueGainModeManual);
	}
#endif
	if (info.find(&controls::AeEnable) != info.end()) {
		controls.set(controls::AeEnable, false);
	}
	if (info.find(&controls::AnalogueGain) != info.end()) {
		controls.set(controls::AnalogueGain, (float)gain);
	}
	// The first frames after start are unusable (an OV5647 hides two of them) and each lasts as long as
	// the exposure, so a long exposure starts the sensor with the shortest exposure of the mode and asks
	// for the requested one with the first request. Frames still carrying the short exposure are dropped
	// by lcam_read(), so the delivered frame is always exposed entirely after the request.
	ControlList request_controls(controls::controls);
	int32_t start_exposure_us = camera->exposure_us;
	double start_minimum = camera->properties.modes[mode].exposure_min;
	if (exposure > LCAM_SHORT_START_THRESHOLD && start_minimum > 0) {
		start_exposure_us = (int32_t)std::lround(start_minimum * 1e6);
	}
	camera->start_exposure_us = start_exposure_us;
	set_exposure(info, controls, start_exposure_us);
	if (start_exposure_us != camera->exposure_us) {
		set_exposure(info, request_controls, camera->exposure_us);
	}
	camera->streaming = streaming;
	camera->discarded = 0;
	camera->camera->requestCompleted.connect(camera, &lcam_camera::request_completed);
	if (camera->camera->start(&controls) < 0) {
		copy_error(error, error_size, "Camera start failed");
		release_capture(camera);
		return false;
	}
	camera->running = true;
	size_t count = streaming ? camera->requests.size() : 1;
	if (!request_controls.empty()) {
		camera->requests[0]->controls().merge(request_controls);
	}
	for (size_t i = 0; i < count; i++) {
		if (camera->camera->queueRequest(camera->requests[i].get()) < 0) {
			copy_error(error, error_size, "Request queueing failed");
			release_capture(camera);
			return false;
		}
	}
	return true;
}

extern "C" int lcam_read(lcam_camera *camera, uint16_t *buffer, size_t buffer_size, unsigned left, unsigned top, unsigned width, unsigned height, lcam_frame *frame, char *error, size_t error_size) {
	if (!camera->running) {
		copy_error(error, error_size, "Capture is not running");
		return -1;
	}
	Request *request = nullptr;
	{
		std::lock_guard<std::mutex> lock(camera->mutex);
		if (camera->completed.empty()) {
			return 0;
		}
		request = camera->completed.front();
		camera->completed.pop_front();
	}
	if (request->status() != Request::RequestComplete) {
		copy_error(error, error_size, "Request was cancelled");
		return -1;
	}
	const StreamConfiguration &configuration = camera->configuration->at(0);
	FrameBuffer *frame_buffer = request->findBuffer(camera->stream);
	auto mapping = camera->mappings.find(frame_buffer);
	if (!frame_buffer || mapping == camera->mappings.end()) {
		copy_error(error, error_size, "Frame buffer not found");
		return -1;
	}
	const ControlList &metadata = request->metadata();
	const auto exposure = metadata.get(controls::ExposureTime);
	const auto gain = metadata.get(controls::AnalogueGain);
	FrameMetadata::Status status = frame_buffer->metadata().status;
	bool exposure_applied = !exposure || std::abs(*exposure - camera->exposure_us) <= std::max(100.0, camera->exposure_us * 0.05);
	// A long exposure starts with a short one. Once a frame no longer carries that start exposure the
	// requested one is in effect, even if the sensor quantizes it outside the tolerance; dropping such
	// frames would cost a full exposure each and run into the driver's readout timeout.
	if (!exposure_applied && camera->start_exposure_us != camera->exposure_us && std::abs(*exposure - camera->start_exposure_us) > std::max(100.0, camera->start_exposure_us * 0.05) && *exposure > camera->start_exposure_us) {
		exposure_applied = true;
	}
	// The Raspberry Pi pipeline returns the first frames after start as errors and then as startup
	// frames until its control algorithms settle. A startup frame that already carries the requested
	// exposure is a valid RAW frame; errors and frames with another exposure are dropped.
	bool usable = status == FrameMetadata::FrameSuccess || status == FrameMetadata::FrameStartup;
	if (camera->discarded < LCAM_MAX_DISCARDED_FRAMES && (!usable || !exposure_applied)) {
		camera->discarded++;
		request->reuse(Request::ReuseBuffers);
		if (camera->camera->queueRequest(request) < 0) {
			copy_error(error, error_size, "Request queueing failed");
			return -1;
		}
		return 0;
	}
	if (!usable) {
		snprintf(error, error_size, "Frame capture failed (status %d)", (int)status);
		return -1;
	}
	if (width == 0 || height == 0 || left + width > configuration.size.width || top + height > configuration.size.height || (size_t)width * height > buffer_size / sizeof(uint16_t)) {
		copy_error(error, error_size, "Invalid frame rectangle");
		return -1;
	}
	const FrameBuffer::Plane &plane = frame_buffer->planes()[0];
	// MIPI CSI-2 packing stores 10-bit samples in groups of 4 in 5 bytes and 12-bit samples in groups of 2 in 3 bytes; sample() reads whole groups.
	size_t width_pixels = configuration.size.width;
	size_t row_bytes = camera->format.container == 16 ? width_pixels * 2 : camera->format.container == 8 ? width_pixels : camera->format.container == 10 ? (width_pixels + 3) / 4 * 5 : (width_pixels + 1) / 2 * 3;
	if (camera->stride < row_bytes || (size_t)camera->stride * configuration.size.height > plane.length) {
		copy_error(error, error_size, "Invalid frame layout");
		return -1;
	}
	const uint8_t *data = (const uint8_t *)mapping->second.first + plane.offset;
	// Frame buffers are DMA memory the CPU reads uncached: sample-by-sample access took 1.2 s for an
	// OV5647 full frame on a Raspberry Pi 3. Each row is copied in one pass into cached memory first.
	struct dma_buf_sync sync = { DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ };
	ioctl(plane.fd.get(), DMA_BUF_IOCTL_SYNC, &sync);
	camera->row.resize(row_bytes);
	for (unsigned y = 0; y < height; y++) {
		memcpy(camera->row.data(), data + (size_t)(top + y) * camera->stride, row_bytes);
		const uint8_t *row = camera->row.data();
		uint16_t *target = buffer + (size_t)y * width;
		for (unsigned x = 0; x < width; x++) {
			target[x] = (uint16_t)sample(camera, row, left + x);
		}
	}
	sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
	ioctl(plane.fd.get(), DMA_BUF_IOCTL_SYNC, &sync);
	frame->width = width;
	frame->height = height;
	frame->bit_depth = camera->format.bit_depth;
	frame->exposure = exposure ? *exposure / 1e6 : camera->exposure_us / 1e6;
	frame->gain = gain ? *gain : 0;
	if (camera->streaming) {
		request->reuse(Request::ReuseBuffers);
		if (camera->camera->queueRequest(request) < 0) {
			copy_error(error, error_size, "Request queueing failed");
			return -1;
		}
	}
	return 1;
}

extern "C" void lcam_stop(lcam_camera *camera) {
	if (camera) {
		release_capture(camera);
	}
}

#else

// libcamera development files were not found at build time: the driver builds and loads, but
// reports no cameras.

extern "C" bool lcam_initialize(void) {
	return false;
}

extern "C" void lcam_terminate(void) {
}

extern "C" int lcam_list(lcam_camera_info *list, int size) {
	return 0;
}

extern "C" lcam_camera *lcam_open(const char *id, lcam_properties *properties, char *error, size_t error_size) {
	if (error && error_size) {
		snprintf(error, error_size, "Driver was built without libcamera");
	}
	return NULL;
}

extern "C" void lcam_close(lcam_camera *camera) {
}

extern "C" bool lcam_start(lcam_camera *camera, int mode, double exposure, double gain, bool streaming, char *error, size_t error_size) {
	return false;
}

extern "C" int lcam_read(lcam_camera *camera, uint16_t *buffer, size_t buffer_size, unsigned left, unsigned top, unsigned width, unsigned height, lcam_frame *frame, char *error, size_t error_size) {
	return -1;
}

extern "C" void lcam_stop(lcam_camera *camera) {
}

#endif
