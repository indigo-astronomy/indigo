// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
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
// 1.0 by Rohan Salodkar <rohan5sep@gmail.com>

/** INDIGO Raspberry Pi CCD driver
 \file indigo_ccd_rpi.c
 */

#define DRIVER_VERSION 0x001
#define DRIVER_NAME "indigo_ccd_rpi"

#include <assert.h>
#include <condition_variable>
#include <ctype.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "indigo_ccd_rpi.h"
#if defined(__linux__) && (defined(__aarch64__) || defined(__arm__))

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
// libcamera headers
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/transform.h>

#include <mutex>
#include <queue>

#include <sys/mman.h>

#define PRIVATE_DATA ((rpi_private_data *)device->private_data)

#define X_CCD_ADVANCED_PROPERTY (PRIVATE_DATA->advanced_property)

#define X_CCD_CONTRAST_ITEM (X_CCD_ADVANCED_PROPERTY->items + 0)
#define X_CCD_SATURATION_ITEM (X_CCD_ADVANCED_PROPERTY->items + 1)
#define X_CCD_BRIGHTNESS_ITEM (X_CCD_ADVANCED_PROPERTY->items + 2)
#define X_CCD_GAMMA_ITEM (X_CCD_ADVANCED_PROPERTY->items + 3)

#define RPI_AF_PROPERTY (PRIVATE_DATA->rpi_af_property)
#define RPI_AF_ITEM (RPI_AF_PROPERTY->items + 0)

#define RPI_AF_ITEM_NAME "AF"
#define RPI_AF_PROPERTY_NAME "RPi AF"

// gp_bits is used as boolean
#define is_connected                     gp_bits

using namespace libcamera;
using namespace std::chrono_literals;
using namespace std;
static std::mutex device_mutex;
static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
// *******************************************************************************************************
// *************************************IMPORTANT
// NOTE****************************************************
// *******************************************************************************************************
// It seems MIPI CSI-2P formats are not supported by INDIGO.
// This somewhat limits the driver capabilities, as most of the cameras
// use MIPI CSI-2P formats for RAW data. This either needs to be fixed in INDIGO
// or the driver needs to have some function to convert MIPI CSI-2P formats to
// regular formats. Something like this:
// #include <stdint.h>
// #include <stdio.h>
// void unpack_raw10(const uint8_t *input, uint16_t *output, size_t num_pixels)
// {
//     size_t in_index = 0;
//     size_t out_index = 0;
//     while (out_index + 3 < num_pixels) {
//         uint8_t b0 = input[in_index++];
//         uint8_t b1 = input[in_index++];
//         uint8_t b2 = input[in_index++];
//         uint8_t b3 = input[in_index++];
//         uint8_t b4 = input[in_index++];
//         output[out_index++] = ((uint16_t)b0) | ((b4 & 0b00000011) << 8); //
//         P0 output[out_index++] = ((uint16_t)b1) | ((b4 & 0b00001100) << 6);
//         // P1 output[out_index++] = ((uint16_t)b2) | ((b4 & 0b00110000) <<
//         4); // P2 output[out_index++] = ((uint16_t)b3) | ((b4 & 0b11000000)
//         << 2); // P3
//     }
// }
static const std::map<libcamera::PixelFormat, unsigned int> raw_bayer = {
	// { libcamera::formats::SRGGB8, 8 },
	// { libcamera::formats::SGRBG8, 8 },
	// { libcamera::formats::SBGGR8, 8 },
	// { libcamera::formats::SRGGB10, 10 },
	// { libcamera::formats::SGRBG10, 10 },
	// { libcamera::formats::SBGGR10, 10 },
	// { libcamera::formats::SGBRG10, 10 },
	// { libcamera::formats::SRGGB12, 12 },
	// { libcamera::formats::SGRBG12, 12 },
	// { libcamera::formats::SBGGR12, 12 },
	// { libcamera::formats::SGBRG12, 12 },
	// { libcamera::formats::SRGGB14, 14 },
	// { libcamera::formats::SGRBG14, 14 },
	// { libcamera::formats::SBGGR14, 14 },
	// { libcamera::formats::SGBRG14, 14 },
	// { libcamera::formats::SRGGB16, 16 },
	// { libcamera::formats::SGRBG16, 16 },
	// { libcamera::formats::SBGGR16, 16 },
	// { libcamera::formats::SGBRG16, 16 },
	// { libcamera::formats::SGBRG8, 8},
	//{ libcamera::formats::SRGGB10_CSI2P, 10}

};

// clang-format off
static const std::map<libcamera::PixelFormat, unsigned int> monochrome_formats = {
	{libcamera::formats::R8, 8},
	{libcamera::formats::R12, 12},
	{libcamera::formats::R16, 16},
	{libcamera::formats::R10, 10}
};

static const std::map<libcamera::PixelFormat, unsigned int> bayer_formats = {
	{libcamera::formats::RGB888, 24}, 
	{libcamera::formats::BGR888, 24},
	// TODO: MIPI CSI-2P formats are not supported by INDIGO.
	// This somewhat limits the driver capabilities, as most of the cameras
	// use MIPI CSI-2P formats for RAW data.
};

static const std::map<std::string, libcamera::PixelFormat> pixelFormatMap = {
	{string("MON8"), libcamera::formats::R8},
	{string("MON10"), libcamera::formats::R10_CSI2P},
	// TODO: MIPI CSI-2P formats are not supported by INDIGO.
	// This somewhat limits the driver capabilities, as most of the cameras
	// use MIPI CSI-2P formats for RAW data.
	// {string("MON10_CSI2P"), libcamera::formats::R10_CSI2P},
	{string("MON12"), libcamera::formats::R12},
	{string("MON16"), libcamera::formats::R16},
	{string("RGB16"), libcamera::formats::RGB565},
	{string("BGR24"), libcamera::formats::BGR888},
	{string("RGB24"), libcamera::formats::RGB888},
	{string("RAW16"), libcamera::formats::SRGGB16},
	{string("RAW8"), libcamera::formats::SRGGB8},
	{string("RAW10"), libcamera::formats::SRGGB10},
	{string("RAW12"), libcamera::formats::SRGGB12},
	{string("RAW14"), libcamera::formats::SRGGB14}
};
// clang-format on
// datatypes
using CameraManager_p = std::unique_ptr<CameraManager>;
using Camera_p = std::shared_ptr<Camera>;
using Config_p = std::unique_ptr<CameraConfiguration>;
using Request_p = std::unique_ptr<Request>;
using Requests_p = std::vector<Request_p>;
using CameraIDs = std::vector<std::string>;
using Cameras = std::vector<Camera_p>;
using CameraIDMap = std::map<std::string, Camera_p>;
using PixelFormats = std::vector<libcamera::PixelFormat>;
using ImageDimensions = std::vector<libcamera::Size>;
using FrameInfo = std::map<libcamera::Size, libcamera::Size>;

static char previousSDKLogLevel[64] = {'\0'};

