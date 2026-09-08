// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

#ifndef ccd_touptek_vendor_h
#define ccd_touptek_vendor_h

#if defined(ALTAIR)

#define ENTRY_POINT						indigo_ccd_altair
#define CAMERA_NAME_PREFIX		"Altair"
#define DRIVER_LABEL					"AltairAstro Camera"
#define DRIVER_NAME						"indigo_ccd_altair"
#define DRIVER_PRIVATE_DATA		altair_private_data

#define SDK_CALL(x)						Altaircam_##x
#define SDK_DEF(x)						ALTAIRCAM_##x
#define SDK_TYPE(x)						Altaircam##x
#define SDK_HANDLE						HAltaircam

#include <altaircam.h>
#include "../ccd_altair/indigo_ccd_altair.h"

#elif defined(BACCAM)
#define ENTRY_POINT						indigo_ccd_baccam
#define CAMERA_NAME_PREFIX		"BacCam"
#define DRIVER_LABEL					"BacCam Camera"
#define DRIVER_NAME						"indigo_ccd_baccam"
#define DRIVER_PRIVATE_DATA		baccam_private_data

#define SDK_CALL(x)						Baccam_##x
#define SDK_DEF(x)						BACCAM_##x
#define SDK_TYPE(x)						Baccam##x
#define SDK_HANDLE						HBaccam

#include <baccam.h>
#include "../ccd_baccam/indigo_ccd_baccam.h"

#elif defined(BRESSER)

#define ENTRY_POINT						indigo_ccd_bresser
#define CAMERA_NAME_PREFIX		"Bresser"
#define DRIVER_LABEL					"Bresser Camera"
#define DRIVER_NAME						"indigo_ccd_bresser"
#define DRIVER_PRIVATE_DATA		bresser_private_data

#define SDK_CALL(x)						Bressercam_##x
#define SDK_DEF(x)						BRESSERCAM_##x
#define SDK_TYPE(x)						Bressercam##x
#define SDK_HANDLE						HBressercam

#include <bressercam.h>
#include "../ccd_bresser/indigo_ccd_bresser.h"

#elif defined(OMEGONPRO)

#define ENTRY_POINT						indigo_ccd_omegonpro
#define CAMERA_NAME_PREFIX		"Omegon"
#define DRIVER_LABEL					"OmegonPro Camera"
#define DRIVER_NAME						"indigo_ccd_omegonpro"
#define DRIVER_PRIVATE_DATA		omegonpro_private_data

#define SDK_CALL(x)						Omegonprocam_##x
#define SDK_DEF(x)						OMEGONPROCAM_##x
#define SDK_TYPE(x)						Omegonprocam##x
#define SDK_HANDLE						HOmegonprocam

#include <omegonprocam.h>
#include "../ccd_omegonpro/indigo_ccd_omegonpro.h"

#elif defined(STARSHOOTG)

#define ENTRY_POINT						indigo_ccd_ssg
#define CAMERA_NAME_PREFIX		"Orion"
#define DRIVER_LABEL					"Orion StarShot G Camera"
#define DRIVER_NAME						"indigo_ccd_ssg"
#define DRIVER_PRIVATE_DATA		ssg_private_data

#define SDK_CALL(x)						Starshootg_##x
#define SDK_DEF(x)						STARSHOOTG_##x
#define SDK_TYPE(x)						Starshootg##x
#define SDK_HANDLE						HStarshootg

#include <starshootg.h>
#include "../ccd_ssg/indigo_ccd_ssg.h"

#elif defined(RISING)

#define ENTRY_POINT						indigo_ccd_rising
#define CAMERA_NAME_PREFIX		"RisingCam"
#define DRIVER_LABEL					"RisingCam Camera"
#define DRIVER_NAME						"indigo_ccd_rising"
#define DRIVER_PRIVATE_DATA		rising_private_data

#define SDK_CALL(x)						Nncam_##x
#define SDK_DEF(x)						NNCAM_##x
#define SDK_TYPE(x)						Nncam##x
#define SDK_HANDLE						HNncam

#include <nncam.h>
#include "../ccd_rising/indigo_ccd_rising.h"

#elif defined(MALLIN)

#define ENTRY_POINT						indigo_ccd_mallin
#define CAMERA_NAME_PREFIX		"MallinCam"
#define DRIVER_LABEL					"MallinCam Camera"
#define DRIVER_NAME						"indigo_ccd_mallin"
#define DRIVER_PRIVATE_DATA		mallin_private_data

#define SDK_CALL(x)						Mallincam_##x
#define SDK_DEF(x)						MALLINCAM_##x
#define SDK_TYPE(x)						Mallincam##x
#define SDK_HANDLE						HMallincam

#include <mallincam.h>
#include "../ccd_mallin/indigo_ccd_mallin.h"

#elif defined(MEADE)

#define ENTRY_POINT						indigo_ccd_meade
#define CAMERA_NAME_PREFIX		"Meade"
#define DRIVER_LABEL					"Meade Camera"
#define DRIVER_NAME						"indigo_ccd_meade"
#define DRIVER_PRIVATE_DATA		meade_private_data

#define SDK_CALL(x)						Toupcam_##x     // Strange - Meade cameras use Toupcam prefix
#define SDK_DEF(x)						TOUPCAM_##x
#define SDK_TYPE(x)						Toupcam##x
#define SDK_HANDLE						HToupCam

#include <meadecam.h>
#include "../ccd_meade/indigo_ccd_meade.h"

#elif defined(OGMA)

#define ENTRY_POINT						indigo_ccd_ogma
#define CAMERA_NAME_PREFIX		"OGMA"
#define DRIVER_LABEL					"OGMA Camera"
#define DRIVER_NAME						"indigo_ccd_ogma"
#define DRIVER_PRIVATE_DATA		ogma_private_data

#define SDK_CALL(x)						Ogmacam_##x
#define SDK_DEF(x)						OGMACAM_##x
#define SDK_TYPE(x)						Ogmacam##x
#define SDK_HANDLE						HOgmacam

#include <ogmacam.h>
#include "../ccd_ogma/indigo_ccd_ogma.h"

#elif defined(SVBONY)

#define ENTRY_POINT						indigo_ccd_svb2
#define CAMERA_NAME_PREFIX		"SvBony"
#define DRIVER_LABEL					"SVBONY (OEM) Camera"
#define DRIVER_NAME						"indigo_ccd_svb2"
#define DRIVER_PRIVATE_DATA		svb2_private_data

#define SDK_CALL(x)						Svbonycam_##x
#define SDK_DEF(x)						SVBONYCAM_##x
#define SDK_TYPE(x)						Svbonycam##x
#define SDK_HANDLE						HSvbonycam

#include <svbonycam.h>
#include "../ccd_svb2/indigo_ccd_svb2.h"

#else

#define TOUPTEK

#define ENTRY_POINT						indigo_ccd_touptek
#define CAMERA_NAME_PREFIX		"Touptek"
#define DRIVER_LABEL					"Touptek Camera"
#define DRIVER_NAME						"indigo_ccd_touptek"
#define DRIVER_PRIVATE_DATA		touptek_private_data

#define SDK_CALL(x)						Toupcam_##x
#define SDK_DEF(x)						TOUPCAM_##x
#define SDK_TYPE(x)						Toupcam##x
#define SDK_HANDLE						HToupCam

#include <toupcam.h>
#include "../ccd_touptek/indigo_ccd_touptek.h"

#endif

#endif
