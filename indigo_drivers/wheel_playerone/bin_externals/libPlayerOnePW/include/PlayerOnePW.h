/****************************************************************************
**
** Copyright (C) 2022 The Player One Astronomy Co., Ltd.
** This software is the secondary software development kit (SDK) for
** the astronomy filter wheel made by Player One Astronomy Co., Ltd.
** Player One Astronomy Co., Ltd (hereinafter referred to as "the company") owns its copyright.
** This SDK is only used for the secondary development of our company's filter wheel or other equipment.
** You can use our company's products and this SDK to develop any products without any restrictions.
** This software may use third-party software or technology (including open source code and public domain code that this software may use),
** and such use complies with their open source license or has obtained legal authorization.
** If you have any questions, please contact us: support@player-one-astronomy.com
** Copyright (C) Player One Astronomy Co., Ltd. All rights reserved.
**
****************************************************************************/
/**************Player One Phoenix Filter Wheel referred to as PW*****************/

#ifndef PLAYERONEPW_H
#define PLAYERONEPW_H


#ifdef __cplusplus
extern "C" {
#endif


#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define POA_PW_API __declspec(dllexport)
#else
#  define POA_PW_API     __attribute__((visibility("default")))
#endif

/*-----For more information, please read our development manual-----*/



typedef enum _PWErrors              ///< Return Error Code Definition
{
    PW_OK = 0,                     ///< operation successful
    PW_ERROR_INVALID_INDEX,        ///< invalid index, means the index is < 0 or >= the count
    PW_ERROR_INVALID_HANDLE,       ///< invalid PW handle
    PW_ERROR_INVALID_ARGU,         ///< invalid argument(parameter)
    PW_ERROR_NOT_OPENED,           ///< PW not opened
    PW_ERROR_NOT_FOUND,            ///< PW not found, may be removed
    PW_ERROR_IS_MOVING,            ///< PW is moving
    PW_ERROR_POINTER,              ///< invalid pointer, when get some value, do not pass the NULL pointer to the function
    PW_ERROR_OPERATION_FAILED,     ///< operation failed (Usually, this is caused by sending commands too often or removed)
    PW_ERROR_FIRMWARE_ERROR,       ///< firmware error (If you encounter this error, try calling POAResetPW)
} PWErrors;

typedef enum _PWState               ///< PW State Definition
{
    PW_STATE_CLOSED = 0,           ///< PW was closed
    PW_STATE_OPENED,               ///< PW was opened, but not moving(Idle)
    PW_STATE_MOVING                ///< PW is moving
} PWState;

typedef struct _PWProperties        ///< PW Properties Definition
{
    char Name[64];                      ///< the PW name
    int Handle;                         ///< it's unique,PW can be controlled and set by the handle
    int PositionCount;                  ///< the filter capacity, eg: 5-position
    char SN[32];                        ///< the serial number of PW,it's unique

    char Reserved[32];                  ///< reserved
} PWProperties;

#define MAX_NAME_LEN (24) ///< custom name and filter alias max length

/***********************************************************************************************************
 * Here is a simple process to show how to use this SDK to manipulate the phoenix filter wheel(PW):
 *
 * POAGetPWCount(); // get the count of pw
 *
 * POAGetPWProperties(int index, PWProperties *pProp); // get the Handle
 *
 * POAOpenPW(int Handle); // open PW with the Handle
 *
 * POAGetCurrentPosition(int Handle, int *pPosition); // show the current postion
 *
 * POAGotoPosition(int Handle, int position); // goto position
 *
 * POAClosePW(int Handle); // close PW
***********************************************************************************************************/


/**
 * @brief POAGetPWCount: get the count of phoenix filter wheel(PW)
 *
 * @return the counts of phoenix filter wheel connected to the computer host
 */
POA_PW_API  int POAGetPWCount();


/**
 * @brief POAGetPWProperties: get the property of the connected PW, NO need to open the PW for this operation
 *
 * @param index (input), the range: [0, PW count), note: index is not the Handle
 *
 * @param pProp (output), pointer to PWProperties structure, PWProperties structure needs to malloc memory first
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pProp is NULL pointer
 *         PW_ERROR_INVALID_INDEX: no PW connected or index value out of boundary
 */
POA_PW_API  PWErrors POAGetPWProperties(int index, PWProperties *pProp);


/**
 * @brief POAGetPWPropertiesByHandle: get the property of the connected PW by Handle, it's a convenience function to get the property of the known PW Handle
 *
 * @param Handle (input), get from in the PWProperties structure
 *
 * @param pProp (output), pointer to PWProperties structure, PWProperties structure needs to malloc memory first
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pProp is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 */
POA_PW_API  PWErrors POAGetPWPropertiesByHandle(int Handle, PWProperties *pProp);


/**
 * @brief POAOpenPW open the PW, note: the following API functions need to open the PW first, NOTE: When opened successfully, phoenix filter wheel will goto 1st position.
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_FOUND: this PW was not found and may have been removed
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAOpenPW(int Handle);


/**
 * @brief POAClosePW: close the PW
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 */
POA_PW_API  PWErrors POAClosePW(int Handle);


/**
 * @brief POAGetCurrentPosition: Get the Current Position of PW
 *
 * @param Handle  (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @param pPosition (output), a pointer to integer for geting the current position
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pPosition is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_IS_MOVING: the PW is moving
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAGetCurrentPosition(int Handle, int *pPosition);


/**
 * @brief POAGotoPosition: Let the PW goto a filter position
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @param position (input), the filter position, range:[0, PositionCount-1]
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_INVALID_ARGU: invalid argument, may be the position out of the range
 *         PW_ERROR_IS_MOVING: the PW is moving
 *         PW_ERROR_FIRMWARE_ERROR: firmware error, please call POAResetPW
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAGotoPosition(int Handle, int position);


/**
 * @brief POAGetOneWay: Get the rotation of Filter Wheel is unidirectional
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @param pIsOneWay (output), a pointer to int value, NOTE: 0 means not one-way, 1 means one-way
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pIsOneWay is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 */
POA_PW_API  PWErrors POAGetOneWay(int Handle, int *pIsOneWay);


/**
 * @brief POASetOneWay: Set the rotation of Filter Wheel to unidirectional
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param isOneWay (output), NOTE: 0 means not one-way, non-0 means one-way
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 */
POA_PW_API  PWErrors POASetOneWay(int Handle, int isOneWay);


/**
 * @brief POAGetPWState: get the current state of PW, this is for easy access to whether PW is moving
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @param pPWState (output), a pointer to PWState enum for geting the current state
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pPWState is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_OPERATION_FAILED: operation failed,may be the PW removed
 */
POA_PW_API  PWErrors POAGetPWState(int Handle, PWState *pPWState);


/**
 * @brief POAGetPWFilterAlias: Get the Filter alias from PW(eg: R, G, or IR-cut)
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param position (input), the filter position, range:[0, PositionCount-1]
 * @param pAliasBuf (output), A buffer that stores alias(get from PW)
 * @param bufLen (input), the length of pAliasBuf, NOTE: the length should NOT be less than MAX_FILTER_NAME_LEN(24)
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pAliasBuf is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_INVALID_ARGU: invalid argument, may be the position out of the range
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAGetPWFilterAlias(int Handle, int position, char *pAliasBuf, int bufLen);


/**
 * @brief POASetPWFilterAlias: Set the Filter alias to PW(eg: you can set alias of 1 position to R)
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param position (input), the filter position, range:[0, PositionCount-1]
 * @param pAliasBuf (input), A buffer that stores alias(set to PW), If it's NULL, the previous value will be cleared
 * @param bufLen (input), the length of pAliasBuf, NOTE: the length should be less than MAX_NAME_LEN(24),If it's 0, the previous value will be cleared
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_INVALID_ARGU: invalid argument, may be the position out of the range
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POASetPWFilterAlias(int Handle, int position, const char *pAliasBuf, int bufLen);


/**
 * @brief POAGetPWFocusOffset: Get focus offset form PW(Each filter has different focus offset, using this with Auto Focuser)
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param position (input), the filter position, range:[0, PositionCount-1]
 * @param pFocusOffset (output), a pointer to int value for geting the focus offset
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pFocusOffset is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_INVALID_ARGU: invalid argument, may be the position out of the range
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAGetPWFocusOffset(int Handle, int position, int *pFocusOffset);


/**
 * @brief POASetPWFocusOffset: Set focus offset to PW(Each filter has different focus offset, using this with Auto Focuser)
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param position (input), the filter position, range:[0, PositionCount-1]
 * @param focusOffset (input), the focus offset value
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_INVALID_ARGU: invalid argument, may be the position out of the range
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POASetPWFocusOffset(int Handle, int position, int focusOffset);


/**
 * @brief POAGetPWCustomName: Get the user custom name from PW(eg: MyPW, and the PW Name will show as PhoenixWheel[MyPW])
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param pNameBuf (output), A buffer that stores custom name(get from PW)
 * @param bufLen (input), the length of pNameBuf, NOTE: the length should NOT be less than MAX_FILTER_NAME_LEN(24)
 * @return PW_OK: operation successful
 *         PW_ERROR_POINTER: pNameBuf is NULL pointer
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAGetPWCustomName(int Handle, char *pNameBuf, int bufLen);


/**
 * @brief POASetPWCustomName: Set the user custom name to PW
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 * @param pNameBuf (input), A buffer that stores custom name(set to PW), If it's NULL, the previous value will be cleared
 * @param bufLen (input), the length of pNameBuf, NOTE: the length should be less than MAX_NAME_LEN(24), If it's 0, the previous value will be cleared
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POASetPWCustomName(int Handle, const char *pNameBuf, int bufLen);


/**
 * @brief POAResetPW: Reset the PW, If PW_ERROR_FIRMWARE_ERROR occurs, call this function
 *
 * @param Handle (input), get from in the PWProperties structure, use POAGetPWProperties function
 *
 * @return PW_OK: operation successful
 *         PW_ERROR_INVALID_HANDLE: no PW with this Handle was found or the Handle is out of boundary
 *         PW_ERROR_NOT_OPENED: the PW is not opened
 *         PW_ERROR_OPERATION_FAILED: operation failed
 */
POA_PW_API  PWErrors POAResetPW(int Handle);


/**
 * @brief POAGetPWErrorString: convert PWErrors enum to char *, it is convenient to print or display errors
 *
 * @param err (intput), PWErrors, the value returned by the API function
 *
 * @return point to const char* error
 */
POA_PW_API const char* POAGetPWErrorString(PWErrors err);


/**
 * @brief POAGetPWAPIVer: get the API version
 *
 * @return it's a integer value, eg: 20200202
*/
POA_PW_API int POAGetPWAPIVer();


/**
 * @brief POAGetPWSDKVer: get the sdk version
 *
 * @return point to const char* version, eg: 1.0.11.17
 */
POA_PW_API const char* POAGetPWSDKVer();


#ifdef __cplusplus
}
#endif

#endif //PLAYERONEPW_H