void ResetSDKLogLevel() {
	if (previousSDKLogLevel[0] == '\0') {
		unsetenv("LIBCAMERA_LOG_LEVELS"); // Reset the log level for the libcamera SDK to default
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Unset LIBCAMERA_LOG_LEVELS");
	} else {
		// Reset the log level for the libcamera SDK to default
		setenv("LIBCAMERA_LOG_LEVELS", previousSDKLogLevel, 1);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Reset LIBCAMERA_LOG_LEVELS to %s", previousSDKLogLevel);
	}
}

void SetSDKLogLevel() {
	char *logLevel = getenv("LIBCAMERA_LOG_LEVELS");
	if (logLevel != nullptr) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Current LIBCAMERA_LOG_LEVELS: %s", logLevel);
		strcpy(previousSDKLogLevel, logLevel);
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Previous LIBCAMERA_LOG_LEVELS: %s", previousSDKLogLevel);
	// Set the log level for the libcamera SDK
	indigo_log_levels indigoLogLevel = indigo_get_log_level();
	if (indigoLogLevel == INDIGO_LOG_DEBUG) {
		setenv("LIBCAMERA_LOG_LEVELS", "DEBUG", 1);
	} else if (indigoLogLevel == INDIGO_LOG_INFO) {
		setenv("LIBCAMERA_LOG_LEVELS", "WARN", 1);
	} else if (indigoLogLevel == INDIGO_LOG_ERROR) {
		setenv("LIBCAMERA_LOG_LEVELS", "ERROR", 1);
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set LIBCAMERA_LOG_LEVELS to %s", getenv("LIBCAMERA_LOG_LEVELS"));
}

std::vector<uint8_t> RemovePadding(const uint8_t *imageData, int rowSize, int height, int stride) {
	std::vector<uint8_t> output(rowSize * height);

	for (int y = 0; y < height; ++y) {
		const uint8_t *srcRow = imageData + y * stride;
		uint8_t *dstRow = output.data() + y * rowSize;
		memcpy(dstRow, srcRow, rowSize);
	}

	return output;
}

// Get camera limits: exposure, gain, etc.
template <typename T> struct Limits {
	T min;
	T max;

	std::string toString() {
		std::ostringstream oss;
		oss << "{ min: " << min << ", max: " << max << "}";

		return oss.str();
	}
};

// Image data read from the frame buffer associated with libcamera::Request
struct FrameData {
	std::vector<uint8_t> imageData;
	uint64_t request;
};

// RegisterCamaraManager singleton class to manage camera instances
// Starts and manages life cycle of libcamera::CameraManager
struct RegisterCameraManager {
	// Accessor methods to get cameras and camera IDs
	Cameras GetCameras() const {
		Cameras cameras = m_cm->cameras();
		// This driver is designed to work with CSI port only
		auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; });
		cameras.erase(rem, cameras.end());
		std::sort(cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); });
		return cameras;
	}

	// Get all camera IDs.
	// IDs are guaranteed to be unique and stable: the same camera, when
	// connected to the system in the same way (e.g. in the same USB port), will
	// have the same ID across both unplug/replug and system reboots.
	CameraIDs GetCameraIDs() const {
		Cameras cameras = GetCameras();
		CameraIDs cameraIds;

		for (const auto &camera : cameras) {
			cameraIds.push_back(camera->id());
		}

		return cameraIds;
	}

	Camera_p GetCamera(std::string cameraId) const {
		Cameras cameras = GetCameras();
		for (const auto &camera : cameras) {
			if (camera->id() == cameraId) {
				return camera;
			}
		}
		return nullptr;
	}

	static RegisterCameraManager *getInstance() {
		if (!instance) {
			instance = new RegisterCameraManager();
		}
		return instance;
	}

  private:
	// Singleton instance
	static RegisterCameraManager *instance;
	// CameraManager instance
	CameraManager_p m_cm;

	RegisterCameraManager() : m_cm(std::make_unique<CameraManager>()) {
		if (m_cm->start() < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to start CameraManager");
		}
	}

	~RegisterCameraManager() {
		if (m_cm) {
			m_cm->stop();
			m_cm.reset();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CameraManager stopped and resources released.");
		}
	}
};

RegisterCameraManager *RegisterCameraManager::instance;

// RPiCamera instance represents a physical camera sensor attached to RPI
struct RPiCamera {
	// Callbacks
	void OnRequestComplete(Request *request);
	// Camera focuser
	bool operator()(indigo_device *device, int steps);
	// Initialization
	int InitCamera(std::string cameraId);

	RPiCamera() {}

	// Accessors
	bool IsMonochrome() const;
	const ControlInfoMap GetControls() const;
	bool GetExposureLimits(Limits<unsigned> &exposureLimits);
	bool GetGainLimits(Limits<float> &gainLimits);

	bool IsCameraAcquired() const { return m_cameraAcquired; }

	optional<Size> GetPixelSize();
	Size GetImageSize() const;
	optional<Size> GetBinning();

	PixelFormat GetPixelFormat() const { return m_config->at(0).pixelFormat; }

	int GetMaxBinning();
	PixelFormats GetPixelFormats();
	bool HasAutoFocus() const;
	bool ReadFrame(FrameData *frameData);
	Stream *VideoStream(uint32_t *w, uint32_t *h, uint32_t *stride) const;

	optional<float> GetAnalogueGain() const {
		if (!m_cameraAcquired) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
			return {};
		}
		return m_controls.get(controls::AnalogueGain);
	}

	optional<float> GetDigitalGain() const {
		if (!m_cameraAcquired) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
			return nullopt;
		}
		return m_controls.get(controls::DigitalGain);
	}

	bool IsRequestCreated() const { return !m_requests.empty(); }

	bool IsAnyRequestComplete() const {
		return std::any_of(m_requests.begin(), m_requests.end(), [](const Request_p &req) { return req->status() == Request::RequestComplete; });
	}

	Size GetMaxImageSize() const;
	FrameInfo GetFrameInfo(const PixelFormat &format);

	optional<float> GetSensorTemperature() const { return m_controls.get(controls::SensorTemperature); }

	// Controls
	int ConfigureCamera();

	// Operations
	int StartExposure(int width, int height, PixelFormat format);
	int StartCamera();
	void ReturnFrameBuffer(FrameData frameData);

	void Cleanup() {
		if (m_camera) {
			m_camera->stop();
			m_camera->release();
			m_camera.reset();
		}
		m_requests.clear();
		m_config.reset();
	}

	void Set(ControlList controls);
	void StopCamera();
	void CloseCamera();
	void ProcessRequest(Request *request);
	// Camera auto-focus trigger
	bool af();

	~RPiCamera() {}

  private:
	// Camera states
	bool m_cameraAcquired = false;
	bool m_cameraStarted = false;

	Camera_p m_camera{};
	Config_p m_config{};
	Requests_p m_requests{};
	std::unique_ptr<FrameBufferAllocator> m_allocator;

	Stream *m_stream = nullptr;
	std::string cameraId;
	PixelFormats m_pixelFormats{libcamera::formats::SRGGB8}; // Default pixel format
	bool m_afEnabled = false;
	std::queue<Request *> m_requestQueue;
	std::map<int, std::pair<void *, unsigned int>> m_mappedBuffers;
	ControlList m_controls;
	std::mutex m_cameraMutex;
	std::condition_variable m_cv;
	bool m_exposureDone = false;
	void ConfigureStill(int width, int height, PixelFormat format);
	int StartCapture();
	int QueueRequest(Request *request);
	void StreamDimensions(Stream const *stream, uint32_t *w, uint32_t *h, uint32_t *stride) const;
	void Reset();
};

