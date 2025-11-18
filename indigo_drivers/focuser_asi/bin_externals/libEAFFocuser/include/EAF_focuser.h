/**************************************************
this is the ZWO focuser EAF SDK
any question feel free contact us:yang.zhou@zwoptical.com

***************************************************/
#ifndef EAF_FOCUSER_H
#define EAF_FOCUSER_H

#ifdef _WINDOWS
#define EAF_API __declspec(dllexport)
#else
#define EAF_API 
#endif

#define EAF_ID_MAX 128

#define BLE_DEVICE_MIN_ID 50

typedef struct _EAF_INFO
{
	int ID;
	char Name[64];
	int MaxStep;//fixed maximum position
} EAF_INFO;


typedef enum _EAF_ERROR_CODE {
	EAF_SUCCESS = 0,
	EAF_ERROR_INVALID_INDEX,
	EAF_ERROR_INVALID_ID,
	EAF_ERROR_INVALID_VALUE,
	EAF_ERROR_REMOVED, //failed to find the focuser, maybe the focuser has been removed
	EAF_ERROR_MOVING,//focuser is moving
	EAF_ERROR_ERROR_STATE,//focuser is in error state
	EAF_ERROR_GENERAL_ERROR,//other error
	EAF_ERROR_NOT_SUPPORTED,
	EAF_ERROR_CLOSED,
	EAF_ERROR_BATTER_INFO,
	EAF_ERROR_INVALID_LENGTH,

	// BLE
	EAF_BLE_READ_DATA_FAILED = 50,
	EAF_BLE_SEND_DATA_FAILED,
	EAF_BLE_CONNECT_FAILED,
	EAF_BLE_DISCONNECT, // ble is not connected, initialization failed.
	EAF_BLE_PAIR_FAILED, // ble pair
	EAF_BLE_CLEAR_PAIR_FAILED, // clear pair
	EAF_BLE_PAIRING_TIMEOUT, // ble pairing timeout.
	EAF_BLE_RECEIVE_TIMEOUT, // ble receive data timeout.
	EAF_BLE_DEVICE_NOT_EXISTS, // The device is not supported
	EAF_BLE_INVALID_CALLBACK,
	EAF_BLE_NEW_PAIR_REQUEST,   // This is a new pair request
	EAF_BLE_DATA_BUSY,
	EAF_BLE_CHECK_SIZE_FAILED,  // The length of the firmware sent and the length received do not match.

	EAF_ERROR_END = -1
}EAF_ERROR_CODE;

typedef struct _EAF_ID{
	unsigned char id[8];
}EAF_ID;

typedef struct _EAF_TYPE {
	char type[16];
}EAF_TYPE;

typedef struct _EAF_BLE_NAME {
	char name[16];
}EAF_BLE_NAME;

typedef struct _EAF_ERROR_MSG {
	char motor_error_code[3];   // 2 characters + 1 terminator
	char battery_error_code[3]; // 2 characters + 1 terminator
}EAF_ERROR_MSG;

typedef struct _EAF_BATTERY_INFO{
	int battery_temp;
    int battery_vol;
    int battery_charge_curr;
    int battery_percentage;
	int battery_discharge_curr;
	int battery_health;
	int battery_charge_vol;
	int battery_num_of_cycles;
}EAF_BATTERY_INFO;

typedef struct _EAF_ALL_INFO {
	int is_run;						// Motor running state.
	int backlash_steps;				// The backlash steps.
	int current_steps;				// The currect steps.
	float temperature;				// The temperature of eaf. 
	int buzzer_state;				// Buzzer status.
	int reverse_state;				// Reverse state.
	int handle_pressed;				// The handle is pressed.
	int handle_connect;             // Handle connection state.
	int max_steps;					// Max steps.
	int led_state;					// Led state.
	char motor_error_code[2];		// Motor Error code
	char battery_error_code[2];		// Battery Error code
	EAF_BATTERY_INFO battery_info;	// Battery info.
} EAF_ALL_INFO;

