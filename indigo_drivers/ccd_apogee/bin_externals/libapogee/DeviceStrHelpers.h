/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class DeviceStrHelpers 
* \brief namespace to assit with parsing device strings 
* 
*/ 


#ifndef DEVICESTRHELPERS_INCLUDE_H__ 
#define DEVICESTRHELPERS_INCLUDE_H__ 

#include <vector>
#include <string>
#include <stdint.h>

namespace DeviceStr
{ 
    std::vector<std::string> GetVect(std::string data);
    std::vector<std::string> GetCameras(std::string data );
    std::string GetType(const std::string & data);
    std::string GetName(const std::string & data);
    std::string GetAddr(const std::string & data);
    std::string GetPort(const std::string & data);
    std::string GetInterface(const std::string & data);
    uint16_t GetFwVer(const std::string & data);
}; 

#endif