bool RPiCamera::IsMonochrome() const {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return false;
	}

	return properties::draft::ColorFilterArrangementEnum::MONO == m_camera->properties().get(properties::draft::ColorFilterArrangement);
}

void RPiCamera::OnRequestComplete(Request *request) {
	std::lock_guard<std::mutex> lock(m_cameraMutex);
	// Handle request completion
	if (request->status() == Request::RequestCancelled) {
		return;
	}
	ProcessRequest(request);
	m_exposureDone = true; // Mark exposure as done
	m_cv.notify_one();	   // wake up any ReadFrame waiting on this generation
}

bool RPiCamera::operator()(indigo_device *device, int steps) {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return false;
	}
	if (!m_cameraStarted) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not started");
		return false;
	}
	m_controls.set(controls::AfMode, controls::AfModeManual);

	// 0 moves the lens to infinity.
	// 0.5 moves the lens to focus on objects 2m away.
	// 2 moves the lens to focus on objects 50cm away.
	// And larger values will focus the lens closer.
	// The default value of the control should indicate a good general position
	// for the lens, often corresponding to the hyperfocal distance (the closest
	// position for which objects at infinity are still acceptably sharp). The
	// minimum will often be zero (meaning infinity), and the maximum value
	// defines the closest focus position.
	float pos = steps / 10.0f; // Convert steps to a float position
	m_controls.set(controls::LensPosition, pos);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "m_controls.set(controls::LensPosition, pos %d)", pos);
	return true;
}

int RPiCamera::InitCamera(std::string cameraId) {

	RegisterCameraManager *cameraManager_ = RegisterCameraManager::getInstance();
	m_camera = cameraManager_->GetCamera(cameraId);
	if (!m_camera) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera %s not found", cameraId.c_str());
		return 1;
	}

	if (m_camera->acquire()) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to acquire camera %s", cameraId.c_str());
		return 1;
	}
	m_cameraAcquired = true;
	return 0;
}

const ControlInfoMap RPiCamera::GetControls() const {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return {};
	}
	return m_camera->controls();
}

bool RPiCamera::GetExposureLimits(Limits<unsigned> &exposureLimits) {

	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return false;
	}

	const ControlInfoMap &controls = m_camera->controls();

	// Exposure limits
	auto exposureInfo = controls.find(&controls::ExposureTime);
	if (exposureInfo != controls.end()) {
		exposureLimits.min = exposureInfo->second.min().get<int32_t>();
		exposureLimits.max = exposureInfo->second.max().get<int32_t>();
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure limits not available");

		return false;
	}

	return true;
}

bool RPiCamera::GetGainLimits(Limits<float> &gainLimits) {

	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return false;
	}
	const ControlInfoMap &controls = m_camera->controls();

	// Gain limits
	auto gainInfo = controls.find(&controls::AnalogueGain);
	if (gainInfo != controls.end()) {
		gainLimits.min = gainInfo->second.min().get<float>();
		gainLimits.max = gainInfo->second.max().get<float>();
	} else {
		cout << "Gain limits not available" << endl;

		return false;
	}

	return true;
}

optional<Size> RPiCamera::GetPixelSize() {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return {};
	}
	libcamera::ControlList c = m_camera->properties();
	return c.get(libcamera::properties::UnitCellSize);
}

Size RPiCamera::GetImageSize() const {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return Size(0, 0);
	}

	if (m_config && !m_config->at(0).size.isNull()) {
		return m_config->at(0).size;
	}

	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera configuration not set");
	return Size(0, 0);
}

optional<Size> RPiCamera::GetBinning() {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return {};
	}

	return Size(m_config->sensorConfig->analogCrop.width / m_config->at(0).size.width, m_config->sensorConfig->analogCrop.height / m_config->at(0).size.height);
}

int RPiCamera::GetMaxBinning() {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return 0;
	}
	return GetFrameInfo(GetPixelFormat()).size();
}

PixelFormats RPiCamera::GetPixelFormats() {

	PixelFormats pixelFormats;
	for (StreamConfiguration &config : *m_config) {
		for (const PixelFormat &format : config.formats().pixelformats()) {

			pixelFormats.push_back(format);
		}
	}
	return pixelFormats;
}

bool RPiCamera::HasAutoFocus() const {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return false;
	}
	// Check if the camera has a focus control
	const ControlInfoMap &controls = m_camera->controls();
	return controls.find(&controls::AfMode) != controls.end();
}

bool RPiCamera::ReadFrame(FrameData *frameData) {
	std::unique_lock<std::mutex> lock(m_cameraMutex);
	m_cv.wait(lock, [this] { return m_exposureDone; }); // Wait until done
	// int w, h, stride;
	if (!m_requestQueue.empty()) {
		Request *request = this->m_requestQueue.front();

		const Request::BufferMap &buffers = request->buffers();
		for (auto it = buffers.begin(); it != buffers.end(); ++it) {
			FrameBuffer *buffer = it->second;
			for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
				const FrameBuffer::Plane &plane = buffer->planes()[i];
				const FrameMetadata::Plane &meta = buffer->metadata().planes()[i];

				void *data = m_mappedBuffers[plane.fd.get()].first;
				int length = std::min(meta.bytesused, plane.length);

				frameData->imageData.assign(static_cast<uint8_t *>(data), static_cast<uint8_t *>(data) + length);
			}
		}
		this->m_requestQueue.pop();
		frameData->request = (uint64_t)request;
		return true;
	} else {
		Request *request = nullptr;
		frameData->request = (uint64_t)request;
		return false;
	}
}

Stream *RPiCamera::VideoStream(uint32_t *w, uint32_t *h, uint32_t *stride) const {
	StreamDimensions(m_stream, w, h, stride);
	return m_stream;
}

Size RPiCamera::GetMaxImageSize() const {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return {};
	}

	std::unique_ptr<CameraConfiguration> config = m_camera->generateConfiguration({StreamRole::StillCapture});
	libcamera::CameraConfiguration::Status status = config->validate();
	if (status != libcamera::CameraConfiguration::Valid) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera configuration is not valid");
		return {};
	}
	StreamConfiguration &streamConfig = config->at(0);
	// Get the format
	PixelFormat pixelFormat = streamConfig.pixelFormat;
	// Access the format map to find all supported sizes
	const StreamFormats &formats = config->at(0).formats();
	// Get all sizes for the pixel format
	std::vector<Size> sizes = formats.sizes(pixelFormat);
	// Find the largest one
	auto maxSize = *std::max_element(sizes.begin(), sizes.end(), [](const Size &a, const Size &b) { return a.width * a.height < b.width * b.height; });

	config.reset(); // Clean up the configuration

	return maxSize;
}