typedef enum _EAF_CONTROL_TYPE {
	CONTROL_BLE_NAME = 0,
	CONTROL_LED,
	CONTROL_BUZZER,
	CONTROL_EAF_TYPE,
	CONTROL_BAT_INFO,
	CONTROL_ERR_CODE,
	CONTROL_FW_UPDATE_BLE,       // BLE Upate EAF Firmware
	CONTROL_FW_UPDATE_USB,
	CONTROL_MAX_INDEX,       // Maximum index.
} EAF_CONTROL_TYPE;

typedef struct _EAF_CONTROL_CAPS {
	char name[32];
	char description[128];
	bool isSupported;
	bool isWritable;
	int maxValue;
	int minValue;
	int defaultValue;
	EAF_CONTROL_TYPE controlType;
	char unused[32];
} EAF_CONTROL_CAPS;

typedef EAF_ID EAF_SN;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BLE_DEVICE_INFO {
	char name[64];
	char address[64]; // format bluetooth address
	int signalStrength;
	long long int bluetoothAddress;
} BLE_DEVICE_INFO_T;

typedef void (*ConnStateCallback)(bool state);
typedef void (*PairStateCallback)(EAF_ERROR_CODE state);

/***************************************************************************
Descriptions:
This is the API for scanning surrounding Bluetooth.
If you use Bluetooth to control EAF, you do not need to call EAFGetNum and EAFOpen API.
Paras:
int DurationMs: Scan time ms.
BLE_DEVICE_INFO_T* devices: device info.
int maxDeviceCount: max device count.
int* actualDeviceCount: actual device count.

Return: EAF_SUCCESS or EAF_BLE_DEVICE_NOT_EXISTS
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEScan(int DurationMs, BLE_DEVICE_INFO_T* devices, int maxDeviceCount, int* actualDeviceCount);

/***************************************************************************
Descriptions:
This is the api for connecting to eaf device bluetooth.
Paras:
const char* pDeviceName: connect device name.
int *ID: connect device id.

Return: EAF_SUCCESS or EAF_ERROR_CLOSED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEConnect(const char* pDeviceName, const char* pDeviceAddress, int *ID);

/***************************************************************************
Descriptions:
Register a Bluetooth pair state callback for an EAF device.
Paras:
int ID: connect device id.
PairStateCallback callback：
Return one of the following `EAF_ERROR_CODE` values:
 *EAF_SUCCESS              : Pairing successful.
 *EAF_BLE_NEW_PAIR_REQUEST : New device pairing request
 *EAF_BLE_PAIRING_TIMEOUT  : Pairing process timed out.
 *EAF_BLE_PAIRING_FAILED   : Pairing failed due to errors.
 If set to `NULL`, no callback will be registered.

Return: EAF_SUCCESS or EAF_ERROR_CLOSED

Note：
 * 1. The callback function is optional; if no callback is provided, the pairing state
 *    will not trigger any user-defined actions.
 * 2. Ensure that the device ID is valid and the EAF device is powered on before
 *    registering the callback.
 * 3. If you wish to stop receiving pairing state updates, you can pass a `NULL` callback
 *    or unregister the callback using the appropriate API.
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLERegPairStateCallback(int ID, PairStateCallback callback);

/***************************************************************************
Descriptions:
This interface is the ble pairing interface;
Paras:
int ID: connect device id.

Return: EAF_SUCCESS or EAF_BLE_PAIR_FAILED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEPair(int ID);

/***************************************************************************
Descriptions:
This interface is the clear ble pairing interface;
Paras:
int ID: connect device id.

Return: EAF_SUCCESS or EAF_BLE_CLEAR_PAIR_FAILED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEClearPair(int ID);

/***************************************************************************
Descriptions:
This is the API to disconnect the Bluetooth connection.
Paras:
int ID: connect device id.

Return: EAF_SUCCESS or EAF_ERROR_CLOSED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEDisconnect(int ID);

/***************************************************************************
Descriptions:
Register a Bluetooth connection state callback for an EAF device.
Paras:
const char* pDeviceName: connect device name.
int ID: connect device id.
ConnStateCallback callback: 
Return value: 
true  :conncet; 
false :not connect

Return: EAF_SUCCESS or EAF_ERROR_CLOSED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLERegConnStateCallback(int ID, ConnStateCallback callback);

/***************************************************************************
Descriptions:
This is the interface to get all the information of EAF through BLE.
Paras:
int ID: connect device id.
EAF_ALL_INFO *pAllInfo:The data returned.

Return: 
EAF_SUCCESS
EAF_BLE_DISCONNECT
EAF_ERROR_INVALID_ID
EAF_ERROR_INVALID_VALUE
EAF_ERROR_NOT_SUPPORTED
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFBLEgetAllInfo(int ID, EAF_ALL_INFO *pAllInfo);

/***************************************************************************
Descriptions:
This should be the first API to be called
get number of connected EAF focuser, call this API to refresh device list if EAF is connected
or disconnected

Return: number of connected EAF focuser. 1 means 1 focuser is connected.
***************************************************************************/
EAF_API int EAFGetNum();

