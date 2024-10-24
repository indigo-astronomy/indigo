/*
 * Copyright 2024 Suzhou Astroasis Vision Technology, Inc. All Rights Reserved.
 *
 * This is header file for Astroasis Oasis Focuser.
 *
 * Note:
 * 1. AOFocuserScan() should be called before any other APIs (except for
 *    AOFocuserGetSDKVersion()) for at least one time because those APIs require
 *    focuser ID as their first parameter.
 * 2. AOFocuserScan() can be called for multiple times at any time as long as
 *    the user wants to refresh focusers.
 * 3. AOFocuserGetSDKVersion() can be called at anytime and any place.
 * 4. The buffer length for names (including product model, serial number,
 *    friendly name, Bluetooth name) should not less than AO_FOCUSER_NAME_LEN.
 * 5. The list of API functions are as follows.
 *        AOFocuserScan(int *number, int *ids);
 *        AOFocuserOpen(int id);
 *        AOFocuserClose(int id);
 *        AOFocuserGetProductModel(int id, char *model);
 *        AOFocuserGetVersion(int id, AOFocuserVersion *version);
 *        AOFocuserGetSerialNumber(int id, char *sn);
 *        AOFocuserGetFriendlyName(int id, char *name);
 *        AOFocuserSetFriendlyName(int id, char *name);
 *        AOFocuserGetBluetoothName(int id, char *name);
 *        AOFocuserSetBluetoothName(int id, char *name);
 *        AOFocuserGetConfig(int id, AOFocuserConfig *config);
 *        AOFocuserSetConfig(int id, AOFocuserConfig *config);
 *        AOFocuserGetStatus(int id, AOFocuserStatus *status);
 *        AOFocuserFactoryReset(int id);
 *        AOFocuserSetZeroPosition(int id);
 *        AOFocuserMove(int id, int step);
 *        AOFocuserMoveTo(int id, int position);
 *        AOFocuserStopMove(int id);
 *        AOFocuserClearStall(int id);
 *        AOFocuserFirmwareUpgrade(int id, unsigned char *data, int len);
 *        AOFocuserGetSDKVersion(char *version);
 *
 * Refer to SDK demo application for the details of the API usage.
 */

#ifndef __AOFOCUS_H__
#define __AOFOCUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WINDOWS
#define AOAPI		__declspec(dllexport)
#else
#define AOAPI
#endif

#define VERSION_INVALID					0

#define PROTOCAL_VERSION_16_0_3			0x10000300
#define PROTOCAL_VERSION_16_1_0			0x10010000

#define AO_FOCUSER_MAX_NUM				32		/* Maximum focuser numbers supported by this SDK */
#define AO_FOCUSER_VERSION_LEN			32		/* Buffer length for version strings */
#define AO_FOCUSER_NAME_LEN				32		/* Buffer length for name strings */

#define TEMPERATURE_INVALID				0x80000000	/* This value indicates the invalid value of ambient temperature */

#define AO_LOG_LEVEL_QUIET					0
#define AO_LOG_LEVEL_ERROR					1
#define AO_LOG_LEVEL_INFO					2
#define AO_LOG_LEVEL_DEBUG					3

typedef enum _AOReturn {
	AO_SUCCESS = 0,						/* Success */
	AO_ERROR_INVALID_ID,				/* Device ID is invalid */
	AO_ERROR_INVALID_PARAMETER,			/* One or more parameters are invalid */
	AO_ERROR_INVALID_STATE,				/* Device is not in correct state for specific API call */
	AO_ERROR_BUFFER_TOO_SMALL,			/* Size of buffer is too small */
	AO_ERROR_COMMUNICATION,				/* Data communication error such as device has been removed from USB port */
	AO_ERROR_TIMEOUT,					/* Timeout occured */
	AO_ERROR_BUSY,						/* Device is being used by another application */
	AO_ERROR_NULL_POINTER,				/* Caller passes null-pointer parameter which is not expected */
	AO_ERROR_OUT_OF_RESOURCE,			/* Out of resouce such as lack of memory */
	AO_ERROR_NOT_IMPLEMENTED,			/* The interface is not currently supported */
	AO_ERROR_FAULT,						/* Significant fault which means the device may not work correctly and hard to recovery it */
	AO_ERROR_INVALID_SIZE,				/* Size is invalid */
	AO_ERROR_INVALID_VERSION,			/* Version is invalid */
	AO_ERROR_UNKNOWN = 0x40,			/* Any other errors */
} AOReturn;

/*
 * Used by AOFocuserSetConfig() to indicates which field wants to be set
 */
#define MASK_MAX_STEP				0x00000001
#define MASK_BACKLASH				0x00000002
#define MASK_BACKLASH_DIRECTION		0x00000004
#define MASK_REVERSE_DIRECTION		0x00000008
#define MASK_SPEED					0x00000010
#define MASK_BEEP_ON_MOVE			0x00000020
#define MASK_BEEP_ON_STARTUP		0x00000040
#define MASK_BLUETOOTH				0x00000080
#define MASK_STALL_DETECTION		0x00000100
#define MASK_HEATING_TEMPERATURE	0x00000200
#define MASK_HEATING_ON				0x00000400
#define MASK_USB_POWER_CAPACITY		0x00000800
#define MASK_ALL					0xFFFFFFFF