bool RPiCamera::af() {
	if (HasAutoFocus()) {
		m_controls.set(controls::AfMode, controls::AfModeAuto);
	}
	return HasAutoFocus();
}

// Returns binning to image size map. Only considers
// bin_x == bin_y.
FrameInfo RPiCamera::GetFrameInfo(const PixelFormat &format) {
	if (!m_cameraAcquired) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not acquired");
		return {};
	}
	const auto &sz = GetMaxImageSize();
	const StreamFormats &formats = m_config->at(0).formats();

	auto sizes = formats.sizes(format);
	// sort size by biggest width and height
	std::sort(sizes.begin(), sizes.end(), [](const Size &a, const Size &b) { return (a.width * a.height) > (b.width * b.height); });
	FrameInfo frameInfo;
	for (auto size : sizes) {
		if (sz.width / size.width == sz.height / size.height) {
			frameInfo.insert(std::make_pair(Size(sz.width / size.width, sz.height / size.height), size));
		}
	}
	return frameInfo;
}

void RPiCamera::StreamDimensions(Stream const *stream, uint32_t *w, uint32_t *h, uint32_t *stride) const {
	StreamConfiguration const &cfg = stream->configuration();
	if (w) {
		*w = cfg.size.width;
	}
	if (h) {
		*h = cfg.size.height;
	}
	if (stride) {
		*stride = cfg.stride;
	}
}

void RPiCamera::ConfigureStill(int width, int height, PixelFormat format) {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Configuring still capture...");

	StreamRole role = (IsMonochrome() || raw_bayer.find(format) != raw_bayer.end()) ? StreamRole::Raw : StreamRole::StillCapture;
	m_config = m_camera->generateConfiguration({role});
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Configuring camera for still capture with role: %s", role == StreamRole::Raw ? "Raw" : "StillCapture");
	if (width && height) {
		libcamera::Size size(width, height);
		m_config->at(0).size = size;
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, " m_config->at(0).pixelFormat = format: %s", format.toString().c_str());
	m_config->at(0).pixelFormat = format;
	m_config->at(0).bufferCount = 1; // Default buffer count

	CameraConfiguration::Status validation = m_config->validate();
	if (validation == CameraConfiguration::Invalid) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to valid stream configurations");
	} else if (validation == CameraConfiguration::Adjusted) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stream configuration adjusted");
	}

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Still capture setup complete...");
}

int RPiCamera::StartCamera() {
	int ret;
	ret = m_camera->configure(m_config.get());
	if (ret < 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failed to configure camera");
		return ret;
	}

	m_camera->requestCompleted.connect(this, &RPiCamera::OnRequestComplete);

	m_allocator = std::make_unique<FrameBufferAllocator>(m_camera);

	return StartCapture();
}

int RPiCamera::StartCapture() {
	int ret;
	unsigned int nbuffers = UINT_MAX;
	for (StreamConfiguration &cfg : *m_config) {
		ret = m_allocator->allocate(cfg.stream());
		if (ret < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't allocate buffers");
			return -ENOMEM;
		}

		unsigned int allocated = m_allocator->buffers(cfg.stream()).size();
		nbuffers = std::min(nbuffers, allocated);
	}

	for (unsigned int i = 0; i < nbuffers; i++) {
		std::unique_ptr<Request> request = m_camera->createRequest();
		if (!request) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't create request");
			return -ENOMEM;
		}

		for (StreamConfiguration &cfg : *m_config) {
			Stream *stream = cfg.stream();
			const std::vector<std::unique_ptr<FrameBuffer>> &buffers = m_allocator->buffers(stream);

			const std::unique_ptr<FrameBuffer> &buffer = buffers[i];

			ret = request->addBuffer(stream, buffer.get());
			if (ret < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't set buffer for request");
				return ret;
			}
			for (const FrameBuffer::Plane &plane : buffer->planes()) {
				void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
				m_mappedBuffers[plane.fd.get()] = std::make_pair(memory, plane.length);
			}
		}

		m_requests.push_back(std::move(request));
	}

	ret = m_camera->start(&this->m_controls);

	if (ret) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to start capture");
		return ret;
	}

	m_cameraStarted = true;
	for (std::unique_ptr<Request> &request : m_requests) {
		ret = QueueRequest(request.get());
		if (ret < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't queue request");
			m_camera->stop();
			return ret;
		}
	}
	m_stream = m_config->at(0).stream();
	return 0;
}

void RPiCamera::Set(ControlList controls) { this->m_controls.merge(controls, ControlList::MergePolicy::OverwriteExisting); }

int RPiCamera::ConfigureCamera() {
	Reset();
	Config_p config = m_camera->generateConfiguration({StreamRole::Raw});

	CameraConfiguration::Status validation = config->validate();
	if (validation == CameraConfiguration::Invalid) {
		return -1; // Invalid configuration
	} else if (validation == CameraConfiguration::Adjusted) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Stream configuration adjusted");
	}
	// Collect all available pixel formats
	m_pixelFormats = config->at(0).formats().pixelformats();

	if (!IsMonochrome()) {
		Config_p config_still = m_camera->generateConfiguration({StreamRole::StillCapture});
		if (config_still->validate() == CameraConfiguration::Valid) {
			m_pixelFormats.clear();
			auto formats = config_still->at(0).formats().pixelformats();

			m_pixelFormats.insert(m_pixelFormats.end(), formats.begin(), formats.end());
			m_config = std::move(config_still);
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to validate still capture configuration");
			return -1;
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Monochrome formats available");
		m_config = std::move(config);
	}

	m_config->at(0).size = GetMaxImageSize();		 // Default size
	m_config->at(0).pixelFormat = m_pixelFormats[0]; // Default pixel format
	m_config->at(0).bufferCount = 1;				 // Default buffer count

	CameraConfiguration::Status status = m_config->validate();

	if (status == CameraConfiguration::Valid || status == CameraConfiguration::Adjusted) {
		return StartCamera();
	}

	return -1; // Failed to validate configuration
}

int RPiCamera::StartExposure(int width, int height, PixelFormat format) {
	std::lock_guard<std::mutex> lock(m_cameraMutex);
	m_exposureDone = false; // Reset exposure done flag
	Reset();
	ConfigureStill(width, height, format);
	return StartCamera();
}

void RPiCamera::Reset() {
	if (m_camera) {
		{
			if (m_cameraStarted) {
				if (m_camera->stop()) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "failed to stop camera");
				}
				m_cameraStarted = false;
			}
		}
		m_camera->requestCompleted.disconnect(this, &RPiCamera::OnRequestComplete);
	}
	while (!m_requestQueue.empty()) {
		m_requestQueue.pop();
	}

	for (auto &iter : m_mappedBuffers) {
		std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}

	m_mappedBuffers.clear();

	m_requests.clear();

	m_allocator.reset();
}

