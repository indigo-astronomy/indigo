/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
*
* \class ApgTimer 
* \brief wrapper for cross platform timing 
* 
*/ 
#ifndef APGTIMER_INCLUDE_H__ 
#define APGTIMER_INCLUDE_H__ 

#ifdef __linux__
#include <tr1/memory>
#else
#include <memory>
#endif

class ITimer;

class ApgTimer 
{ 
    public: 
        ApgTimer(); 
        virtual ~ApgTimer(); 

        void Start();
        void Stop();

        double GetTimeInMs();
        double GetTimeInSec();

    private:
				std::shared_ptr<ITimer> m_timer;
}; 

#endif
