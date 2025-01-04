/**************************************************
this is the ZWO caa CAA SDK
any question feel free contact us:yang.zhou@zwoptical.com

***************************************************/
#ifndef CAA_API_H
#define CAA_API_H

#ifdef _WINDOWS
#define CAA_API __declspec(dllexport)
#else
#define CAA_API 
#endif

#define CAA_ID_MAX 128

typedef struct _CAA_INFO
{
	int ID;
	char Name[64];
	int MaxStep;//fixed maximum degree
} CAA_INFO;


typedef enum _CAA_ERROR_CODE{
    CAA_SUCCESS = 0,            // Success
    CAA_ERROR_INVALID_INDEX,    // Invalid USB ID
    CAA_ERROR_INVALID_ID,       // Invalid connection ID
    CAA_ERROR_INVALID_VALUE,    // Invalid parameter
    CAA_ERROR_REMOVED,          // CAA device not found, possibly removed
    CAA_ERROR_MOVING,           // CAA is currently moving
    CAA_ERROR_ERROR_STATE,      // CAA is in an invalid state
    CAA_ERROR_GENERAL_ERROR,    // General error
    CAA_ERROR_NOT_SUPPORTED,    // Interface not supported in the current version
    CAA_ERROR_CLOSED,           // Failed to close
    CAA_ERROR_OUT_RANGE,        // Out of 0-360 range
    CAA_ERROR_OVER_LIMIT,       // Exceeded limit
    CAA_ERROR_STALL,            // Stall condition
    CAA_ERROR_TIMEOUT,          // Timeout occurred
    CAA_ERROR_END = -1          // End of errors
} CAA_ERROR_CODE;

typedef struct _CAA_ID{
	unsigned char id[8];
}CAA_ID;

typedef CAA_ID CAA_SN;

typedef struct _CAA_TYPE {
	char type[16];
}CAA_TYPE;

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************
Descriptions:
This should be the first API to be called
get number of connected CAA caa, call this API to refresh device list if CAA is connected
or disconnected

Return: number of connected CAA caa. 1 means 1 caa is connected.
***************************************************************************/
CAA_API int CAAGetNum();

/***************************************************************************
Descriptions:
Get the product ID of each caa, at first set pPIDs as 0 and get length and then malloc a buffer to load the PIDs

Paras:
int* pPIDs: pointer to array of PIDs

Return: length of the array.

Note: This api will be deprecated. Please use CAACheck instead
***************************************************************************/
CAA_API int CAAGetProductIDs(int* pPIDs);

/***************************************************************************
Descriptions:
Check if the device is CAA

Paras:
int iVID: VID is 0x03C3 for CAA
int iPID: PID of the device

Return: If the device is CAA, return 1, otherwise return 0
***************************************************************************/
CAA_API int CAACheck(int iVID, int iPID);

/***************************************************************************
Descriptions:
Get ID of caa

Paras:
int index: the index of caa, from 0 to N - 1, N is returned by GetNum()

int* ID: pointer to ID. the ID is a unique integer, between 0 to CAA_ID_MAX - 1, after opened,
all the operation is base on this ID, the ID will not change.


Return: 
CAA_ERROR_INVALID_INDEX: index value is invalid
CAA_SUCCESS:  operation succeeds

***************************************************************************/
CAA_API CAA_ERROR_CODE CAAGetID(int index, int* ID);

/***************************************************************************
Descriptions:
Open caa

Paras:
int ID: the ID of caa

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_GENERAL_ERROR: number of opened caa reaches the maximum value.
CAA_ERROR_REMOVED: the caa is removed.
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAOpen(int ID);

/***************************************************************************
Descriptions:
Get property of caa.

Paras:
int ID: the ID of caa

CAA_INFO *pInfo:  pointer to structure containing the property of CAA

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_SUCCESS: operation succeeds

***************************************************************************/

CAA_API	CAA_ERROR_CODE CAAGetProperty(int ID, CAA_INFO *pInfo); 



