/*
 * 1394-Based Digital Camera Control Library
 *
 * Written by Rodolphe Pineau <pineau@rti-zone.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "control.h"
#include "platform.h"
#include "internal.h"
#include "macosx.h"

#include <CoreFoundation/CoreFoundation.h>

int dc1394_capture_schedule_with_runloop (dc1394camera_t * camera,
        CFRunLoopRef run_loop, CFStringRef run_loop_mode)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->capture_schedule_with_runloop)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    return d->capture_schedule_with_runloop (cpriv->pcam, run_loop, run_loop_mode);
}