void RPiCamera::StopCamera() {
	Reset();
	m_controls.clear();
}

void RPiCamera::CloseCamera() {
	if (m_cameraAcquired) {
		m_camera->release();
	}
	m_cameraAcquired = false;
	m_camera.reset();
}

void RPiCamera::ReturnFrameBuffer(FrameData frameData) {
	if (m_cameraStarted) {
		uint64_t request = frameData.request;
		Request *req = (Request *)request;
		req->reuse(Request::ReuseBuffers);
		QueueRequest(req);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Camera not started, cannot return frame buffer");
	}
}

int RPiCamera::QueueRequest(Request *request) {
	if (!m_cameraStarted) {
		return -1;
	}
	{
		request->controls() = std::move(m_controls);
	}
	return m_camera->queueRequest(request);
}

void RPiCamera::ProcessRequest(Request *request) { m_requestQueue.push(request); }

typedef struct {
	std::string eid;
	RPiCamera *pRPiCamera;
	unsigned char *buffer;
	indigo_timer *exposure_timer;
	RPiCamera *focus;
	indigo_device *focuser;
	indigo_property *rpi_af_property;
	indigo_property *advanced_property;
} rpi_private_data;

static void handle_af(indigo_device *device) {

	std::lock_guard<std::mutex> lock(device_mutex);

	RPI_AF_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, RPI_AF_PROPERTY, NULL);
	if (PRIVATE_DATA->pRPiCamera->af()) {
		RPI_AF_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		RPI_AF_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	RPI_AF_ITEM->sw.value = false;
	indigo_update_property(device, RPI_AF_PROPERTY, NULL);
}

// --------------------------------------------------------------------------------
// INDIGO CCD device implementation
static void exposure_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		int state = 0;
		bool ready = false;

		RPiCamera *pRPiCamera = PRIVATE_DATA->pRPiCamera;
		ready = pRPiCamera->IsRequestCreated() ? pRPiCamera->IsAnyRequestComplete() : -1;

		while (state != -1 && !ready) {
			indigo_usleep(200);
			ready = pRPiCamera->IsAnyRequestComplete();
		}

		FrameData frameData;
		if (state != -1) {
			state = pRPiCamera->ReadFrame(&frameData);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->ReadFrame(&frameData) -> %d", state);
		}
		if (state != -1) {
			PixelFormat format = pRPiCamera->GetPixelFormat();
			auto bppMap = bayer_formats.find(format);
			if (bppMap == bayer_formats.end()) {
				bppMap = monochrome_formats.find(format);
			}
			if (bppMap == monochrome_formats.end()) {
				bppMap = raw_bayer.find(format);
			}
			if (bppMap == raw_bayer.end()) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Unsupported pixel format: %s", format.toString().c_str());
				indigo_ccd_failure_cleanup(device);
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, nullptr);
				return;
			}
			int bpp = bppMap->second;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Pixel format: %s, BPP: %d", format.toString().c_str(), bpp);
			uint32_t width, height, stride;
			pRPiCamera->VideoStream(&width, &height, &stride);

			unsigned int rowSize = bpp * width / 8; // Calculate row size in bytes

			if (rowSize < stride) {
				frameData.imageData = RemovePadding(frameData.imageData.data(), rowSize, height, stride);
			}

			memcpy((char *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), frameData.imageData.data(), frameData.imageData.size());
			std::string bayerPattern = format.toString();
			bayerPattern.erase(std::remove_if(bayerPattern.begin(), bayerPattern.end(), ::isdigit), bayerPattern.end());
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bayer pattern: %s", bayerPattern.c_str());
			indigo_fits_keyword keywords[2];
			// Make the c++ compiler happy by strongly typing
			// the nested types
			keywords[0].type = INDIGO_FITS_STRING;
			keywords[0].name = "BAYERPAT";
			keywords[0].string = bayerPattern.c_str();
			keywords[0].comment = "Bayer color pattern";
			// end
			keywords[1].type = (indigo_fits_keyword_type)0;
			keywords[1].name = " ";
			keywords[1].string = " ";
			keywords[1].comment = " ";

			indigo_fits_keyword *fits_keywords = NULL;
			if (bpp != 24 && bpp != 48 && bayer_formats.find(format) != bayer_formats.end()) {
				fits_keywords = keywords;
			}
			indigo_process_image(device, PRIVATE_DATA->buffer, width, height, bpp, true, true, fits_keywords, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			indigo_ccd_failure_cleanup(device);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to read frame data");
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, nullptr);
		}
	}
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		// RPI_AF
		RPI_AF_PROPERTY = indigo_init_switch_property(NULL, device->name, RPI_AF_PROPERTY_NAME, "RPi AF", "Autofocus", INDIGO_OK_STATE, INDIGO_RW_PERM,
													  INDIGO_AT_MOST_ONE_RULE, 1);
		if (RPI_AF_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		RPI_AF_PROPERTY->hidden = NULL;
		indigo_init_switch_item(RPI_AF_ITEM, RPI_AF_ITEM_NAME, "Start autofocus", false);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
		INDIGO_FAILED;
	}
	return INDIGO_FAILED;
}