/***************************************************************************
Descriptions:
Get the product ID of each focuser, at first set pPIDs as 0 and get length and then malloc a buffer to load the PIDs

Paras:
int* pPIDs: pointer to array of PIDs

Return: length of the array.

Note: This api will be deprecated. Please use EAFCheck instead
***************************************************************************/
EAF_API int EAFGetProductIDs(int* pPIDs);

/***************************************************************************
Descriptions:
Check if the device is EAF

Paras:
int iVID: VID is 0x03C3 for EAF
int iPID: PID of the device

Return: If the device is EAF, return 1, otherwise return 0
***************************************************************************/
EAF_API int EAFCheck(int iVID, int iPID);

/***************************************************************************
Descriptions:
Get ID of focuser

Paras:
int index: the index of focuser, from 0 to N - 1, N is returned by GetNum()

int* ID: pointer to ID. the ID is a unique integer, between 0 to EAF_ID_MAX - 1, after opened,
all the operation is base on this ID, the ID will not change.


Return: 
EAF_ERROR_INVALID_INDEX: index value is invalid
EAF_SUCCESS:  operation succeeds

***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetID(int index, int* ID);

/***************************************************************************
Descriptions:
Open focuser

Paras:
int ID: the ID of focuser

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_GENERAL_ERROR: number of opened focuser reaches the maximum value.
EAF_ERROR_REMOVED: the focuser is removed.
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFOpen(int ID);

/***************************************************************************
Descriptions:
Get property of focuser.

Paras:
int ID: the ID of focuser

EAF_INFO *pInfo:  pointer to structure containing the property of EAF

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_SUCCESS: operation succeeds

***************************************************************************/

EAF_API	EAF_ERROR_CODE EAFGetProperty(int ID, EAF_INFO *pInfo); 

