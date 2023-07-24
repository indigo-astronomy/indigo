#pragma once

/// @file AtikCameras.h
///	@brief Atik SDK C interface header

#ifdef _WIN32
#include <comdef.h>
#else
#include <stddef.h>
/// Typedef for non windows builds
typedef bool BOOL;
/// Typedef for non windows builds
typedef void * HINSTANCE;
#endif

#include "AtikDefs.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

	/// Main error enum for methods with "int" as return type.
	enum ARTEMISERROR
	{
		/// Operation successful
		ARTEMIS_OK = 0,
		/// The camera handle passed is not a valid handle.
		/// @see ArtemisConnect()
		ARTEMIS_INVALID_PARAMETER,
		/// Camera is not connected
		ARTEMIS_NOT_CONNECTED,
		/// Not impl
		ARTEMIS_NOT_IMPLEMENTED,
		/// No response
		ARTEMIS_NO_RESPONSE,
		/// Invalid function
		ARTEMIS_INVALID_FUNCTION,
		/// Camera Not init
		ARTEMIS_NOT_INITIALIZED,
		/// Failed
		ARTEMIS_OPERATION_FAILED,
	};

	/// Camera colour properties
	enum ARTEMISCOLOURTYPE
	{
		/// Either the device is not a camera or the colour cannot be determined
		ARTEMIS_COLOUR_UNKNOWN = 0,
		/// Device sensor is monochrome
		ARTEMIS_COLOUR_NONE,
		/// Device sensor is colour (RGGB)
		ARTEMIS_COLOUR_RGGB
	};

	/// @brief 
	enum ARTEMISPRECHARGEMODE
	{
		/// Precharge ignored
		PRECHARGE_NONE = 0,		
		/// In-camera precharge subtraction
		PRECHARGE_ICPS,			
		/// Precharge sent with image data
		PRECHARGE_FULL,			
	};

	/// @see ArtemisCameraState()
	enum ARTEMISCAMERASTATE
	{
		CAMERA_ERROR = -1,
		CAMERA_IDLE = 0,
		CAMERA_WAITING,
		CAMERA_EXPOSING,
		CAMERA_READING,
		CAMERA_DOWNLOADING,
		CAMERA_FLUSHING,
	};

	// @see ArtemisCameraConnectionState
	enum ARTEMISCONNECTIONSTATE
	{
		CAMERA_CONNECTING      = 1,
		CAMERA_CONNECTED       = 2,
		CAMERA_CONNECT_FAILED  = 3,
		CAMERA_SUSPENDED       = 4,
		CAMERA_CONNECT_UNKNOWN = 5
	};

	/// Flags for ArtemisGet/SetProcessing
	/// @see ArtemisGetProcessing(), ArtemisSetProcessing()
	enum ARTEMISPROCESSING
	{
		/// compensate for JFET nonlinearity
		ARTEMIS_PROCESS_LINEARISE = 1,	
		/// adjust for 'Venetian Blind effect'
		ARTEMIS_PROCESS_VBE = 2, 
	};

	/// @brief Index into the ccdflags value of ARTEMISPROPERTIES
	/// @see ARTEMISPROPERTIES
	enum ARTEMISPROPERTIESCCDFLAGS
	{
		/// CCD is interlaced type
		ARTEMIS_PROPERTIES_CCDFLAGS_INTERLACED = 1, 
		/// Enum padding to 4 bytes. Not used
		ARTEMIS_PROPERTIES_CCDFLAGS_DUMMY = 0x7FFFFFFF 
	};

	/// Index into the camera flags of ARTEMISPROPERTIES
	/// @see ARTEMISPROPERTIES
	enum ARTEMISPROPERTIESCAMERAFLAGS
	{
		/// Camera has readout FIFO fitted
		ARTEMIS_PROPERTIES_CAMERAFLAGS_FIFO = 1, 
		/// Camera has external trigger capabilities
		ARTEMIS_PROPERTIES_CAMERAFLAGS_EXT_TRIGGER = 2, 
		/// Camera can return preview data
		ARTEMIS_PROPERTIES_CAMERAFLAGS_PREVIEW = 4, 
		/// Camera can return subsampled data
		ARTEMIS_PROPERTIES_CAMERAFLAGS_SUBSAMPLE = 8, 
		/// Camera has a mechanical shutter
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_SHUTTER = 16, 
		/// Camera has a guide port
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GUIDE_PORT = 32, 
		/// Camera has GPIO capability
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_GPIO = 64, 
		/// Camera has a window heater
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_WINDOW_HEATER = 128, 
		/// Camera can download 8-bit images
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_EIGHT_BIT_MODE = 256, 
		/// Camera can overlap
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_OVERLAP_MODE = 512, 
		/// Camera has internal filterwheel
		ARTEMIS_PROPERTIES_CAMERAFLAGS_HAS_FILTERWHEEL = 1024, 
		/// Enum padding to 4 bytes. Not used
		ARTEMIS_PROPERTIES_CAMERAFLAGS_DUMMY = 0x7FFFFFFF 
	};

	/// @brief Index into ArtemisCoolingInfo() Flags
	/// @see ArtemisCoolingInfo()
	enum ARTEMISCOOLINGINFO
	{
		/// Camera can be cooled. 0= No cooling ability 1= Has cooling
		ARTEMIS_COOLING_INFO_HASCOOLING          = 1,
		/// Cooling is always on or can be controlled. 0= Always on 1= Controllable
		ARTEMIS_COOLING_INFO_CONTROLLABLE        = 2,
		/// Cooling can be switched On/Off. 0= On/Off control not available 1= On/Off control available
		ARTEMIS_COOLING_INFO_ONOFFCOOLINGCONTROL = 4,
		/// Cooling can be set via ArtemisSetCoolingPower()
		ARTEMIS_COOLING_INFO_POWERLEVELCONTROL   = 8,
		/// Cooling can be set via ArtemisSetCooling()
		ARTEMIS_COOLING_INFO_SETPOINTCONTROL     = 16,
		/// Currently warming up. 0= Normal control 1= Warming Up
		ARTEMIS_COOLING_INFO_WARMINGUP           = 32,
		/// Currently cooling. 0= Cooling off 1= Cooling on
		ARTEMIS_COOLING_INFO_COOLINGON           = 64,
		/// Currently under setpoint control 0= No set point control 1= Set point control
		ARTEMIS_COOLING_INFO_SETPOINTCONTROLON   = 128
	};

	/// @brief Filter wheel type. 
	/// @note An EFW3 will show as EFW2 as they use the same firmware
	/// @see ArtemisEFWGetDeviceDetails()
	enum ARTEMISEFWTYPE
	{
		ARTEMIS_EFW1 = 1,
		ARTEMIS_EFW2 = 2
	};

	/// @brief ID's for the camera specific options
	/// @see ArtemisHasCameraSpecificOption()
	enum CameraSpecificOptionsIDs
	{
		ID_GOPresetMode     = 1,
		ID_GOPresetLow      = 2,
		ID_GOPresetMed      = 3,
		ID_GOPresetHigh     = 4,
		ID_GOCustomGain     = 5,
		ID_GOCustomOffset   = 6,
		ID_EvenIllumination = 12,
		ID_PadData          = 13,
		ID_ExposureSpeed    = 14,
		ID_BitSendMode      = 15, //ACIS specific
		ID_16BitMode        = 18, //ChemiMOS specific
		ID_FX3Version       = 200,
		ID_FPGAVersion      = 201,
	};

	/// Return type for ArtemisProperties
	/// @see ArtemisProperties()
	struct ARTEMISPROPERTIES
	{
		/// Firmware version
		int Protocol;
		/// X resolution
		int nPixelsX;
		/// Y resolution
		int nPixelsY;
		/// Physical size of each pixel in microns, horizontally
		float PixelMicronsX;
		/// Physical size of each pixel in microns, vertically
		float PixelMicronsY;
		/// CCD flags
		/// @see ARTEMISPROPERTIESCCDFLAGS
		int ccdflags;
		/// Camera flags
		/// @see ARTEMISPROPERTIESCAMERAFLAGS
		int cameraflags;
		/// Model of the device
		char Description[40];
		/// Manufacturer of device
		char Manufacturer[40];
	};

	/// Atik SDK handle type
	/// @see ArtemisConnect(), ArtemisEFWConnect()
	typedef void * ArtemisHandle;

	/// @brief DLL handle set by ArtemisLoadDLL(). 
	/// This is irrelevant if you are linking at compile time
	extern HINSTANCE hArtemisDLL;

	//////////////////////////////////////////////////////////////////////////
	//
	// Interface functions for Atik Cameras Camera Library
	//


	/// @brief Function declaration linkage is implicitly extern
	/// This definition and further usage has been left in for legacy purposes
	#define artfn extern

	// -------------------  DLL --------------------------
		
	/// @brief Get API version. This may be the same as the DLL version.
	/// @return API version as an integer, such as: 20200904 
	artfn int  ArtemisAPIVersion(void);

	/// @brief Get DLL version. This may be the same as the API version.
	/// @return API version as an integer, such as: 20200904 
	artfn int  ArtemisDLLVersion(void);

	/// @brief Gets whether the connection to the camera is local.
	/// @return True if the connection is local (E.G. through an USB cable), false otherwise
	artfn BOOL ArtemisIsLocalConnection(void);

	/// @brief Allows debug output to be output to standard error.
	/// @param value Whether to enable the output.
	/// @see ArtemisAllowDebugCallback(), ArtemisAllowDebugCallbackContext()
	artfn void ArtemisAllowDebugToConsole(bool value);

	/// @brief Provide a pointer to a function, which will be invoked when debug output is produced.
	/// This can be used in combination with ArtemisAllowDebugToConsole.
	/// @param callback A function pointer to a compatible log function. 
	/// @see ArtemisAllowDebugToConsole, ArtemisAllowDebugCallbackContext()
	artfn void ArtemisSetDebugCallback(void(*callback)(const char *message));

	/// @brief Same as ArtemisSetDebugCallback(), but allows to pass an additional void pointer.
	/// This can be used to store a user-allocated data structure which will be passed to the callback.
	/// This pointer needs to be allocated and managed by the user. The context will never be freed by the API.
	/// @param context The user-allocated pointer.
	/// @param callback A function pointer to a compatible log function
	/// @see ArtemisSetDebugCallback(), ArtemisAllowDebugToConsole()
	artfn void ArtemisSetDebugCallbackContext(void * context, void(*callback)(void * context, const char *message));

	/// @brief Internal function used during flashing.
	/// @param firmwareDir The firmware directory as null terminated string.
	artfn void ArtemisSetFirmwareDir(const char * firmwareDir);

	/// @brief Connect to an AtikAir instance.
	/// @param host The hostname or ip address of the instance, E.G. "192.168.0.1"
	/// @param port the port of the instance.
	artfn void ArtemisSetAtikAir(const char * host, int port);

	/// @brief Deallocates all internal DLL structures.
	/// The SDK functions may not be called after calling this function.
	artfn void ArtemisShutdown(void);

	// -------------------  Device --------------------------
	
	/// @brief Returns the number of connected and recognised devices.
	/// The count does not include misconfigured devices (E.G. if drivers are missing).
	/// @return The number of connected and recognised devices.
	artfn int			ArtemisDeviceCount(void);

	/// @brief Duplicate of ArtemisDevicePresent().
	/// @param iDevice the device index.
	/// @return TRUE if the device is present, FALSE otherwise 
	/// @see ArtemisDevicePresent()
	artfn BOOL			ArtemisDeviceIsPresent(int iDevice);

	/// @brief checks if the device at the index is connected.
	/// @param iDevice the device index.
	/// @return TRUE if the device is present, FALSE otherwise 
	artfn BOOL			ArtemisDevicePresent(  int iDevice);

	/// @brief Checks if the device has already been connected to.
	/// @param iDevice the device index.
	/// @return TRUE if the device is has a handle acquired to it, FALSE otherwise 
	artfn BOOL			ArtemisDeviceInUse(    int iDevice);

	/// @brief Retrieves the device's printable name.
	/// @param iDevice the device index.
	/// @param pName a pointer to a user-allocated char array of length 100.
	/// @return TRUE if the name was set successfully, FALSE if not.
	artfn BOOL			ArtemisDeviceName(          int iDevice, char *pName);

	/// @brief Retrieves the device's serial number.
	/// @param iDevice the device index.
	/// @param pSerial a pointer to a user-allocated char array of length 100.
	/// @return TRUE if the name was set successfully, FALSE if not.
	artfn BOOL			ArtemisDeviceSerial(        int iDevice, char *pSerial);

	/// @brief Return whether the device at the specified index is a camera (and not E.G. a filter wheel device).
	/// @param iDevice the device index.
	/// @return TRUE if the device is a camera, FALSE otherwise
	artfn BOOL			ArtemisDeviceIsCamera(      int iDevice);

	/// @brief Return whether the device at the specified index has a filter wheel device.
	/// @param iDevice the device index.
	/// @return TRUE if the device has a filter wheel device, FALSE otherwise
	artfn BOOL			ArtemisDeviceHasFilterWheel(int iDevice);

	/// @brief Return whether the device at the specified index has a guide port.
	/// @param iDevice the device index.
	/// @return TRUE if the device has a guide port, FALSE otherwise
	artfn BOOL			ArtemisDeviceHasGuidePort(  int iDevice);

	/// Connect to an Atik device by index and obtain an ArtemisHandle to it for further use.
	/// Use ArtemisDeviceCount to discover how many devices are available for connection.
	/// @param iDevice Device index. Use 0 if using a single camera.
	/// @return Non-null ArtemisHandle if successful, null on failure.
	/// @see ArtemisDeviceCount()
	artfn ArtemisHandle	ArtemisConnect(    int iDevice);

	/// @brief Returns whether the handle is currently connected to a camera
	/// @param handle The device handle
	/// @return TRUE if the handle is connected, FALSE otherwise
	artfn BOOL			ArtemisIsConnected(ArtemisHandle handle);

	/// @brief Disconnects from the device with that handle and invalidates the handle.
	/// @param handle The device handle
	/// @return TRUE if the handle disconnection was successful, FALSE otherwise
	/// @see ArtemisConnect()
	artfn BOOL			ArtemisDisconnect( ArtemisHandle handle);

	/// @brief Updates the available device count.
	/// @return 
	/// @see ArtemisDeviceCount()
	artfn int			ArtemisRefreshDevicesCount(void);

