/*
 * 1394-Based Digital Camera Control Library
 *
 * Functions for the manual allocations of ISO ressources.
 *
 * Written by David Moore <dcm@acm.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "iso.h"
#include "platform.h"
#include "internal.h"

dc1394error_t
dc1394_iso_set_persist (dc1394camera_t * camera)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->iso_set_persist)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    dc1394error_t err;
    if ((err = d->iso_set_persist (cpriv->pcam)) != DC1394_SUCCESS)
        return err;

    cpriv->iso_persist = 1;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_allocate_channel (dc1394camera_t * camera,
        uint64_t channels_allowed, int * channel)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->iso_allocate_channel)
        return DC1394_FUNCTION_NOT_SUPPORTED;

    if (channels_allowed == 0) {
        if (camera->bmode_capable)
            channels_allowed = ~((uint64_t)0);
        else
            channels_allowed = 0xffff;
    }

    if ((err = d->iso_allocate_channel (cpriv->pcam, channels_allowed,
            channel)) != DC1394_SUCCESS)
        return err;

    cpriv->allocated_channels |= ((uint64_t)1 << *channel);
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_channel (dc1394camera_t * camera, int channel)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->iso_release_channel)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    dc1394error_t err;
    if ((err = d->iso_release_channel (cpriv->pcam, channel))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_channels &= ~((uint64_t)1 << channel);
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_allocate_bandwidth (dc1394camera_t * camera, int bandwidth_units)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->iso_allocate_bandwidth)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    dc1394error_t err;
    if ((err = d->iso_allocate_bandwidth (cpriv->pcam, bandwidth_units))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_bandwidth += bandwidth_units;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_bandwidth (dc1394camera_t * camera, int bandwidth_units)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    const platform_dispatch_t * d = cpriv->platform->dispatch;
    if (!d->iso_release_bandwidth)
        return DC1394_FUNCTION_NOT_SUPPORTED;
    dc1394error_t err;
    if ((err = d->iso_release_bandwidth (cpriv->pcam, bandwidth_units))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_bandwidth -= bandwidth_units;
    if (cpriv->allocated_bandwidth < 0)
        cpriv->allocated_bandwidth = 0;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_all (dc1394camera_t * camera)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    int i;
    for (i = 0; i < 64; i++)
        if (cpriv->allocated_channels & ((uint64_t)1 << i))
            dc1394_iso_release_channel (camera, i);
    if (cpriv->allocated_bandwidth)
        dc1394_iso_release_bandwidth (camera, cpriv->allocated_bandwidth);

    if (cpriv->allocated_bandwidth || cpriv->allocated_channels)
        return DC1394_FAILURE;

    return DC1394_SUCCESS;
}