/***************************************************************************
Descriptions:
Get number of controls available for this eaf. the eaf need be opened at first.

Paras:
int ID: the ID of focuser
int * piNumberOfControls: pointer to an int to save the number of controls

return:
EAF_SUCCESS : Operation is successful
EAF_ERROR_CLOSED : eaf didn't open
EAF_BLE_DISCONNECT: eaf ble disconnect
EAF_ERROR_INVALID_ID  :no eaf of this ID is connected or ID value is out of boundary
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetNumOfControls(int ID, int* piNumberOfControls);

/***************************************************************************
Descriptions:
Get controls property available for this eaf. the eaf need be opened at first.
user need to malloc and maintain the buffer.

Paras:
int ID: the ID of focuser
int iControlIndex: index of control, NOT control type
EAF_CONTROL_CAPS * pControlCaps: Pointer to structure containing the property of the control
user need to malloc the buffer

return:
EAF_SUCCESS : Operation is successful
EAF_ERROR_CLOSED : eaf didn't open
EAF_BLE_DISCONNECT: eaf ble disconnect 
EAF_ERROR_INVALID_ID  :no eaf of this ID is connected or ID value is out of boundary
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetControlCaps(int ID, int iControlIndex, EAF_CONTROL_CAPS *pControlCaps);

/***************************************************************************
Descriptions:
Move focuser to an absolute position.

Paras:
int ID: the ID of focuser

int iStep: step value is between 0 to EAF_INFO::MaxStep

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFMove(int ID, int iStep);


/***************************************************************************
Descriptions:
Stop moving.

Paras:
int ID: the ID of focuser

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFStop(int ID); 


/***************************************************************************
Descriptions:
Check if the focuser is moving.

Paras:
int ID: the ID of focuser
bool *pbVal: pointer to the value, imply if focuser is moving
bool* pbHandControl: pointer to the value, imply focuser is moved by handle control, can't be stopped by calling EAFStop()
Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFIsMoving(int ID, bool *pbVal, bool* pbHandControl); 


/***************************************************************************
Descriptions:
Get current position.

Paras:
int ID: the ID of focuser
bool *piStep: pointer to the value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetPosition(int ID, int* piStep);

/***************************************************************************
Descriptions:
Set as current position

Paras:
int ID: the ID of focuser
int iStep: step value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFResetPostion(int ID, int iStep);

/***************************************************************************
Descriptions:
Get the value of the temperature detector, if it's moved by handle, the temperature value is unreasonable, the value is -273 and return error

Paras:
int ID: the ID of focuser
bool *pfTemp: pointer to the value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed
EAF_ERROR_GENERAL_ERROR: temperature value is unusable
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetTemp(int ID, float* pfTemp);

/***************************************************************************
Descriptions:
Turn on/off beep, if true the focuser will beep at the moment when it begins to move 

Paras:
int ID: the ID of focuser
bool bVal: turn on beep if true

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFSetBeep(int ID, bool bVal);

/***************************************************************************
Descriptions:
Get if beep is turned on 

Paras:
int ID: the ID of focuser
bool *pbVal: pointer to the value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed

***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetBeep(int ID, bool* pbVal);

/***************************************************************************
Descriptions:
Set the maximum position

Paras:
int ID: the ID of focuser
int iVal: maximum position

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_MOVING: focuser is moving, should wait until idle
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFSetMaxStep(int ID, int iVal);

/***************************************************************************
Descriptions:
Get the maximum position

Paras:
int ID: the ID of focuser
bool *piVal: pointer to the value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_MOVING: focuser is moving, should wait until idle
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetMaxStep(int ID, int* piVal);

/***************************************************************************
Descriptions:
Get the position range

Paras:
int ID: the ID of focuser
bool *piVal: pointer to the value

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
EAF_ERROR_MOVING: focuser is moving, should wait until idle
EAF_ERROR_ERROR_STATE: focuser is in error state
EAF_ERROR_REMOVED: focuser is removed
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFStepRange(int ID, int* piVal);

/***************************************************************************
Descriptions:
Set moving direction of focuser

Paras:
int ID: the ID of focuser

bool bVal: if set as true, the focuser will move along reverse direction

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFSetReverse(int ID, bool bVal);

/***************************************************************************
Descriptions:
Get moving direction of focuser

Paras:
int ID: the ID of focuser

bool *pbVal: pointer to direction value.

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetReverse(int ID, bool* pbVal);

/***************************************************************************
Descriptions:
Set backlash of focuser

Paras:
int ID: the ID of focuser

int iVal: backlash value.

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_INVALID_VALUE: iVal needs to be between 0 and 255
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFSetBacklash(int ID, int iVal);

/***************************************************************************
Descriptions:
Get backlash of focuser

Paras:
int ID: the ID of focuser

int *piVal: pointer to backlash value.

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetBacklash(int ID, int* piVal);

/***************************************************************************
Descriptions:
Close focuser

Paras:
int ID: the ID of focuser

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFClose(int ID);

/***************************************************************************
Descriptions:
get version string, like "1, 4, 0"
***************************************************************************/
EAF_API char* EAFGetSDKVersion();

