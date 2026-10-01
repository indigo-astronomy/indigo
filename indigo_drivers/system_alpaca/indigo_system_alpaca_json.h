// Copyright (c) 2026 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO ASCOM Alpaca client driver, JSON parser
 \file indigo_system_alpaca_json.h
 */

#ifndef system_alpaca_json_h
#define system_alpaca_json_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Deepest accepted nesting of arrays and objects. Alpaca replies need 4.
 */
#ifndef ALPACA_JSON_MAX_DEPTH
#define ALPACA_JSON_MAX_DEPTH							32
#endif

/** Longest text accepted by alpaca_json_parse(), in bytes. The DOM needs up to 20 bytes of memory per byte of text.
 */
#ifndef ALPACA_JSON_MAX_SIZE
#define ALPACA_JSON_MAX_SIZE							(4 * 1024 * 1024)
#endif

/** Longest text accepted by alpaca_json_parse_image(), in bytes.
 */
#ifndef ALPACA_JSON_IMAGE_MAX_SIZE
#define ALPACA_JSON_IMAGE_MAX_SIZE				0x7FFFFFFF
#endif

/** Size of the ErrorMessage copy in alpaca_json_image, including the terminating NUL.
 */
#define ALPACA_JSON_ERROR_MESSAGE_SIZE		256

/** Value types. ALPACA_JSON_NONE is reported for a NULL value, e.g. a member that does not exist.
 */
typedef enum {
	ALPACA_JSON_NONE = 0,
	ALPACA_JSON_NULL,
	ALPACA_JSON_BOOL,
	ALPACA_JSON_NUMBER,
	ALPACA_JSON_STRING,
	ALPACA_JSON_ARRAY,
	ALPACA_JSON_OBJECT
} alpaca_json_type;

/** DOM node. The whole document is a single allocation owned by the root value.
 */
typedef struct alpaca_json_value alpaca_json_value;

/** Parse a JSON text of the given length (it does not have to be NUL terminated) into a DOM.

 The text is a single JSON value of any type (RFC 8259), optionally preceded by a UTF-8 BOM and surrounded
 by whitespace; anything else after the value is an error. Strings are unescaped to UTF-8, a raw byte that
 is not part of a valid UTF-8 sequence is replaced by '?', so every string of the DOM is valid UTF-8.
 Numbers are parsed independently of the locale. Texts longer than ALPACA_JSON_MAX_SIZE or nested deeper than
 ALPACA_JSON_MAX_DEPTH are rejected; the parser does not recurse.

 Returns the root value, to be released with alpaca_json_free(), or NULL on failure. On failure error (if not
 NULL) is filled with "<message> at offset <byte offset>".
 */
extern alpaca_json_value *alpaca_json_parse(const char *text, size_t length, char *error, int error_size);

/** Release a document. Only the root value returned by alpaca_json_parse() may be passed, NULL is ignored.
 */
extern void alpaca_json_free(alpaca_json_value *root);

/** All accessors below accept NULL as the value and then report ALPACA_JSON_NONE, false, 0 or NULL, so that
 lookups can be chained without intermediate checks. Returned values and strings belong to the document.
 */

/** Type of the value, ALPACA_JSON_NONE for NULL.
 */
extern alpaca_json_type alpaca_json_get_type(const alpaca_json_value *value);

/** True if the value exists and is the JSON null.
 */
extern bool alpaca_json_is_null(const alpaca_json_value *value);

/** Number of elements of an array or of members of an object, 0 for anything else.
 */
extern int alpaca_json_size(const alpaca_json_value *value);

/** Object member with the given name compared without regard to ASCII case (as ASCOM clients do), or NULL.
 If the object has several matching members, the first one is returned.
 */
extern const alpaca_json_value *alpaca_json_get(const alpaca_json_value *object, const char *name);

/** Object member with exactly the given name, or NULL.
 */
extern const alpaca_json_value *alpaca_json_get_exact(const alpaca_json_value *object, const char *name);

/** Array element (or object member) with the given index, or NULL. The cost is proportional to the index,
 use alpaca_json_first() and alpaca_json_next() to walk long arrays.
 */
extern const alpaca_json_value *alpaca_json_array_get(const alpaca_json_value *array, int index);

/** First element of an array or first member of an object, or NULL.
 */
extern const alpaca_json_value *alpaca_json_first(const alpaca_json_value *container);

/** Next element or member of the same array or object, or NULL.
 */
extern const alpaca_json_value *alpaca_json_next(const alpaca_json_value *value);

/** Name of the value if it is an object member, NULL otherwise.
 */
extern const char *alpaca_json_get_name(const alpaca_json_value *value);

/** The typed getters return false and leave *result untouched if the value is missing or has another type;
 nothing is coerced between booleans, numbers and strings. result may be NULL to check the type only.
 */

