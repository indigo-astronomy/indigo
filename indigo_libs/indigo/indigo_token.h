// Copyright (c) 2020 Rumen G.Bogdanovski
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
// 2.0 by Rumen G.Bogdanvski <rumenastro@gmail.com>

/** INDIGO token handling
 \file indigo_token.h
 */

#ifndef indigo_token_h
#define indigo_token_h

#define MAX_TOKENS 256

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long long unsigned indigo_token;


/** Convert hex string to indigo_token, 0 for error or no token
 */
indigo_token indigo_string_to_token(const char *token_string);

/** Add device and token to the list, if the device exists, token will be updated
 */
extern bool indigo_add_device_token(const char *device, indigo_token token);

/** Remove device access token from the list
 */
extern bool indigo_remove_device_token(const char *device);

/** Get device access token from the list or 0 if not set
 */
extern indigo_token indigo_get_device_token(const char *device);

/** Get device token if set else get master access token or 0 if none set
 */
extern indigo_token indigo_get_device_or_master_token(const char *device);

/** Get master token
 */
extern indigo_token indigo_get_master_token();

/** Set master token
 */
extern void indigo_set_master_token(indigo_token token);

/** Clear token list
 */
extern void indigo_clear_device_tokens();

/** Read device tokens from file
    NOTE: Existing list will not be removed read tokens will be added or uptaded.
    File format:
    # This is comment
    12345 Rotator Sumulator
    765433 CCD Imager Simulator @ indigosky
    # Set master token ('@' means master token)
    1232 @
 */
extern bool indigo_load_device_tokens_from_file(const char *file_name);

/** Save device tokens to file
 */
extern bool indigo_save_device_tokens_to_file(const char *file_name);

#ifdef __cplusplus
}
#endif

#endif /* indigo_token_h */
