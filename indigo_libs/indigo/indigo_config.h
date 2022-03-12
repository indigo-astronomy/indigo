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

/** INDIGO Build configuration
 \file indigo_config.h
 */

#ifndef indigo_config_h
#define indigo_config_h

/** INDIGO Build number
 */
#define INDIGO_BUILD "171"

/** Conditional compilation wrapper for TRACE log level
 */
#define INDIGO_TRACE(c) c

/** Conditional compilation wrapper for DEBUG log level
 */
#define INDIGO_DEBUG(c) c

/** Conditional compilation wrapper for ERROR log level
 */
#define INDIGO_ERROR(c) c

/** Conditional compilation wrapper for LOG log level
 */
#define INDIGO_LOG(c) c

/** Conditional compilation wrapper for TRACE log level (for wire protocol adapters)
 */
#define INDIGO_TRACE_PROTOCOL(c) c

/** Conditional compilation wrapper for TRACE log level (for wire protocol parsers)
 */
#define INDIGO_TRACE_PARSER(c)

/** Conditional compilation wrapper for DEBUG log level (for wire protocol adapters)
 */
#define INDIGO_DEBUG_PROTOCOL(c) c

/** Conditional compilation wrapper for TRACE log level (for drivers)
 */
#define INDIGO_TRACE_DRIVER(c) c

/** Conditional compilation wrapper for DEBUG log level (for drivers)
 */
#define INDIGO_DEBUG_DRIVER(c) c

#endif /* indigo_config_h */