static void ccd_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) {
			if (indigo_try_global_lock(device) != INDIGO_OK) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "indigo_try_global_lock(): failed to get lock.");
			} else {
				indigo_define_property(device, RPI_AF_PROPERTY, NULL);
				// Create new camera instance
				PRIVATE_DATA->pRPiCamera = new RPiCamera();
				PRIVATE_DATA->pRPiCamera->InitCamera(PRIVATE_DATA->eid);
			}
		}
		RPiCamera *pRPiCamera = PRIVATE_DATA->pRPiCamera;
		if (pRPiCamera && pRPiCamera->IsCameraAcquired()) {

			Size maxImageSize = pRPiCamera->GetMaxImageSize();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->GetMaxImageSize() -> %s ", maxImageSize.toString().c_str());
			CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max =
				maxImageSize.width;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CCD_INFO_WIDTH_ITEM->number.value = %f", CCD_INFO_WIDTH_ITEM->number.value);
			CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max =
				maxImageSize.height;

			const auto &pixelSize = pRPiCamera->GetPixelSize();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->GetPixelSize() %s", pixelSize->toString().c_str());
			if (pixelSize) {
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(pixelSize->width / 10.0) / 100.0;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(pixelSize->height / 10.0) / 100.0;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get pixel size");
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = 0.0;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 0.0;
			}
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 24;
			CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
			// Temporarily start the camera to get the metadata
			int configured = pRPiCamera->ConfigureCamera();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiamera->ConfigureCamera() -> %d", configured);

			if (pRPiCamera->GetMaxBinning()) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RPiCamera->GetMaxBinning() -> %d)", pRPiCamera->GetMaxBinning());
				CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = pRPiCamera->GetMaxBinning();
				CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = pRPiCamera->GetMaxBinning();
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get min/max binning");
				CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 1;
				CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;
			}
			Limits<unsigned> exposureLimits;
			if (pRPiCamera->GetExposureLimits(exposureLimits)) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->GetExposureLimits(exposureLimits %s)", exposureLimits.toString().c_str());
				CCD_EXPOSURE_ITEM->number.min = exposureLimits.min / 1000000.0;
				CCD_EXPOSURE_ITEM->number.max = exposureLimits.max / 1000000.0;
			} else {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to get exposure limits");
				CCD_EXPOSURE_ITEM->number.min = 0.0;
				CCD_EXPOSURE_ITEM->number.max = 1.0;
			}
			// Get available pixel formats
			PixelFormats pixelFormats = pRPiCamera->GetPixelFormats();
			CCD_MODE_PROPERTY->count = 0;
			for (const auto &format : pixelFormats) {

				if (bayer_formats.find(format) != bayer_formats.end()) {

					FrameInfo frameInfo = pRPiCamera->GetFrameInfo(format);
					char name[32];

					for (auto frame : frameInfo) {
						int frame_width = frame.second.width;
						int frame_height = frame.second.height;
						char description[64];
						std::string strFormat = bayer_formats.find(format)->first.toString();
						strFormat.erase(std::remove_if(strFormat.begin(), strFormat.end(), ::isdigit), strFormat.end());
						strFormat += std::to_string(bayer_formats.find(format)->second);
						sprintf(description, "%s %dx%d", strFormat.c_str(), frame_width, frame_height);
						sprintf(name, "%s_%d", strFormat.c_str(), frame.first.width);
						indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, description, CCD_MODE_PROPERTY->count == 0);
						CCD_MODE_PROPERTY->count++;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CCD_MODE_PROPERTY->count++; %d", CCD_MODE_PROPERTY->count);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "name %s ", name);
					}
				}

				if (raw_bayer.find(format) != raw_bayer.end()) {

					char name[32];
					FrameInfo frameInfo = pRPiCamera->GetFrameInfo(format);
					for (auto frame : frameInfo) {
						int frame_width = frame.second.width;
						int frame_height = frame.second.height;
						char description[64];
						std::string strFormat = "RAW";
						strFormat += std::to_string(raw_bayer.find(format)->second);
						sprintf(description, "%s %dx%d", strFormat.c_str(), frame_width, frame_height);
						sprintf(name, "%s_%d", strFormat.c_str(), frame.first.width);
						indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, description, CCD_MODE_PROPERTY->count == 0);
						CCD_MODE_PROPERTY->count++;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CCD_MODE_PROPERTY->count++; %d", CCD_MODE_PROPERTY->count);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "name %s ", name);
					}
				}
				if (monochrome_formats.find(format) != monochrome_formats.end() && pRPiCamera->IsMonochrome()) {

					FrameInfo frameInfo = pRPiCamera->GetFrameInfo(format);
					char name[32];
					for (auto frame : frameInfo) {
						int frame_width = frame.second.width;
						int frame_height = frame.second.height;
						char description[64];
						std::string strFormat = "MON";
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Format to name : %s", format.toString().c_str());
						strFormat += std::to_string(monochrome_formats.find(format)->second);
						sprintf(description, "%s %dx%d", strFormat.c_str(), frame_width, frame_height);
						sprintf(name, "%s_%d", strFormat.c_str(), frame.first.width);
						indigo_init_switch_item(CCD_MODE_ITEM + CCD_MODE_PROPERTY->count, name, description, CCD_MODE_PROPERTY->count == 0);
						CCD_MODE_PROPERTY->count++;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CCD_MODE_PROPERTY->count++; %d", CCD_MODE_PROPERTY->count);
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "name %s ", name);
					}
				}
			}

			Limits<float> gainLimits;
			bool bHAsGain = pRPiCamera->GetGainLimits(gainLimits);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCametra->GetGainLimits(gainLimits %s ) -> %d", gainLimits.toString().c_str(), bHAsGain);

			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;

			if (bHAsGain) {
				CCD_GAIN_PROPERTY->hidden = false;
				CCD_GAIN_ITEM->number.min = 0.0;
				CCD_GAIN_ITEM->number.max = gainLimits.max;
				CCD_GAIN_ITEM->number.step = 1.0;
				CCD_GAIN_ITEM->number.value = 0.0;

				if (pRPiCamera->GetAnalogueGain()) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->GetAnalogueGain() -> %g)", pRPiCamera->GetAnalogueGain().value_or(0.0));
					CCD_EGAIN_PROPERTY->hidden = false;
					CCD_EGAIN_ITEM->number.value = static_cast<double>(pRPiCamera->GetAnalogueGain().value_or(0.0));
				}
			}

			if (X_CCD_ADVANCED_PROPERTY) {
				indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
			}
			// CCD properties
			X_CCD_ADVANCED_PROPERTY =
				indigo_init_number_property(NULL, device->name, "X_CCD_ADVANCED", CCD_ADVANCED_GROUP, "Advanced Settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
			if (X_CCD_ADVANCED_PROPERTY != NULL) {
				auto controls = pRPiCamera->GetControls();

				if (controls.find(&controls::Contrast) != controls.end()) {
					float contrast_def = controls.find(&controls::Contrast)->second.def().get<float>();
					float contrast_max = controls.find(&controls::Contrast)->second.max().get<float>();
					float contrast_min = controls.find(&controls::Contrast)->second.min().get<float>();
					indigo_init_number_item(X_CCD_CONTRAST_ITEM, "CONTRAST", "Contrast", contrast_min, contrast_max, 1, contrast_def);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Contrast control not found");
				}

				if (controls.find(&controls::Saturation) != controls.end()) {
					float saturation_def = controls.find(&controls::Saturation)->second.def().get<float>();
					float saturation_max = controls.find(&controls::Saturation)->second.max().get<float>();
					float saturation_min = controls.find(&controls::Saturation)->second.min().get<float>();
					indigo_init_number_item(X_CCD_SATURATION_ITEM, "SATURATION", "Saturation", saturation_min, saturation_max, 1, saturation_def);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Saturation control not found");
				}

				if (controls.find(&controls::Brightness) != controls.end()) {
					float brightness_def = controls.find(&controls::Brightness)->second.def().get<float>();
					float brightness_max = controls.find(&controls::Brightness)->second.max().get<float>();
					float brightness_min = controls.find(&controls::Brightness)->second.min().get<float>();
					indigo_init_number_item(X_CCD_BRIGHTNESS_ITEM, "BRIGHTNESS", "Brightness", brightness_min, brightness_max, 0.1, brightness_def);
				} else {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "Brightness control not found");
				}
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->HasAutoFocus() -> %d", pRPiCamera->HasAutoFocus());
			if (pRPiCamera->HasAutoFocus()) {
				// Assign the focuser function pointer
				PRIVATE_DATA->focus = pRPiCamera;
				indigo_attach_device(PRIVATE_DATA->focuser);
			}

			PRIVATE_DATA->buffer =
				(unsigned char *)indigo_alloc_blob_buffer(3 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "indigo_alloc_blob_buffer(width %.1f, height %.1f)", CCD_INFO_WIDTH_ITEM->number.value,
								CCD_INFO_HEIGHT_ITEM->number.value);
			assert(PRIVATE_DATA->buffer != NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			device->is_connected = true; // Set the device as connected
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {

		indigo_delete_property(device, RPI_AF_PROPERTY, NULL);
		if (X_CCD_ADVANCED_PROPERTY)
			indigo_delete_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			PRIVATE_DATA->pRPiCamera->StopCamera();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->StopCamera()");
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->exposure_timer);
		}
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		if (device->is_connected) {
			PRIVATE_DATA->pRPiCamera->StopCamera();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->StopCamera()");
			PRIVATE_DATA->pRPiCamera->CloseCamera();
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->CloseCamera()");
			// If the focuser is attached, detach it
			if (PRIVATE_DATA->focuser) {
				indigo_detach_device(PRIVATE_DATA->focuser);
			}
			delete PRIVATE_DATA->pRPiCamera;
			PRIVATE_DATA->pRPiCamera = nullptr;
			device->is_connected = false;
			indigo_global_unlock(device);
		}
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void handle_focus(indigo_device *device) {
	FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	if ((*PRIVATE_DATA->focus)(device->master_device, (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -1 : 1) * FOCUSER_STEPS_ITEM->number.value)) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		// --------------------------------------------------------------------------------
		// FOCUSER_ABORT_MOTION
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			(*PRIVATE_DATA->focus)(device->master_device, 0);
		}
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
		// FOCUSER_STEPS
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_focus, NULL);
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