/***************************************************************************
Descriptions:
Get firmware version of focuser

Paras:
int ID: the ID of focuser

int *major, int *minor, int *build: pointer to value.

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API	EAF_ERROR_CODE EAFGetFirmwareVersion(int ID, unsigned char *major, unsigned char *minor, unsigned char *build);

/***************************************************************************
Descriptions:
Get the serial number from a EAF

Paras:
int ID: the ID of focuser

EAF_SN* pSN: pointer to SN

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support serial number
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetSerialNumber(int ID, EAF_SN* pSN);

/***************************************************************************
Descriptions:
Get the battery percentage and battery temperature of the EAF

Paras:
int ID: the ID of focuser

EAF_BATTERY_INFO* pBatteryInfo: battery info

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support serial number
EAF_ERROR_BATTER_INFO: The battery temperature is abnormal, and pBatteryInfo returns -1
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetBatteryInfo(int ID, EAF_BATTERY_INFO *pBatteryInfo);

/***************************************************************************
Descriptions:
Set the alias to a EAF

Paras:
int ID: the ID of focuser

EAF_ID alias: the struct which contains the alias

Return: 
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support setting alias
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFSetID(int ID, EAF_ID alias);

/***************************************************************************
Descriptions:
Get the type of a EAF.

Paras:
int ID: the ID of focuser

EAF_TYPE *pEAFType:

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds

***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetType(int ID, EAF_TYPE* pEAFType);

/***************************************************************************
Descriptions:
Get the ble name of a EAF.

Paras:
int ID: the ID of focuser

EAF_BLE_NAME *pEAFBLEName:

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds

Note: Limited to the HID connection to get the eaf Bluetooth name.
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetBLEName(int ID, EAF_BLE_NAME* pEAFBLEName);

/***************************************************************************
Descriptions:
set the ble name of a EAF.
EAFBLEName length >0 and <= 6.

Paras:
int ID: the ID of focuser

EAF_BLE_NAME EAFBLEName:

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_SUCCESS: operation succeeds

Note: Only the last six bits can be modified. 
A successful update will disconnect the connection and require a scan to reconnect.
Example: EAF Pro_57fb5d -> EAF Pro_123456
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFSetBLEName(int ID, EAF_BLE_NAME EAFBLEName);

/***************************************************************************
Descriptions:
get EAF LED switch state;

Paras:
int ID: the ID of focuser
bool *bState: true/false; 
true:	on
false:  off

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_BLE_DISCONNECT：BLE is disconnect
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetLedState(int ID, bool *bState);

/***************************************************************************
Descriptions:
set EAF LED switch state

Paras:
int ID: the ID of focuser

bool bState: 
true： Restore the current state (on)；
false: LED off；

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_BLE_DISCONNECT：BLE is disconnect
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFSetLedState(int ID, bool bState);

/***************************************************************************
Descriptions:
get EAF error code;

Paras:
int ID: the ID of focuser
ERROR_CODE* pErrorCode: error code 
error code:
	E0 no error
	E5 motor stall
	E6 High temperature stop charge
	E7 High temperature stop discharge
	E8 Low temperature stop charge

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_BLE_DISCONNECT：BLE is disconnect
EAF_ERROR_NOT_SUPPORTED: not supported
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetErrorCode(int ID, EAF_ERROR_MSG* pErrorCode);

/***************************************************************************
Descriptions:
Set shipping mode;

Paras:
int ID: the ID of focuser

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support setting shipping mode
EAF_BLE_DISCONNECT: ble not disconnect
EAF_SUCCESS: operation succeeds

Note: Now setting serial number dose not through SDK, so this api is not used.
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFSetShippingMode(int ID);

/***************************************************************************
Descriptions:
get EAF power off reason;

Paras:
int ID: the ID of focuser
int* pErrorCode: reason
Reason:
	0 normal
	1 shipping mode

Return:
EAF_ERROR_INVALID_ID: invalid ID value
EAF_ERROR_CLOSED: not opened
EAF_BLE_DISCONNECT：BLE is disconnect
EAF_ERROR_NOT_SUPPORTED: not supported
EAF_SUCCESS: operation succeeds
***************************************************************************/
EAF_API EAF_ERROR_CODE EAFGetReason(int ID, int* pReason);

#ifdef __cplusplus
}
#endif

#endif
