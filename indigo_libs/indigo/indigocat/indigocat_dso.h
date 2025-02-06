// Copyright (c) 2019 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO DSO data
 \file indigocat_dso.h
 */

#ifndef indigocat_dso_h
#define indigocat_dso_h

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

typedef enum {
	GALAXY,
	GALAXY_PAIR,
	GALAXY_TRIPLET,
	OPEN_CLUSTER,
	GLOBULAR_CLUSTER,
	NEBULA,
	PLANETARY_NEBULA,
	REFLECTION_NEBULA,
	EMISSION_NEBULA,
	SUPERNOVA_REMNANT,
	HII_REGION,
	ASSOCIATION_OF_STARS,
	STAR_CLUSTER_NEBULA,
	GROUP_OF_GALAXIES,
	NOVA_STAR
} indigocat_dso_type;

typedef struct {
	char *id;
	char type;
	double ra, dec;
	float mag, r1, r2, angle;
	char *name;
	double ra_now, dec_now;
} indigocat_dso_entry;

INDIGO_EXTERN char *indigocat_dso_type_description[];

INDIGO_EXTERN indigocat_dso_entry *indigocat_get_dso_data(void);

#endif /* indigocat_dso_h */