static void ccd_start_exposure(indigo_device *device) {
	if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
	}
	if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
	}
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);

	RPiCamera *pRPiCamera = PRIVATE_DATA->pRPiCamera;

	int state = -1;

	if (pRPiCamera->IsCameraAcquired()) {
		ControlList m_controls;
		int64_t frame_time = CCD_EXPOSURE_ITEM->number.target * 1000000;
		// Set frame rate
		m_controls.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({frame_time, frame_time}));
		m_controls.set(controls::ExposureTimeMode, controls::ExposureTimeModeManual);
		m_controls.set(controls::ExposureTime,
					   CCD_EXPOSURE_ITEM->number.target * 1000000); // Convert seconds to microseconds
		m_controls.set(controls::AeEnable, false);
		m_controls.set(controls::AwbEnable, true);

		int bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		int left = (int)CCD_FRAME_LEFT_ITEM->number.value / bin_x;
		int top = (int)CCD_FRAME_TOP_ITEM->number.value / bin_y;

		int width = CCD_INFO_WIDTH_ITEM->number.value / bin_x;
		int height = CCD_INFO_HEIGHT_ITEM->number.value / bin_y;

		std::string pixelFormatName = CCD_MODE_ITEM[0].name;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			if (CCD_MODE_ITEM[i].sw.value) {
				// Set the pixel format based on the selected mode
				pixelFormatName = CCD_MODE_ITEM[i].name;
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CCD_MODE_ITEM[i].name %s", CCD_MODE_ITEM[i].name);
			}
		}
		size_t pos = pixelFormatName.find_last_of("_");
		pixelFormatName = pixelFormatName.substr(0, pos);
		PixelFormat format = pRPiCamera->GetPixelFormat();
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->GetPixelFormat() -> %s", format.toString().c_str());

		//   ROI calculation
		if (!m_controls.get(controls::ScalerCrop) && !m_controls.get(controls::rpi::ScalerCrops)) {
			const Rectangle sensor_area = pRPiCamera->GetControls().at(&controls::ScalerCrop).max().get<Rectangle>();
			const Rectangle default_crop = pRPiCamera->GetControls().at(&controls::ScalerCrop).def().get<Rectangle>();
			Rectangle crop;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Sensor area: %s", sensor_area.toString().c_str());
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Default crop: %s", default_crop.toString().c_str());

			unsigned int w = CCD_FRAME_WIDTH_ITEM->number.value;
			unsigned int h = CCD_FRAME_HEIGHT_ITEM->number.value;
			if ((left + w < CCD_INFO_WIDTH_ITEM->number.value) || (top + h < CCD_INFO_HEIGHT_ITEM->number.value)) {
				libcamera::Rectangle roi(left, top, left + w, top + h);
				m_controls.set(controls::ScalerCrop, roi);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ROI set to: %s", roi.toString().c_str());
			}
		}
		if (!m_controls.get(controls::AnalogueGain) && CCD_GAIN_ITEM->number.value) {
			m_controls.set(controls::AnalogueGainMode, controls::AnalogueGainModeManual);
			// Set manual analogue gain
			m_controls.set(controls::AnalogueGain, CCD_GAIN_ITEM->number.value);
		} else {
			// Enable auto gain
			m_controls.set(controls::AnalogueGainMode, controls::AnalogueGainModeAuto);
		}

		pRPiCamera->Set(m_controls);
		PixelFormat pixelFormatTarget =
			pixelFormatMap.find(pixelFormatName) != pixelFormatMap.end() ? pixelFormatMap.find(pixelFormatName)->second : formats::RGB888;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->StartExposure(width %d, height %d, format %s)", width, height, pixelFormatName.c_str());
		state = pRPiCamera->StartExposure(width, height, pixelFormatTarget);
	}
	if (state != -1) {
		indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback, &PRIVATE_DATA->exposure_timer);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure failed...pRPiCamera->StartExposure() returned -1");
	}
}

static void ccd_abort_exposure(indigo_device *device) {
	RPiCamera *pRPiCamera = PRIVATE_DATA->pRPiCamera;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "pRPiCamera->StopCamera()");
	pRPiCamera->StopCamera();
}

