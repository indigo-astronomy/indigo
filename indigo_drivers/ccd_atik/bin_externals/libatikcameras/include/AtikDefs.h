#ifndef ATIK_DEFS_H
#define ATIK_DEFS_H

/// @file AtikDefs.h
///	@brief Atik SDK structure definitions.

#ifdef __cplusplus
extern "C" 
{
#endif
	/// Atik SDK handle type
	/// @see ArtemisConnect(), ArtemisEFWConnect()
	typedef void* ArtemisHandle;

	enum SensorName {
		SENSOR_NAME_UNKNOWN, 
		SENSOR_NAME_MN34230, // HORIZON
		SENSOR_NAME_IMX_428, SENSOR_NAME_IMX_249, SENSOR_NAME_IMX_304, // ACIS
		SENSOR_NAME_IMX_533, SENSOR_NAME_IMX_571, SENSOR_NAME_IMX_455, // APX
		SENSOR_NAME_E2V_77, SENSOR_NAME_E2V_47, SENSOR_NAME_E2V_42, // TE
	};

	enum CameraFamily { 
		CAMERA_FAMILY_UNKNOWN, CAMERA_FAMILY_HORIZON, CAMERA_FAMILY_ACIS, CAMERA_FAMILY_APX,
		CAMERA_FAMILY_TE, CAMERA_FAMILY_IC24, CAMERA_FAMILY_SONY_SCI, CAMERA_FAMILY_GP
	};

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
		/// 
		ARTEMIS_INVALID_PASSWORD
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
		CAMERA_EXTERNAL_TRIGGER
	};

	// @see ArtemisCameraConnectionState
	enum ARTEMISCONNECTIONSTATE
	{
		CAMERA_CONNECTING = 1,
		CAMERA_CONNECTED = 2,
		CAMERA_CONNECT_FAILED = 3,
		CAMERA_SUSPENDED = 4,
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
		ARTEMIS_COOLING_INFO_HASCOOLING = 1,
		/// Cooling is always on or can be controlled. 0= Always on 1= Controllable
		ARTEMIS_COOLING_INFO_CONTROLLABLE = 2,
		/// Cooling can be switched On/Off. 0= On/Off control not available 1= On/Off control available
		ARTEMIS_COOLING_INFO_ONOFFCOOLINGCONTROL = 4,
		/// Cooling can be set via ArtemisSetCoolingPower()
		ARTEMIS_COOLING_INFO_POWERLEVELCONTROL = 8,
		/// Cooling can be set via ArtemisSetCooling()
		ARTEMIS_COOLING_INFO_SETPOINTCONTROL = 16,
		/// Currently warming up. 0= Normal control 1= Warming Up
		ARTEMIS_COOLING_INFO_WARMINGUP = 32,
		/// Currently cooling. 0= Cooling off 1= Cooling on
		ARTEMIS_COOLING_INFO_COOLINGON = 64,
		/// Currently under setpoint control 0= No set point control 1= Set point control
		ARTEMIS_COOLING_INFO_SETPOINTCONTROLON = 128
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
		ID_GOPresetMode = 1,
		ID_GOPresetLow = 2,
		ID_GOPresetMed = 3,
		ID_GOPresetHigh = 4,
		ID_GOCustomGain = 5,
		ID_GOCustomOffset = 6,
		ID_EvenIllumination = 12,
		ID_PadData = 13,
		ID_ExposureSpeed = 14,
		ID_BitSendMode = 15, //ACIS specific
		ID_16BitMode = 18, //ChemiMOS specific
		ID_ReadoutModeTE = 20, // TE series specific
		ID_ADC1OffsetTE = 88,
		ID_FX3Version = 200,
		ID_FPGAVersion = 201,
	};

	enum ReadoutModeTE
	{
		EXP_MODE_TE_LOWEST_NOISE,
		EXP_MODE_TE_LOW_NOISE,
		EXP_MODE_TE_PREVIEW
	};

	enum HotPixelSensitivity
	{ 
		HPS_HIGH,
		HPS_MEDIUM,
		HPS_LOW
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


    ///
    /// @brief Structure of the data, a pointer to which is, passed to
    /// the info parameter of FastCallbackEx function set by a call to
    /// ArtemisSetFastCallbackEx.
    /// 
    /// @see ArtemisSetFastCallbackEx
    ///
    struct FastCallbackInfo_
    {
        /// Set to the size of this structure minus 1.
        unsigned char size;

        /// Number of dropped frames since the previous callback (softDroppedFrames + hardDroppedFrames).
        unsigned char droppedFrames;

        /// Number of dropped frames in software since the previous callback.
        unsigned char softDroppedFrames;

        /// Number of dropped frames in camera since the previous callback.
        unsigned char hardDroppedFrames;

        /// Start of exposure year.
        int exposureStartTimeYear;

        /// Start of exposure month. 
        int exposureStartTimeMonth;

        /// Start of exposure day.
        int exposureStartTimeDay;

        /// Start of exposure hour (24 hour clock format) (UTC).
        int exposureStartTimeHour;

        /// Start of exposure minute (UTC).
        int exposureStartTimeMinute;

        /// Start of exposure second (UTC).
        int exposureStartTimeSecond;

        /// Start of exposure millisecond (UTC).
        int exposureStartTimeMS;

        /// Total number of exposures since the last ArtemisStartFastExposure() call. Includes dropped frames.
        int exposureNumber;

        /// The average image data transfer rate in MB/s.
        double dataRateMBps;
    };
    typedef struct FastCallbackInfo_ FastCallbackInfo;

	typedef void(*FastModeCallbackFnPtr)(ArtemisHandle handle, int x, int y, int w, int h, int binx, int biny, void* imageBuffer);
	typedef void(*FastModeCallbackExFnPtr)(ArtemisHandle handle, int x, int y, int w, int h, int binx, int biny, void* imageBuffer, unsigned char* info);

	///
	/// @brief Structure for holding region-of-interest (ROI) information.
	///
	/// @see ArtemisSetRegionsOfInterest()
	/// @see ArtemisGetRegionsOfInterest()
	///
	struct AtikROI_
	{
		//! The X coordinate of the top-left of the ROI.
		int x;

		//!The Y coordinate of the top-left of the ROI.
		int y;

		//! The width of the ROI. 
		/*!
			When passed to ArtemisSetRegionsOfInterest(),
			this is the width of the ROI before binning.

			When returned from ArtemisGetRegionsOfInterest(),
			this is the width of the ROI after binning.
		*/
		int w;

		//! The height of the ROI.
		/*!
			When passed to ArtemisSetRegionsOfInterest(),
			this is the height of the ROI before binning.

			When returned from ArtemisGetRegionsOfInterest(),
			this is the height of the ROI after binning.
		*/
		int h;

		//!The binning factor in the x direction.
		int binx;

		//!The binning factor in the y direction.
		int biny;

		//! Pointer to image data for this ROI.
		/*!
			Set when an instance of this structure is returned
			from the ArtemisGetRegionsOfInterest() function.

			No used by ArtemisSetRegionsOfInterest().
		*/
		void* imageBuffer;
	};

	typedef struct AtikROI_ AtikROI;

#ifdef __cplusplus
}
#endif

#endif // ATIK_DEFS_H
