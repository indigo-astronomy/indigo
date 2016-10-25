//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Build configuration
 \file indigo_config.h
 */

#ifndef indigo_config_h
#define indigo_config_h

/** Current framework and protocol version
 */
#define INDIGO_VERSION_CURRENT 0x0200

/** INDIGO Build number
 */
#define INDIGO_BUILD           0

/** Conditional compilation wrapper for TRACE log level
 */
#define INDIGO_TRACE(c)

/** Conditional compilation wrapper for DEBUG log level
 */
#define INDIGO_DEBUG(c)

/** Conditional compilation wrapper for ERROR log level
 */
#define INDIGO_ERROR(c) c

/** Conditional compilation wrapper for LOG log level
 */
#define INDIGO_LOG(c) c

/** Conditional compilation wrapper for TRACE log level (for wire protocol adapters)
 */
#define INDIGO_TRACE_PROTOCOL(c)

/** Conditional compilation wrapper for DEBUG log level (for wire protocol adapters)
 */
#define INDIGO_DEBUG_PROTOCOL(c)

// #define INDIG_USE_SYSLOG

#endif /* indigo_config_h */