/***************************************************************************
Descriptions:
Move caa to an relatively degree.(移动相对角度)

Paras:
int ID: the ID of caa

int iStep: step value is between 0 to CAA_INFO::MaxStep

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAMove(int ID, float iAngle);

/***************************************************************************
Descriptions:
Move caa to an absolute degree.(移动绝对角度)

Paras:
int ID: the ID of caa

int iStep: step value is between 0 to CAA_INFO::MaxStep

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAMoveTo(int ID, float iAngle);



/***************************************************************************
Descriptions:
Stop moving.

Paras:
int ID: the ID of caa

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAStop(int ID); 


/***************************************************************************
Descriptions:
Check if the caa is moving.

Paras:
int ID: the ID of caa
bool *pbVal: pointer to the value, imply if caa is moving
bool* pbHandControl: pointer to the value, imply caa is moved by handle control, can't be stopped by calling CAAStop()
Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAIsMoving(int ID, bool *pbVal, bool* pbHandControl); 


/***************************************************************************
Descriptions:
Get current degree.

Paras:
int ID: the ID of caa
bool *piStep: pointer to the value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetDegree(int ID, float* piAngle);

/***************************************************************************
Descriptions:
Set as current degree

Paras:
int ID: the ID of caa
int iStep: step value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAACurDegree(int ID, float iAngle);

/***************************************************************************
Descriptions:
Get mini degree

Paras:
int ID: the ID of caa
int iStep: step value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAMinDegree(int ID, float* piAngle);

/***************************************************************************
Descriptions:
Get max degree

Paras:
int ID: the ID of caa
int iStep: step value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAASetMaxDegree(int ID, float iAngle);

/***************************************************************************
Descriptions:
Get max degree

Paras:
int ID: the ID of caa
int iStep: step value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetMaxDegree(int ID, float* piAngle);


/***************************************************************************
Descriptions:
Get the value of the temperature detector, if it's moved by handle, the temperature value is unreasonable, the value is -273 and return error

Paras:
int ID: the ID of caa
bool *pfTemp: pointer to the value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed
CAA_ERROR_GENERAL_ERROR: temperature value is unusable
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetTemp(int ID, float* pfTemp);

/***************************************************************************
Descriptions:
Turn on/off beep, if true the caa will beep at the moment when it begins to move 

Paras:
int ID: the ID of caa
bool bVal: turn on beep if true

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAASetBeep(int ID, bool bVal);

/***************************************************************************
Descriptions:
Get if beep is turned on 

Paras:
int ID: the ID of caa
bool *pbVal: pointer to the value

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
CAA_ERROR_ERROR_STATE: caa is in error state
CAA_ERROR_REMOVED: caa is removed

***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetBeep(int ID, bool* pbVal);

/***************************************************************************
Descriptions:
Set moving direction of caa

Paras:
int ID: the ID of caa

bool bVal: if set as true, the caa will move along reverse direction

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAASetReverse(int ID, bool bVal);

/***************************************************************************
Descriptions:
Get moving direction of caa

Paras:
int ID: the ID of caa

bool *pbVal: pointer to direction value.

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetReverse(int ID, bool* pbVal);


/***************************************************************************
Descriptions:
Close caa

Paras:
int ID: the ID of caa

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAClose(int ID);

/***************************************************************************
Descriptions:
get version string, like "1, 4, 0"
***************************************************************************/
CAA_API char* CAAGetSDKVersion();

/***************************************************************************
Descriptions:
Get firmware version of caa

Paras:
int ID: the ID of caa

int *major, int *minor, int *build: pointer to value.

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API	CAA_ERROR_CODE CAAGetFirmwareVersion(int ID, unsigned char *major, unsigned char *minor, unsigned char *build);

/***************************************************************************
Descriptions:
Get the serial number from a CAA

Paras:
int ID: the ID of caa

CAA_SN* pSN: pointer to SN

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support serial number
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API CAA_ERROR_CODE CAAGetSerialNumber(int ID, CAA_SN* pSN);

/***************************************************************************
Descriptions:
Set the alias to a CAA

Paras:
int ID: the ID of caa

CAA_ID alias: the struct which contains the alias

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support setting alias
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API CAA_ERROR_CODE CAASetID(int ID, CAA_ID alias);

/***************************************************************************
Descriptions:
Get the type of a CAA.
For example:CAA-M54

Paras:
int ID: the ID of caa

CAA_TYPE *CAAType: caa type

Return:
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
CAA_SUCCESS: operation succeeds
***************************************************************************/
CAA_API CAA_ERROR_CODE CAAGetType(int ID, CAA_TYPE* pCAAType);

//#define ASIPRODUCE //API for Produce. It needs to be commented out when it is released to the public
#ifdef ASIPRODUCE

CAA_API CAA_ERROR_CODE CAASendCMD(int ID, unsigned char* buf, int size, bool bRead = false, unsigned char* readBuf = 0);

/***************************************************************************
Descriptions:
Set the serial number to a CAA

Paras:
int ID: the ID of caa

CAA_SN* pSN: pointer to SN

Return: 
CAA_ERROR_INVALID_ID: invalid ID value
CAA_ERROR_CLOSED: not opened
EFW_ERROR_NOT_SUPPORTED: the firmware does not support setting serial number
CAA_SUCCESS: operation succeeds

Note: Now setting serial number dose not through SDK, so this api is not used.
***************************************************************************/
CAA_API CAA_ERROR_CODE CAASetSerialNumber(int ID, CAA_SN* pSN);

#endif

#ifdef __cplusplus
}
#endif

#endif