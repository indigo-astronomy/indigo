// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Rigel Systems nSTEP focuser driver
 \file indigo_focuser_nstep.h
 */

#ifndef focuser_nstep_h
#define focuser_nstep_h

#include "indigo_driver.h"
#include "indigo_focuser_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Register nSTEP focuser hot-plug callback
 */

extern indigo_result indigo_focuser_nstep(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* focuser_nstep_h */

