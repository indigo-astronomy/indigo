// Copyright (C) 2020 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen G. Bogdanovski

/** INDIGO Lunatico Armadillo, Platypus etc. focuser driver
 \file indigo_focuser_lunatico.c
 */

#include "indigo_focuser_lunatico.h"

#define DRIVER_ENTRY_POINT       indigo_focuser_lunatico
#define DRIVER_NAME              "indigo_focuser_lunatico"
#define CONFLICTING_DRIVER       "indigo_rotator_lunatico"
#define DRIVER_INFO              "Lunatico Astronomia Focuser"
#define DEFAULT_DEVICE           TYPE_FOCUSER
#define DRIVER_VERSION           0x0008

#include "shared/lunatico_shared.c"

//fake metainfo
//SET_DRIVER_INFO(info, DRIVER_INFO, __FUNCTION__, DRIVER_VERSION, false, last_action);