/** Value of true or false.
 */
extern bool alpaca_json_get_bool(const alpaca_json_value *value, bool *result);

/** Integer value of a number. Succeeds for a number written as an integer that fits into int64_t and for
 a number written with a fraction or an exponent whose value is integral and not above 2^53 in magnitude
 (123.0, 1e3). Fails for 1.5 and for integers that do not fit.
 */
extern bool alpaca_json_get_int(const alpaca_json_value *value, int64_t *result);

/** Value of any number, an integer included.
 */
extern bool alpaca_json_get_double(const alpaca_json_value *value, double *result);

/** Unescaped, NUL terminated UTF-8 text of a string.
 */
extern bool alpaca_json_get_string(const alpaca_json_value *value, const char **result);

/** Length of a string in bytes, which differs from strlen() if the string contains \u0000; 0 for anything else.
 */
extern size_t alpaca_json_get_string_length(const alpaca_json_value *value);

/** Element type of the buffer filled by alpaca_json_parse_image().
 */
typedef enum {
	ALPACA_JSON_IMAGE_INT32 = 0,
	ALPACA_JSON_IMAGE_DOUBLE
} alpaca_json_image_format;

/** Result of alpaca_json_parse_image().
 */
typedef struct {
	bool has_value;															///< Value is present and is an array, the buffer is filled
	int type;																		///< Type member (1 Int16, 2 Int32, 3 Double), 0 if missing
	int rank;																		///< Rank member, or the rank found in Value if Rank is missing, 0 if neither
	int dimensions[3];													///< sizes of Value[x][y][plane], dimensions[2] is 0 for rank 2
	size_t count;																///< number of elements stored in the buffer
	double minimum;															///< smallest stored element
	double maximum;															///< largest stored element
	size_t rounded;															///< number of elements with a fraction rounded to int32
	bool has_error_number;											///< ErrorNumber is present and is an integer
	int64_t error_number;												///< ErrorNumber, 0 if missing
	char error_message[ALPACA_JSON_ERROR_MESSAGE_SIZE];	///< ErrorMessage, truncated at a character boundary, "" if missing
	bool has_client_transaction_id;							///< ClientTransactionID is present and fits into uint32_t
	uint32_t client_transaction_id;							///< ClientTransactionID, 0 if missing
	bool has_server_transaction_id;							///< ServerTransactionID is present and fits into uint32_t
	uint32_t server_transaction_id;							///< ServerTransactionID, 0 if missing
} alpaca_json_image;

/** Scan the reply to the Alpaca imagearray request without building a DOM.

 The text is the envelope object {"Type":2,"Rank":2,"Value":[[...],...],"ErrorNumber":0,"ErrorMessage":"",
 "ClientTransactionID":1,"ServerTransactionID":2} with the members in any order and with names compared
 without regard to case. Unknown members are validated and skipped; a known scalar member of an unexpected
 type (e.g. null) is treated as missing.

 Value has to be a rectangular array of rank 2 or 3 containing numbers only. Its elements are stored in the
 order of transmission, element Value[x][y][plane] at index (x * dimensions[1] + y) * dimensions[2] + plane
 (x * dimensions[1] + y for rank 2). An empty or ragged array, rank 1 or above 3, mixed nesting, a Rank member
 that does not match the array and a second Value array are errors.

 Format ALPACA_JSON_IMAGE_INT32 stores int32_t: integers and numbers with a zero fraction (123.0, 1.2e2) are
 stored exactly, a number with a fraction (as a Type 3 reply may contain) is rounded to the nearest integer
 and counted in image->rounded, a value that does not fit into int32_t is an error. Format
 ALPACA_JSON_IMAGE_DOUBLE stores every number as double. The caller chooses the format before the Type
 member is known; INT32 is the right choice for camera data, the rounded count tells if precision was lost.

 *buffer and *capacity (in elements) describe the target buffer. If growable is true, the buffer is
 reallocated with indigo_safe_realloc() as needed (*buffer may be NULL with *capacity 0) and both are updated;
 otherwise a Value that does not fit is an error. The buffer always stays owned by the caller, also on failure.

 Returns true if the envelope is well formed, then the caller checks image->error_number first and
 image->has_value next (Value may be missing or null, typically together with an error). Returns false and
 fills error (if not NULL) with "<message> at offset <byte offset>" otherwise; the content of image and of
 the buffer is undefined then.
 */
extern bool alpaca_json_parse_image(const char *text, size_t length, alpaca_json_image_format format, void **buffer, size_t *capacity, bool growable, alpaca_json_image *image, char *error, int error_size);

#ifdef __cplusplus
}
#endif

#endif /* system_alpaca_json_h */
