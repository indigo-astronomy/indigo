// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO Version mapping
 \file indigo_version.h
 */

#ifndef indigo_version_h
#define indigo_version_h

#include <indigo/indigo_bus.h>

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Copy name into property definition, translate if version doesn't match.
 */
INDIGO_EXTERN void indigo_copy_property_name(indigo_version version, indigo_property *property, const char *name);

/** Copy name into item definition, translate if version doesn't match.
 */
INDIGO_EXTERN void indigo_copy_item_name(indigo_version version, indigo_property *property, indigo_item *item, const char *name);

/** Get property name, translate if version doesn't match.
 */
INDIGO_EXTERN const char *indigo_property_name(indigo_version version, indigo_property *property);

/** Get item name, translate if version doesn't match.
 */
INDIGO_EXTERN const char *indigo_item_name(indigo_version version, indigo_property *property, indigo_item *item);

#ifdef __cplusplus
}
#endif

#endif /* indigo_version_h */