#ifndef WIN32
		int  			ArtemisDeviceGetLibUSBDevice(int iDevice, libusb_device ** device);
#endif

	// ------------------- Camera Info -----------------------------------
	
	/// @brief Retrieves the serial number of the connected device.
	/// @param handle the connected device handle.
	/// @param flags a pointer to an integer which will be set to internal device flags.
	/// @param serial a pointer to an integer which will be set to the serial number of the connected Atik device.
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure.
	artfn int ArtemisCameraSerial(ArtemisHandle handle, int* flags, int* serial);

	/// @brief Retrieves the colour properties of the the connected device.
	/// @param handle the connected device handle.
	/// @param colourType a pointer to a user provided ARTEMISCOLOURTYPE enum which will be set to an ARTEMISCOLOURTYPE value.
	/// @param normalOffsetX a pointer to an integer which will be set to the normal offset over the X axis.
	/// @param normalOffsetY a pointer to an integer which will be set to the normal offset over the Y axis.
	/// @param previewOffsetX a pointer to an integer which will be set to the preview offset over the X axis.
	/// @param previewOffsetY a pointer to an integer which will be set to the preview offset over the Y axis.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisColourProperties(ArtemisHandle handle, enum ARTEMISCOLOURTYPE *colourType, int *normalOffsetX, int *normalOffsetY, int *previewOffsetX, int *previewOffsetY);

	/// Gets the connected camera's physical properties.
	/// @param handle The connected camera's handle.
	/// @param pProp if successful, a pointer to a user provided ARTEMISPROPERTIES instance.
	/// @see ARTEMISERROR, ARTEMISPROPERTIES
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisProperties(		   ArtemisHandle handle, struct ARTEMISPROPERTIES *pProp);

	/// @brief Retrieves the camera connection state.
	/// @param handle the connected Atik device handle.
	/// @param state if successful, a pointer to a user provided ARTEMISCONNECTIONSTATE instance.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCameraConnectionState(ArtemisHandle handle, enum ARTEMISCONNECTIONSTATE * state);

	// ------------------- Exposure Settings -----------------------------------

	/// @brief Sets the binning for the device.
	/// This will cause the resolution of the captured image to change accordingly with the requested bin values.
	/// @param handle the connected Atik device handle.
	/// @param x the X binning value.
	/// @param y the Y binning value.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisBin(								ArtemisHandle handle, int  x, int  y);

	/// @brief Gets the binning for the device.
	/// @param handle the connected Atik device handle.
	/// @param x pointer to an integer which will be set to the X binning value.
	/// @param y pointer to an integer which will be set to the Y binning value.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisGetBin(							ArtemisHandle handle, int *x, int *y);

	/// @brief Retrieves the maximum binning supported by the device.
	/// @param handle the connected Atik device handle.
	/// @param x pointer to an integer which will be set to the maximum supported X binning value.
	/// @param y pointer to an integer which will be set to the maximum supported Y binning value.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisGetMaxBin(						ArtemisHandle handle, int *x, int *y);

	/// @brief Retrieves the current subframing setting for the device.
	/// @param handle the connected Atik device handle.
	/// @param x pointer to an integer which will be set to the current subframing X offset.
	/// @param y pointer to an integer which will be set to the current subframing Y offset.
	/// @param w pointer to an integer which will be set to the current subframing width.
	/// @param h pointer to an integer which will be set to the current subframing height.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisGetSubframe(						ArtemisHandle handle, int *x, int *y, int *w, int *h);

	/// @brief Set the device's subframe position and size.
	/// This is equivalent to calling ArtemisSubframePos() followed by ArtemisSubframeSize().
	/// @param handle the connected camera's handle
	/// @param x the X offset of the subframe region.
	/// @param y the Y offset of the subframe region.
	/// @param w the width of the subframe region.
	/// @param h the height of the subframe region.
	/// @see ARTEMISERROR, ArtemisSubframePos(), ArtemisSubframeSize()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSubframe(							ArtemisHandle handle, int  x, int  y, int  w, int  h);

	/// @brief Sets the device's subframe position.
	/// @param handle the connected Atik device handle.
	/// @param x the X offset of the subframe region.
	/// @param y the Y offset of the subframe region.
	/// @see ARTEMISERROR, ArtemisSubframe()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSubframePos(						ArtemisHandle handle, int  x, int y);

	/// @brief Sets the device's subframe width and height.
	/// @param handle the connected Atik device handle.
	/// @param w the width of the subframe region.
	/// @param h the height of the subframe region.
	/// @see ARTEMISERROR, ArtemisSubframe()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSubframeSize(						ArtemisHandle handle, int  w, int h);

	/// @brief Set whether subsampling mode is enabled on the device.
	/// @param handle the connected Atik device handle.
	/// @param bSub 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetSubSample(						ArtemisHandle handle, bool bSub);

	/// @brief Retrieves whether continuous exposing is supported by the device. Only relevant
	/// for our Titan Camera.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if supported, FALSE if not.
	artfn BOOL ArtemisContinuousExposingModeSupported(	ArtemisHandle handle);

	/// @brief Retrieves whether continuous exposing is enabled for the device. Only relevant
	/// for our Titan Camera.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if continuous exposing mode is enabled, FALSE otherwise. 
	artfn BOOL ArtemisGetContinuousExposingMode(		ArtemisHandle handle);

	/// @brief Set whether continuous exposing mode is enabled.Only relevant for our Titan Camera.
	/// This only has an effect on supported devices.
	/// @param handle the connected Atik device handle.
	/// @param bEnable 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetContinuousExposingMode(		ArtemisHandle handle, bool bEnable);

	/// @brief Retrieves whether dark mode is enabled for the device.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if continuous exposing mode is enabled, FALSE otherwise. 
	artfn BOOL ArtemisGetDarkMode(						ArtemisHandle handle);

	/// @brief Sets whether dark mode is enabled for the device.
	/// @param handle the connected Atik device handle.
	/// @param bEnable whether to enable or disable dark mode.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetDarkMode(						ArtemisHandle handle, bool bEnable);

	/// @brief Sets whether preview mode is enabled for the device.
	/// If preview mode is enabled, the sensor is not cleared between exposures.
	/// Using preview mode, there might be more noise/glow in the resulting image.
	/// @param handle the connected Atik device handle.
	/// @param bPrev whether to enable or disable preview mode.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetPreview(						ArtemisHandle handle, bool bPrev);

	/// @brief Sets whether black auto-adjustment is enabled.
	/// @param handle the connected Atik device handle.
	/// @param bEnable whether black levels will be auto-adjusted.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisAutoAdjustBlackLevel(				ArtemisHandle handle, bool bEnable);

	/// @brief Sets the precharge mode of the camera.
	/// Precharge applies an in-camera offset, mainly for astronomy use. 
	/// @param handle the connected Atik device handle.
	/// @param mode ARTEMISPRECHARGEMODE enumeration value
	/// @see ARTEMISPRECHARGEMODE, ARTEMISERROR
	/// @returns ARTEMIS_OK on success, other ARTEMISERROR on error
	artfn int  ArtemisPrechargeMode(					ArtemisHandle handle, int mode);

	/// @brief Sets whether 8-bit mode is enabled on the device.
	/// This affects the size of the returned image buffer, which by default is 16 bits per pixel.
	/// @param handle the connected Atik device handle.
	/// @param eightbit whether to enable 8-bit mode.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisEightBitMode(						ArtemisHandle handle, bool eightbit);

	/// @brief Retrieves whether 8-bit mode is enabled on the device.
	/// @param handle the connected Atik device handle.
	/// @param eightbit pointer to a boolean, which will be set to whether the 8-bit mode is enabled.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisGetEightBitMode(					ArtemisHandle handle, bool *eightbit);

	/// @brief Begin an overlapped exposure.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisStartOverlappedExposure(			ArtemisHandle handle);

	/// @brief Returns whether the overlapped exposure is still valid.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if valid, FALSE otherwise.
	artfn BOOL ArtemisOverlappedExposureValid(			ArtemisHandle handle);

	/// @brief Set the overlapped exposure time.
	/// @param handle the connected Atik device handle.
	/// @param fSeconds the time as a floating point number of seconds.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetOverlappedExposureTime(		ArtemisHandle handle, float fSeconds);

	/// @brief Sets whether the device will await a triggered exposure
	/// @param handle the connected Atik device handle.
	/// @param bAwaitTrigger whether to await a triggered exposure
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisTriggeredExposure(				ArtemisHandle handle, bool bAwaitTrigger);

	/// @brief Return which post processing flags are enabled for the device.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISPROCESSING
	/// @return ARTEMISPROCESSING enumeration of flags enabled for the device.
	artfn int  ArtemisGetProcessing(					ArtemisHandle handle);

	/// @brief Sets which post processing effects are enabled for the device.
	/// @param handle the connected Atik device handle.
	/// @param options ARTEMISPROCESSING enumeration of post processing flags to enable for the device.
	/// @see ARTEMISERROR, ARTEMISPROCESSING
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetProcessing(					ArtemisHandle handle, int options);

	/// @brief Provides an exposure length where by the passed adu value and values below will
	/// occupy the percentile passed. This is a blocking call and will return 
	/// with an ARTEMIS_OPERATION_FAILED after 20 iterations if the parameters can not be met.
	/// @param handle the connected Atik device handle
	/// @param percentile the percentage of pixels which the passed adu value and below occupy
	/// @param adu the adu value to
	/// @param exposureLength the exposure length that satisfies the passed percentile/adu combination
	/// @param startingExposureLength the exposure length to start the calculation at
	/// @return ARTEMIS_OK on success, or one of the ARTEMISERROR enumerations on failure
	artfn int ArtemisAutoExposureLength(ArtemisHandle handle, int percentile, unsigned short adu, 
										float* exposureLength, float startingExposureLength);

	// ------------------- Exposures -----------------------------------

	/// @brief Begin an exposure for the device, specifying a duration as floating point seconds.
	/// @param handle the connected Atik device handle.
	/// @param seconds the seconds to perform the exposure for, as a floating point number.
	/// @see ARTEMISERROR, ArtemisStartExposureMS(), ArtemisAbortExposure()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisStartExposure(			 ArtemisHandle handle, float seconds);

	/// @brief Begin an exposure for the device, specifying a duration as milliseconds.
	/// @param handle the connected Atik device handle.
	/// @param ms the exposure duration in milliseconds.
	/// @see ARTEMISERROR, ArtemisStartExposure(), ArtemisAbortExposure()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisStartExposureMS(			 ArtemisHandle handle, int ms);

	/// @brief Stop the current exposure for the device.
	/// This is analogous to ArtemisStopExposure().
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR, ArtemisStartExposure(), ArtemisStartExposureMS(), ArtemisStopExposure()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisAbortExposure(			 ArtemisHandle handle);

	/// @brief Stop the current exposure for the device.
	/// This is analogous to ArtemisAbortExposure().
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR, ArtemisStartExposure(), ArtemisStartExposureMS(), ArtemisAbortExposure()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisStopExposure(			 ArtemisHandle handle);

	/// @brief Returns whether the image has been fully downloaded and is ready to access.
	/// This method can be polled until it returns TRUE, after which ArtemisGetImageData() and ArtemisImageBuffer() can be called
	/// to get the results of the exposure.
	/// It is recommended to add a delay in between polling (E.G. 10 milliseconds).
	/// @param handle the connected Atik device handle.
	/// @return TRUE if the image is ready, FALSE on failure
	artfn BOOL  ArtemisImageReady(				 ArtemisHandle handle);

	/// @brief Returns the device's state as an ARTEMISCAMERASTATE enumeration.
	/// @param handle the connected device handle.
	/// @see ARTEMISERROR, ARTEMISCAMERASTATE
	/// @return ARTEMISCAMERASTATE enumeration value.
	artfn int   ArtemisCameraState(				 ArtemisHandle handle);

	/// @brief Returns how much time is left in the exposure as a floating point number.
	/// Note this does not include the download time.
	/// @param handle the connected device handle.
	/// @see ArtemisImageReady(), ArtemisStartExposure()
	/// @return the number of seconds remaining in the exposure as a floating point number.
	artfn float ArtemisExposureTimeRemaining(	 ArtemisHandle handle);

	/// @brief Returns the download progress in percent.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return the download progress in percent (0-100).
	artfn int   ArtemisDownloadPercent(			 ArtemisHandle handle);

	/// @brief Provides information about the latest acquired image.
	/// @param handle the connected Atik device handle.
	/// @param x pointer to an integer which will be set to the subframe size in pixels, on the X axis.
	/// @param y pointer to an integer which will be set to the subframe size in pixels, on the Y axis.
	/// @param w pointer to an integer which will be set to the width of the image in pixels.
	/// @param h pointer to an integer which will be set to the height of the image in pixels.
	/// @param binx pointer to an integer which will be set to the horizontal binning used.
	/// @param biny pointer to an integer which will be set to the vertical binning used.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisGetImageData(			 ArtemisHandle handle, int *x, int *y, int *w, int *h, int *binx, int *biny);

	/// @brief Returns a pointer to the internal image buffer which contains the latest captured image.
	/// The buffer size can be calculated by width * height * pixel depth.
	/// @param handle the connected Atik device handle.
	/// @see ArtemisGetImageData()
	/// @return a pointer to the internal image buffer
	artfn void* ArtemisImageBuffer(				 ArtemisHandle handle);

	/// @brief Returns the duration of the last exposure as floating point number of seconds.
	/// @param handle the connected Atik device handle.
	/// @return the duration of the last exposure as floating point number of seconds.
	artfn float ArtemisLastExposureDuration(	 ArtemisHandle handle);

	/// @brief Returns a pointer to the formatted last exposure start time.
	/// The buffer is internal to the SDK and is overwritten every time this function is called.
	/// Does not include milliseconds.
	/// @param handle the connected Atik device handle.
	/// @see ArtemisLastStartTimeMilliseconds()
	/// @return pointer to a null terminated buffer containing the formatted time exposure was started at.
	artfn char* ArtemisLastStartTime(			 ArtemisHandle handle);

	/// @brief Returns the last exposure start time millisecond component.
	/// @param handle the connected Atik device handle.
	/// @see ArtemisLastStartTime
	/// @return millisecond component of the last exposure time.
	artfn int   ArtemisLastStartTimeMilliseconds(ArtemisHandle handle);

	/// @brief This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int   ArtemisClearVReg(				 ArtemisHandle handle);
	
	/// @brief Gets whether the device supports fast mode.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if fast mode is supported, FALSE if not
	artfn BOOL ArtemisHasFastMode(      ArtemisHandle handle);

	/// @brief Begins a fast mode exposure.
	/// @param handle the connected Atik device handle.
	/// @param ms the exposure duration in milliseconds.
	/// @return TRUE if started successfully, FALSE if not.
	artfn BOOL ArtemisStartFastExposure(ArtemisHandle handle, int ms);

	/// @brief Set the callback that will be invoked when a fast mode exposure is completed.
	/// @param handle the connected Atik device handle.
	/// @param callback a pointer to a function which will be invoked when fast mode is completed.
	/// @return TRUE on success, FALSE on failure.
	artfn BOOL ArtemisSetFastCallback(  ArtemisHandle handle, void(*callback)(ArtemisHandle handle, int x, int y, int w, int h, int binx, int biny, void * imageBuffer));

	/// @brief Set the callback that will be invoked when a fast mode exposure is completed. This extension provides
	/// a pointer to extra info passed to the function. See AtikDefs.h for a description of the structure
	/// passed via the info parameter. Cast the unsigned char pointer to a FastCallbackInfo pointer to
	/// access the information.
	/// 
	/// @param handle the connected Atik device handle.
	/// @param callback a pointer to a function which will be invoked when fast mode is completed.
	/// @return TRUE on success, FALSE on failure.
	artfn BOOL ArtemisSetFastCallbackEx(ArtemisHandle handle, void(*callback)(ArtemisHandle handle, int x, int y, int w, int h, int binx, int biny, void * imageBuffer, unsigned char * info));

	// ------------------- Amplifier -----------------------------------

	/// @brief Enable/disable the device's amplifier.
	/// This function is equivalent to ArtemisSetAmplifierSwitched().
	/// @param handle the connected Atik device handle.
	/// @param bOn whether to enable or disable the built in amplifier.
	/// @see ARTEMISERROR, ArtemisSetAmplifierSwitched()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisAmplifier(		   ArtemisHandle handle, bool bOn);

	/// @brief Returns whether the amplifier is enabled.
	/// @param handle the connected Atik device handle.
	/// @return TRUE if the amplifier is enabled, FALSE otherwise.
	artfn BOOL ArtemisGetAmplifierSwitched(ArtemisHandle handle);

	/// @brief Enable/disable the device's amplifier.
	/// This function is equivalent to ArtemisAmplifier().
	/// @param handle the connected Atik device handle.
	/// @param bSwitched whether to enable switching amplifier or not
	/// @see ARTEMISERROR, ArtemisAmplifier()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisSetAmplifierSwitched(ArtemisHandle handle, bool bSwitched);
		
	// ------------ Camera Specific Options -------------

	/// @brief Returns whether the specified option is available
	/// @param handle the connected Atik device handle.
	/// @param id the camera specific option @see CameraSpecificOptionsIDs
	/// @return true if supported, false if not.
	artfn bool ArtemisHasCameraSpecificOption(    ArtemisHandle handle, unsigned short id);

	/// @brief Used to get the specified option's current value. Please check that the current camera has this option using ArtemisHasCameraSpecificOption()
	/// @param handle the connected Atik device handle.
	/// @param id 
	/// @param data 
	/// @param dataLength 
	/// @param actualLength 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_PARAM if the opton is not available or ARTEMISERROR on failure
	artfn int  ArtemisCameraSpecificOptionGetData(ArtemisHandle handle, unsigned short id, unsigned char * data, int dataLength, int * actualLength);

	/// @brief Used to set the specified option's value. Please check that the current camera has this option using ArtemisHasCameraSpecificOption()
	/// @param handle the connected Atik device handle.
	/// @param id 
	/// @param data 
	/// @param dataLength 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_PARAM if the opton is not available or ARTEMISERROR on failure
	artfn int  ArtemisCameraSpecificOptionSetData(ArtemisHandle handle, unsigned short id, unsigned char * data, int dataLength);

	// ------------------- Column Repair ----------------------------------	

	/// @brief Set the columns on which column repair post processing is performed.
	/// @param handle the connected Atik device handle.
	/// @param nColumn the length of the array of column indices provided in columns.
	/// @param columns an array of column indices to perform repair to.
	/// @see ARTEMISERROR, ArtemisGetColumnRepairColumns
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetColumnRepairColumns(		ArtemisHandle handle, int   nColumn, unsigned short * columns);

	/// @brief Get the columns on which column repair post processing is performed.
	/// @param handle the connected Atik device handle.
	/// @param nColumn a pointer to an integer which will be set to the length of the columns array.
	/// @param columns a pointer to a user-managed array of length 1000, which will be set to the column values.
	/// @see ARTEMISERROR, ArtemisSetColumnRepairColumns
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetColumnRepairColumns(		ArtemisHandle handle, int * nColumn, unsigned short * columns);

	/// @brief Remove all column repair processing set previously.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisClearColumnRepairColumns(		ArtemisHandle handle);

	/// @brief Sets whether column repair is enabled
	/// @param handle the connected Atik device handle.
	/// @param value whether column repair is enabled or not. 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetColumnRepairFixColumns(		ArtemisHandle handle, bool value);

	/// @brief Retrieves whether column repair is enabled or not.
	/// @param handle the connected Atik device handle.
	/// @param value a pointer to a boolean which will be set to whether the column repair is enabled or not.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetColumnRepairFixColumns(		ArtemisHandle handle, bool * value);

	/// @brief Retrieves whether column repair can be enabled.
	/// @param handle the connected Atik device handle.
	/// @param value a pointer to a boolean which will be set to whether the column repair can be enabled or not.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetColumnRepairCanFixColumns(	ArtemisHandle handle, bool * value);

	// ---------------- EEPROM -------------------------

	/// @brief Retrieves whether the EEPROM can be interacted with.
	/// This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @param canInteract 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCanInteractWithEEPROM(ArtemisHandle handle, bool * canInteract);

	/// @brief Writes a value to the EEPROM.
	/// This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @param password password required to interact with the device.
	/// @param address address of EEPROM
	/// @param length length of the data
	/// @param data pointer to the buffer containing the data
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisWriteToEEPROM(		   ArtemisHandle handle, char * password, int address, int length, const unsigned char * data);

	/// @brief Reads a value from the EEPROM.
	/// This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @param password password required to interact with the device.
	/// @param address address of EEPROM
	/// @param length length of the data
	/// @param data pointer to the buffer containing the data	
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisReadFromEEPROM(	   ArtemisHandle handle, char * password, int address, int length,       unsigned char * data);


	// ------------------- Filter Wheel -----------------------------------

	/// @brief Retrieve the state of the filter wheel.
	/// @param handle the connected Atik device handle.
	/// @param numFilters pointer to an integer which will be set to the number of filters in the filter wheel.
	/// @param moving pointer to an integer which will be set to 1 if the filter wheel is moving, 0 otherwise.
	/// @param currentPos pointer to an integer which will be set to the current position.
	/// @param targetPos pointer to an integer which will be set to the target position.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisFilterWheelInfo(ArtemisHandle handle, int *numFilters, int *moving, int *currentPos, int *targetPos);

	/// @brief Move the filter wheel to the desired location
	/// @param handle the connected Atik device handle.
	/// @param targetPos the target filter wheel position.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisFilterWheelMove(ArtemisHandle handle, int targetPos);

	/// @brief Checks whether a filter wheel is present at the index.
	/// @param i the index to check.
	/// @return TRUE if present, FALSE if not.
	artfn BOOL			ArtemisEFWIsPresent(int i);

	/// @brief Get information on the filter wheel device at the specified index.
	/// This is the same as ArtemisEFWGetDetails(), but without connecting first.
	/// @param i the index to get details for.
	/// @param type a pointer to an ARTEMISEFWTYPE enumeration, which will be set to the type of the filter wheel device.
	/// @param serialNumber a pointer to a char array of length 100, which will be set to the serial number of the filter wheel.
	/// @see ARTEMISERROR, ARTEMISEFWTYPE, ArtemisEFWGetDetails()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisEFWGetDeviceDetails(int i, enum ARTEMISEFWTYPE * type, char * serialNumber);

	/// @brief Connect to a filter wheel device at the specified index.
	/// @param i The index of the filter wheel device to connect to.
	/// @return a valid ArtemisHandle on success, NULL on failure.
	artfn ArtemisHandle ArtemisEFWConnect(int i);

	/// @brief Retrieves whether the handle for the filter wheel is valid.
	/// @param handle the filter wheel handle.
	/// @see ARTEMISERROR
	/// @return true if connected, false if not.
	artfn bool			ArtemisEFWIsConnected(ArtemisHandle handle);

	/// @brief Disconnect from the filter wheel handle.
	/// @param handle the connected Atik device handle.
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisEFWDisconnect( ArtemisHandle handle);

	/// @brief Get information on the connected filter wheel device.
	/// This is the same as ArtemisEFWGetDeviceDetails(), after connecting.
	/// @param handle the connected Atik device handle.
	/// @param type a pointer to an ARTEMISEFWTYPE enumeration, which will be set to the type of the filter wheel device.
	/// @param serialNumber a pointer to a char array of length 100, which will be set to the serial number of the filter wheel.
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisEFWGetDetails( ArtemisHandle handle, enum ARTEMISEFWTYPE * type, char * serialNumber);

	/// @brief Gets the number of filters inside the filter wheel.
	/// @param handle the connected Atik device handle.
	/// @param nPosition a pointer to an integer which will be set to the current filter wheel position.
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisEFWNmrPosition(ArtemisHandle handle, int * nPosition);

	/// @brief Sets the device's filter wheel position.
	/// @param handle the connected Atik device handle.
	/// @param iPosition the desired position
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int			ArtemisEFWSetPosition(ArtemisHandle handle, int   iPosition);

	/// @brief Gets the device's filter wheel position.
	/// @param handle the connected Atik device handle.
	/// @param iPosition pointer to an integer which will be set to the filter wheel's current position.
	/// @param isMoving pointer to a boolean which will be set to whether the filter wheel is currently moving.
	/// @return 
	artfn int			ArtemisEFWGetPosition(ArtemisHandle handle, int * iPosition, bool * isMoving);

	// ------------------- Firmware ----------------------------------------	

	/// @brief Returns whether firmware can be uploaded to the device.
	/// This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @return true if it can upload, false if not.
	artfn bool ArtemisCanUploadFirmware(ArtemisHandle handle);

	/// @brief Upload a new firmware on the device.
	/// This API is for internal use.
	/// @param handle the connected Atik device handle.
	/// @param fileName the firmware file's full path.
	/// @param password the password needed to enable upload.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int  ArtemisUploadFirmware(   ArtemisHandle handle, char * fileName, char * password);

	// ------------------- Gain ----------------------------------

	/// @brief Get the currently set gain and offset for the device.
	/// Note that Horizon/ACIS cameras need to use ArtemisCameraSpecificOptionGetData() to get the gain.
	/// @param handle the connected Atik device handle.
	/// @param isPreview whether gain will be returned for preview mode or normal mode.
	/// @param gain pointer to an integer which will be set to the current gain.
	/// @param offset pointer to an integer which will be set to the current offset. 
	/// @see ARTEMISERROR, ArtemisCameraSpecificOptionGetData()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetGain(ArtemisHandle handle, bool isPreview, int *gain, int *offset);

	/// @brief Set the gain and offset for the device
	/// Note that Horizon/ACIS cameras need to use ArtemisCameraSpecificOptionSetData() to set the gain.
	/// @param handle the connected Atik device handle.
	/// @param isPreview whether to set gain for preview mode or normal mode.
	/// @param gain the gain to set (scale of 1-24).
	/// @param offset the offset to set.
	/// @see ARTEMISERROR, ArtemisCameraSpecificOptionSetData()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetGain(ArtemisHandle handle, bool isPreview, int  gain, int  offset);

	// ------------------- GPIO -----------------------------------

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param lineCount 
	/// @param lineValues 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetGpioInformation(ArtemisHandle handle, int* lineCount, int* lineValues);

	/// @brief 
	/// @param handle 
	/// @param directionMask 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetGpioDirection(  ArtemisHandle handle, int directionMask);

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param lineValues 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetGpioValues(     ArtemisHandle handle, int lineValues);

	// ------------------- Guiding -----------------------------------

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param axis 1= North 2= South 3= East 4= West
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGuide(					   ArtemisHandle handle, int axis);

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param nibble 0b0001= North 0b0010= South 0b0100=East 0b1000=West
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGuidePort(				   ArtemisHandle handle, int nibble);

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param axis 1= North 2= South 3= East 4= West
	/// @param milli number of milliseconds to pulse for on the selected axis
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisPulseGuide(			   ArtemisHandle handle, int axis, int milli);

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisStopGuiding(			   ArtemisHandle handle);

	/// @brief 
	/// @param handle the connected Atik device handle.
	/// @param bEnable 
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisStopGuidingBeforeDownload(ArtemisHandle handle, bool bEnable);

	// ------------------- Hot Pixel ------------------------------

	enum HotPixelSensitivity { HPS_HIGH, HPS_MEDIUM, HPS_LOW };

	/// @brief A software based hot pixel remover with some pre defined parameters. 
	/// darkFrame = false, checkForAdjacentHotPixels = false, hps = HPS_MEDIUM
	/// @see ArtemisAdvancedHotPixelRemoval()
	/// @param handle the connected Atik device handle.
	/// @param on turns the hot pixel removal off/on
	artfn int ArtemisHotPixelAutoRemoval(ArtemisHandle handle, bool on);

	/// @brief A software based hot pixel remover with several parameters.
	/// @param handle the connected Atik device handle.
	/// @param on turns the hot pixel removal off/on
	/// @param darkFrame When true @ArtemisHotPixelAdvancedStartCalculateHotPixels will need to be 
	/// called to create a hot pixel 'map' that will be used later to remove hot pixels. If false 
	/// the internal hot pixel array will be regenarated for each exposure.
	/// @param checkForAdjacentHotPixels If true any surrounding hot pixels will not be used to determine
	/// the value of  the current hot pixel.
	/// @param hps This determines what defines a hot pixel. HPS_HIGH will see the most hot pixels,
	/// but may think that some normal pixels are hot.
	artfn int ArtemisHotPixelAdvancedRemoval(ArtemisHandle handle, bool on, bool darkFrame, 
											 bool checkForAdjacentHotPixels, enum HotPixelSensitivity hps);

	/// @brief Will begin the process of calculating the internal array of hot pixels determined using
	/// the darkFrame option of @ArtemisHotPixelAdvancedRemoval this function needs to be called after any 
	/// dimension, temperature or binning change.
	/// @param handle the connected Atik camera.
	/// @param exposureLength determines the length of the dark frame to take.
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_FUNCTION if the camera is a colour camera or
	/// ARTEMIS_INVALID_PARAMETER if the camera handle is no longer valid.
	artfn int ArtemisHotPixelAdvancedStartCalculateHotPixels(ArtemisHandle handle, float exposureLength);

	/// @brief After calling @ArtemisHotPixelAdvancedRecalculateHotPixels we recommend that you poll this function
	/// until calculationComplete returns true.
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_PARAMETER if the handle cannot be found or
	/// ARTEMIS_OPERATION_FAILED if the camera never returns from taking the darkFrame.
	artfn int ArtemisHotPixelAdvancedCalculationComplete(ArtemisHandle handle, bool* calculationComplete);

	// ------------------- Lens -----------------------------------

	/// @brief Gets the current lens aperture value.
	/// @param handle the connected Atik device handle.
	/// @param aperture a pointer to an integer which will be set to the current aperture.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, ARTEMIS_NOT_INITIALIZED if lens not initialised, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetLensAperture(ArtemisHandle handle, int* aperture);

	/// @brief Gets the current lens focus value.
	/// @param handle the connected Atik device handle.
	/// @param focus a pointer to an integer which will be set to the current lens focus value.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, ARTEMIS_NOT_INITIALIZED if lens not initialised, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetLensFocus(   ArtemisHandle handle, int* focus);

	/// @brief Returns the lens numerical limits for the device.
	/// @param handle the connected Atik device handle.
	/// @param apertureMin a pointer to an integer which will be set to the minimum aperture value.
	/// @param apertureMax a pointer to an integer which will be set to the maximum aperture value.
	/// @param focusMin a pointer to an integer which will be set to the minimum focus value.
	/// @param focusMax a pointer to an integer which will be set to the maximum focus value.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, ARTEMIS_NOT_INITIALIZED if lens not initialised, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetLensLimits(  ArtemisHandle handle, int* apertureMin, int* apertureMax, int* focusMin, int* focusMax);

	/// @brief Initialise lens controls.
	/// If not called, lens control methods will return ARTEMIS_NOT_INITIALIZED.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisInitializeLens( ArtemisHandle handle);

	/// @brief Sets the lens aperture.
	/// @param handle the connected Atik device handle.
	/// @param aperture the aperture to set
	/// @see ARTEMISERROR, ArtemisGetLensLimits()
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_PARAMETER if value out of limits, ARTEMIS_NOT_INITIALIZED if lens not initialised, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetLensAperture(ArtemisHandle handle, int aperture);

	/// @brief Sets the lens focus.
	/// @param handle the connected Atik device handle.
	/// @param focus the focus value to set
	/// @see ARTEMISERROR, ArtemisGetLensLimits()
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_PARAMETER if value out of limits, ARTEMIS_NOT_INITIALIZED if lens not initialised, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetLensFocus(   ArtemisHandle handle, int focus);

	// ------------------- Shutter ----------------------------------		

	/// @brief Checks whether the shutter can be opened and closed on the device.
	/// @param handle the connected Atik device handle.
	/// @param canControl a pointer to a boolean which will be set to whether the shutter can be opened and closed.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCanControlShutter( ArtemisHandle handle, bool * canControl);

	/// @brief Opens the shutter on the device. 
	/// Please call ArtemisCanControlShutter() to see if the shutter can be controlled.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR, ArtemisCanControlShutter()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisOpenShutter(		ArtemisHandle handle);

	/// @brief Closes the shutter on the device. 
	/// Please call ArtemisCanControlShutter() to see if the shutter can be controlled
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR, ArtemisCanControlShutter()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCloseShutter(	    ArtemisHandle handle);

	/// @brief Checks whether the shutter speed can be set on the device.
	/// @param handle the connected Atik device handle.
	/// @param canSetShutterSpeed a pointer to a boolean which will be set to whether the shutter speed can be set.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCanSetShutterSpeed(ArtemisHandle handle, bool *canSetShutterSpeed);

	/// @brief Gets the shutter speed.
	/// @param handle the connected Atik device handle.
	/// @param speed a pointer to an integer which will be set to the speed of the shutter.
	/// @see ARTEMISERROR, ArtemisCanSetShutterSpeed(), ArtemisSetShutterSpeed()
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_FUNCTION if the shutter speed is not supported for the device, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetShutterSpeed(	ArtemisHandle handle, int *speed);

	/// @brief Sets the shutter speed.
	/// @param handle the connected Atik device handle.
	/// @param speed the speed to set, between 1 and 200.
	/// @see ARTEMISERROR, ArtemisCanSetShutterSpeed(), ArtemisGetShutterSpeed()
	/// @return ARTEMIS_OK on success, ARTEMIS_INVALID_FUNCTION if the shutter speed is not supported for the device, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetShutterSpeed(	ArtemisHandle handle, int  speed);

	// ------------------- Temperature -----------------------------------

	/// @brief Gets temperature of the device for the specific sensor index, in hundreds of degrees centigrade.
	/// If "sensor" is set to 0, this will be set to the number of temperature sensors on the device.
	/// @param handle the connected Atik device handle.
	/// @param sensor the index of the sensor to obtain information from.
	/// @param temperature a pointer to an integer which will be set to the temperature in tenths of degrees celsius, or the number of temperature sensors on the device.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisTemperatureSensorInfo(ArtemisHandle handle, int sensor, int* temperature);

	/// @brief Sets the cooling target temperature of the device.
	/// This will override ArtemisSetCoolingPower().
	/// @param handle the connected Atik device handle.
	/// @param setpoint the target temperature of the device in hundreds of degrees celsius.
	/// @see ARTEMISERROR, ArtemisSetCoolingPower()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetCooling(		   ArtemisHandle handle, int setpoint);

	/// @brief Sets the cooling power of the cooler directly.
	/// This will override ArtemisSetCooling().
	/// @param handle the connected Atik device handle.
	/// @param power the power level of the cooler, from 0 to 255.
	/// @see ARTEMISERROR, ArtemisSetCooling()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetCoolingPower(      ArtemisHandle handle, int power);

	/// @brief Retrieves information about the cooling settings for the device.
	/// @param handle the connected Atik device handle.
	/// @param flags internal flags representing the cooling state.
	/// @param level the power level of the cooler, usually from 0 to 255.
	/// @param minlvl displays the minimum cooling power level
	/// @param maxlvl displays the maximum cooling power level
	/// @param setpoint the target setpoint temperature for the camera in degrees celsius.
	/// @see ARTEMISERROR, ArtemisSetCoolingPower()
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCoolingInfo(		   ArtemisHandle handle, int* flags, int* level, int* minlvl, int* maxlvl, int* setpoint);

	/// @brief Disables active cooling for the device.
	/// On some devices, this will perform a slow ramp down of the cooler, to avoid thermal shock.
	/// @param handle the connected Atik device handle.
	/// @see ARTEMISERROR
	/// @return ARTEMIS_OK on success, or ARTEMISERROR enumeration on failure
	artfn int ArtemisCoolerWarmUp(		   ArtemisHandle handle);

	/// @brief Gets the window heater power.
	/// @param handle the connected Atik device handle.
	/// @param windowHeaterPower a pointer to an integer, which will be set to the current window heater power, between 0 and 255.
	/// @see ARTEMISERROR, ArtemisSetWindowHeaterPower()
	/// @return ARTEMIS_OK on success, ARTEMARTEMIS_INVALID_PARAMETER if the device does not have a window heater, or ARTEMISERROR enumeration on failure
	artfn int ArtemisGetWindowHeaterPower( ArtemisHandle handle, int* windowHeaterPower);

	/// @brief Sets the window heater power.
	/// @param handle the connected Atik device handle
	/// @param windowHeaterPower A value between 0 and 255 specifying the power to the window heater.
	/// @see ARTEMISERROR, ArtemisGetWindowHeaterPower()
	/// @return ARTEMIS_OK on success, ARTEMARTEMIS_INVALID_PARAMETER if the device does not have a window heater, or ARTEMISERROR enumeration on failure
	artfn int ArtemisSetWindowHeaterPower( ArtemisHandle handle, int  windowHeaterPower);
	
	/// @brief Dynamically loads the Atik implementation DLL.
	/// This method is only needed if the DLL is linked dynamically.
	/// This method is part of the DLL example code.
	/// @param fileName the DLL's file path
	/// @see ArtemisUnLoadDLL()
	/// @return Returns true if the dll could be loaded, false otherwise.
	artfn bool ArtemisLoadDLL(const char * fileName);

	/// @brief Unloads the Atik DLL and frees the internal DLL handle.
	/// This method is only needed if the DLL is linked dynamically.
	/// This method is part of the DLL example code.
	/// @see ArtemisLoadDLL()
	artfn void ArtemisUnLoadDLL();

	#undef artfn

#ifdef __cplusplus
}
#endif