typedef struct _AOFocuserVersion
{
	unsigned int protocal;			/* Version of protocal over USB and Bluetooth communication */
	unsigned int hardware;			/* Focuser hardware version */
	unsigned int firmware;			/* Focuser firmware version */
	char built[24];					/* Null-terminated string which indicates firmware building time */
} AOFocuserVersion;

typedef struct _AOFocuserConfig {
	unsigned int mask;				/* Used by AOFocuserSetConfig() to indicates which field wants to be set */
	int maxStep;					/* Maximum step or position */
	int backlash;					/* Backlash value */
	int backlashDirection;			/* Backlash direction. 0 - IN, others - OUT */
	int reverseDirection;			/* 0 - Not reverse motor moving direction, others - Reverse motor moving direction */
	int speed;						/* Reserved for motor speed setting. Should always be zero for now */
	int beepOnMove;					/* 0 - Turn off beep for move, others - Turn on beep for move */
	int beepOnStartup;				/* 0 - Turn off beep for device startup, others - Turn on beep for device startup */
	int bluetoothOn;				/* 0 - Turn off Bluetooth, others - Turn on Bluetooth */
	int stallDetection;				/* 0 - Turn off motor stall detection, others - Turn on motor stall detection */
	int heatingTemperature;			/* Target heating temperature in 0.01 degree unit */
	int heatingOn;					/* 0 - Turn off heating, others - Turn on heating */
	int usbPowerCapacity;			/* 0 - 500mA, 1 - 1000mA or higher */
} AOFocuserConfig;

/*
 * Note:
 * temperatureExt is valid only when temperatureDetection is not zero and
 * temperatureExt is not equal to TEMPERATURE_INVALID
 */
typedef struct _AOFocuserStatus {
	int temperatureInt;				/* Internal (on board) temperature in 0.01 degree unit */
	int temperatureExt;				/* External (ambient) temperature in 0.01 degree unit */
	int temperatureDetection;		/* 0 - ambient temperature probe is not inserted, others - ambient temperature probe is inserted */
	int position;					/* Current motor position */
	int moving;						/* 0 - motor is not moving, others - Motor is moving */
	int stallDetection;				/* 0 - motor stall is not detected, others - motor stall detected */
	int heatingOn;					/* 0 - heating is disabled, others - heating is enabled */
	int heatingPower;				/* Current heating power in 1% unit */
	int dcPower;					/* Current DC power in 0.1V unit */
	int reserved[20];
} AOFocuserStatus;

/*
 * API: AOFocuserScan
 *
 * Description
 *     Scan for connected focusers.
 *
 * Parameters
 *     number:
 *         Pointer to an int value to hold the number of connected focusers.
 *     ids:
 *         Pointer to an int array to hold IDs of connected focusers.
 *         The length of the 'ids' array should be equal to or larger than
 *         AO_FOCUSER_MAX_NUM, and it's the caller's responsibility to allocate
 *         memory for "ids" before the API is called.
 *         The valid IDs are stored in first "number" of elements in "ids" array.
 *
 * Return value
 *     AO_SUCCESS:
 *         The function succeeds. This is the only value the API returns.
 *
 * Remarks
 *     This function should be called before any other APIs except for
 *     AOFocuserGetSDKVersion().
 *     Each focuser has a unique ID. If one focuser is removed from USB port
 *     and then plugged in again, it will be considered as different focuser
 *     and will be assigned with different ID during next scan. If one focuser
 *     remains connected during two or more scan operations, it will always
 *     have the same ID to indicate it's the same focuser. That is, the caller
 *     can call AOFocuserScan() at any time and the IDs of connected focusers
 *     returned by previous AOFocuserScan() will still be valid.
 */
AOAPI AOReturn AOFocuserScan(int *number, int *ids);
AOAPI AOReturn AOFocuserOpen(int id);
AOAPI AOReturn AOFocuserClose(int id);
AOAPI AOReturn AOFocuserGetProductModel(int id, char *model);
AOAPI AOReturn AOFocuserGetVersion(int id, AOFocuserVersion *version);
AOAPI AOReturn AOFocuserGetSerialNumber(int id, char *sn);
AOAPI AOReturn AOFocuserGetFriendlyName(int id, char *name);
AOAPI AOReturn AOFocuserSetFriendlyName(int id, char *name);
AOAPI AOReturn AOFocuserGetBluetoothName(int id, char *name);
AOAPI AOReturn AOFocuserSetBluetoothName(int id, char *name);
AOAPI AOReturn AOFocuserGetConfig(int id, AOFocuserConfig *config);
AOAPI AOReturn AOFocuserSetConfig(int id, AOFocuserConfig *config);
AOAPI AOReturn AOFocuserGetStatus(int id, AOFocuserStatus *status);
AOAPI AOReturn AOFocuserFactoryReset(int id);
AOAPI AOReturn AOFocuserSetZeroPosition(int id);
AOAPI AOReturn AOFocuserSyncPosition(int id, int position);
AOAPI AOReturn AOFocuserMove(int id, int step);
AOAPI AOReturn AOFocuserMoveTo(int id, int position);
AOAPI AOReturn AOFocuserStopMove(int id);
AOAPI AOReturn AOFocuserClearStall(int id);
AOAPI AOReturn AOFocuserUpgrade(int id);
AOAPI AOReturn AOFocuserFirmwareUpgrade(int id, unsigned char *data, int len);
AOAPI AOReturn AOFocuserGetSDKVersion(char *version);
AOAPI AOReturn AOFocuserSetLogLevel(int level);

#ifdef __cplusplus
}
#endif

#endif	/* __AOFOCUS_H__ */