static void ccd_handle_advanced_property(indigo_device *device) {
	X_CCD_ADVANCED_PROPERTY->state = INDIGO_OK_STATE;
	if (X_CCD_ADVANCED_PROPERTY->count != 1) {
		RPiCamera *pRPicamera = PRIVATE_DATA->pRPiCamera;
		ControlList m_controls;
		auto controlsInfo = pRPicamera->GetControls();

		m_controls.set(controls::Contrast, X_CCD_CONTRAST_ITEM->number.value);
		m_controls.set(controls::Saturation, X_CCD_SATURATION_ITEM->number.value);
		m_controls.set(controls::Brightness, X_CCD_BRIGHTNESS_ITEM->number.value);

		pRPicamera->Set(m_controls);

		indigo_update_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
	}
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CONNECTION
		if (indigo_ignore_connection_change(device, property)) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			return INDIGO_OK;
		}
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		indigo_set_timer(device, 0, ccd_start_exposure, NULL);	
	} else if (indigo_property_match_changeable(RPI_AF_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// DSLR_AF
		indigo_property_copy_values(RPI_AF_PROPERTY, property, false);
		indigo_set_timer(device, 0, handle_af, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			indigo_set_timer(device, 0, ccd_abort_exposure, NULL);
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match_changeable(CCD_MODE_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_MODE
		indigo_property_copy_values(CCD_MODE_PROPERTY, property, false);
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			if (item->sw.value) {
				char *underscore = strchr(item->name, '_');
				unsigned binning = atoi(underscore + 1);
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.target = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = atoi(item->name + 3);
				CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
				CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = binning;
				CCD_BIN_VERTICAL_ITEM->number.target = CCD_BIN_VERTICAL_ITEM->number.value = binning;
				CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
				CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
				break;
			}
		}
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_BIN
		int prev_h_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int prev_v_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;

		if (prev_h_bin != horizontal_bin) {
			vertical_bin = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target =
				CCD_BIN_VERTICAL_ITEM->number.value = horizontal_bin;
		} else if (prev_v_bin != vertical_bin) {
			horizontal_bin = CCD_BIN_HORIZONTAL_ITEM->number.target = CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.target =
				CCD_BIN_VERTICAL_ITEM->number.value = vertical_bin;
		}
		char *selected_name = CCD_MODE_PROPERTY->items[0].name;
		for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[k];
			if (item->sw.value) {
				selected_name = item->name;
				break;
			}
		}
		for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[k];
			char *underscore = strchr(item->name, '_');
			unsigned bin = atoi(underscore + 1);
			if (bin == horizontal_bin && !strncmp(item->name, selected_name, 5)) {
				indigo_set_switch(CCD_MODE_PROPERTY, item, true);
				CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
				CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
				return INDIGO_OK;
			}
		}
		CCD_MODE_PROPERTY->state = INDIGO_ALERT_STATE;
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_FRAME_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		char name[INDIGO_NAME_SIZE];

		for (int j = 0; j < CCD_MODE_PROPERTY->count; j++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[j];
			if (item->sw.value) {
				strcpy(name, item->name);
				sprintf(name + 3, "%02d", (int)(CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value));
				name[5] = '_';
				for (int k = 0; k < CCD_MODE_PROPERTY->count; k++) {
					item = &CCD_MODE_PROPERTY->items[k];
					if (!strcmp(name, item->name)) {
						indigo_set_switch(CCD_MODE_PROPERTY, item, true);
						CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
						return indigo_ccd_change_property(device, client, property);
					}
				}
			}
		}
		CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_GAIN_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// CCD_GAIN
		indigo_property_copy_values(CCD_GAIN_PROPERTY, property, false);
		if (IS_CONNECTED && !CCD_GAIN_PROPERTY->hidden) {
			indigo_update_property(device, CCD_GAIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	} else if (X_CCD_ADVANCED_PROPERTY && indigo_property_match_defined(X_CCD_ADVANCED_PROPERTY, property)) {
		// --------------------------------------------------------------------------------
		// X_CCD_ADVANCED
		indigo_property_copy_values(X_CCD_ADVANCED_PROPERTY, property, false);
		indigo_set_timer(device, 0, ccd_handle_advanced_property, NULL);
		
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);

		if (X_CCD_ADVANCED_PROPERTY) {
			indigo_release_property(X_CCD_ADVANCED_PROPERTY);
		}
	}
	indigo_release_property(RPI_AF_PROPERTY);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#define MAX_DEVICES 10

static indigo_device *devices[MAX_DEVICES];
static std::string new_eid;

bool IsNew(std::string eid) {
	for (int i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device && PRIVATE_DATA->eid == eid) {
			return false;
		}
	}
	return true;
}

std::string GetCameraName(std::string new_eid) {

	size_t lastSlash = new_eid.find_last_of('/');
	// Extract the word after the last slash
	std::string lastWord = (lastSlash != std::string::npos) ? new_eid.substr(lastSlash + 1) : new_eid;

	return lastWord;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	if (IS_CONNECTED) {

		indigo_define_matching_property(RPI_AF_PROPERTY);

		if (X_CCD_ADVANCED_PROPERTY && indigo_property_match(X_CCD_ADVANCED_PROPERTY, property)) {
			indigo_define_property(device, X_CCD_ADVANCED_PROPERTY, NULL);
		}
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static void attach_rpi_ccd() {

	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER("", ccd_attach, ccd_enumerate_properties, ccd_change_property, NULL, ccd_detach);

	static indigo_device focuser_template =
		INDIGO_DEVICE_INITIALIZER("", focuser_attach, indigo_focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

	new_eid = "-1";
	std::lock_guard<std::mutex> lock(device_mutex);
	RegisterCameraManager *cameraManager_ = RegisterCameraManager::getInstance();

	for (std::string id : cameraManager_->GetCameraIDs()) {

		string cameraName = GetCameraName(id);
		if (IsNew(id)) {
			char name[128] = "RPI Camera ";
			strcat(name, cameraName.c_str());

			char *end = name + strlen(name) - 1;
			while (end > name && isspace((unsigned char)*end))
				end--;
			end[1] = '\0';
			rpi_private_data *private_data = (rpi_private_data *)indigo_safe_malloc(sizeof(rpi_private_data));
			private_data->eid = id;

			indigo_device *device = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			indigo_device *master_device = device;
			device->master_device = master_device;
			snprintf(device->name, INDIGO_NAME_SIZE, "%s", name);
			indigo_make_name_unique(device->name, "%s", cameraName.c_str());
			device->private_data = private_data;

			indigo_device *focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->master_device = device;
			snprintf(focuser->name, INDIGO_NAME_SIZE, "%s (focuser)", name);
			indigo_make_name_unique(device->name, "%s", cameraName.c_str());
			focuser->private_data = private_data;
			private_data->focuser = focuser;

			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
		}
	}
}

indigo_result indigo_ccd_rpi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Raspberry Pi Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		// Match the SDK log level to indigo log level
		SetSDKLogLevel();
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		// CSI cameras use dedicated ports which
		// is not supported by libusb. So no hot-plug
		// support. Directly attach the camera divice and
		// let camera module list the attached sensors
		attach_rpi_ccd();
		return INDIGO_OK;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;

		for (int i = MAX_DEVICES - 1; i >= 0; i--) {
			indigo_device *device = devices[i];
			if (device) {
				indigo_detach_device(device);
				if (device->master_device == device) {
					rpi_private_data *private_data = PRIVATE_DATA;
					if (private_data->buffer != NULL) {
						free(private_data->buffer);
					}
					if (PRIVATE_DATA->focuser) {
						indigo_detach_device(PRIVATE_DATA->focuser);
						free(PRIVATE_DATA->focuser);
						PRIVATE_DATA->focuser = nullptr;
					}
					if (private_data->pRPiCamera != nullptr) {
						delete private_data->pRPiCamera;
						private_data->pRPiCamera = nullptr;
					}
					free(private_data);
				}
				free(device);
				devices[i] = NULL;
			}
		}
		// Reset the SDK log level to default
		ResetSDKLogLevel();
		break;
	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}

#else

indigo_result indigo_ccd_rpi(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Raspberry Pi Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	switch (action) {
	case INDIGO_DRIVER_INIT:
	case INDIGO_DRIVER_SHUTDOWN:
		return INDIGO_UNSUPPORTED_ARCH;
	case INDIGO_DRIVER_INFO:
		break;
	}
	return INDIGO_OK;
}
#endif