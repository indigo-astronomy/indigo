/* Structures for JSON parsing using only fixed-extent memory
 *
 * This file is Copyright (c) 2010-2019 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <ctype.h>
#include <stdbool.h>
#include <sys/time.h>      // for struct timespec

/* the json_type is the type of the C variable the JSON
 * value gets placed in.  It is NOT the JSON type as used
 * in the JSON standard.  But it does partly specify how
 * the JSON value is decoded.
 *
 * For example a t_character must be in quotes, but a t_byte
 * is a bare number. */
typedef enum {t_integer, t_uinteger, t_real,
	      t_string, t_boolean, t_character,
	      t_time,
	      t_object, t_structobject, t_array,
	      t_check, t_ignore,
	      t_short, t_ushort, t_byte, t_ubyte}
    json_type;

struct json_enum_t {
    char	*name;
    int		value;
};

struct json_array_t {
    json_type element_type;
    union {
	struct {
	    const struct json_attr_t *subtype;
	    char *base;
	    size_t stride;
	} objects;
	struct {
	    char **ptrs;
	    char *store;
	    int storelen;
	} strings;
	struct {
	    int *store;
	} bytes;
	struct {
	    unsigned int *store;
	} ubytes;
	struct {
	    int *store;
	} integers;
	struct {
	    unsigned int *store;
	} uintegers;
	struct {
	    short *store;
	} shorts;
	struct {
	    unsigned short *store;
	} ushorts;
	struct {
	    double *store;
	} reals;
	struct {
	    bool *store;
	} booleans;
	struct {
	    struct timespec *store;
	} timespecs;
    } arr;
    int *count, maxlen;
};

struct json_attr_t {
    char *attribute;
    json_type type;
    union {
	bool *boolean;
	char *byte;
	char *character;
	char *string;
	double *real;
	int *integer;
	short *shortint;
	size_t offset;
	struct json_array_t array;
	unsigned char *ubyte;
	unsigned int *uinteger;
	unsigned short *ushortint;
        struct timespec *ts;
    } addr;
    union {
	bool boolean;
	char byte;
	char character;
	char *check;
	double real;
	int integer;
	short shortint;
	unsigned char ubyte;
	unsigned int uinteger;
	unsigned short ushortint;
        struct timespec ts;
    } dflt;
    size_t len;
    const struct json_enum_t *map;
    bool nodefault;
};

#define JSON_ATTR_MAX	31	/* max chars in JSON attribute name */
#define JSON_VAL_MAX	512	/* max chars in JSON value part */

#ifdef __cplusplus
extern "C" {
#endif
int json_read_object(const char *, const struct json_attr_t *,
		     const char **);
int json_read_array(const char *, const struct json_array_t *,
		    const char **);
const char *json_error_string(int);

void json_enable_debug(int, FILE *);
#ifdef __cplusplus
}
#endif

#define JSON_ERR_OBSTART	1	/* non-WS when expecting object start */
#define JSON_ERR_ATTRSTART	2	/* non-WS when expecting attrib start */
#define JSON_ERR_BADATTR	3	/* unknown attribute name */
#define JSON_ERR_ATTRLEN	4	/* attribute name too long */
#define JSON_ERR_NOARRAY	5	/* saw [ when not expecting array */
#define JSON_ERR_NOBRAK 	6	/* array element specified, but no [ */
#define JSON_ERR_STRLONG	7	/* string value too long */
#define JSON_ERR_TOKLONG	8	/* token value too long */
#define JSON_ERR_BADTRAIL	9	/* garbage while expecting comma or } or ] */
#define JSON_ERR_ARRAYSTART	10	/* didn't find expected array start */
#define JSON_ERR_OBJARR 	11	/* error while parsing object array */
#define JSON_ERR_SUBTOOLONG	12	/* too many array elements */
#define JSON_ERR_BADSUBTRAIL	13	/* garbage while expecting array comma */
#define JSON_ERR_SUBTYPE	14	/* unsupported array element type */
#define JSON_ERR_BADSTRING	15	/* error while string parsing */
#define JSON_ERR_CHECKFAIL	16	/* check attribute not matched */
#define JSON_ERR_NOPARSTR	17	/* can't support strings in parallel arrays */
#define JSON_ERR_BADENUM	18	/* invalid enumerated value */
#define JSON_ERR_QNONSTRING	19	/* saw quoted value when expecting nonstring */
#define JSON_ERR_NONQSTRING	19	/* didn't see quoted value when expecting string */
#define JSON_ERR_MISC		20	/* other data conversion error */
#define JSON_ERR_BADNUM		21	/* error while parsing a numerical argument */
#define JSON_ERR_NULLPTR	22	/* unexpected null value or attribute pointer */
#define JSON_ERR_NOCURLY	23	/* object element specified, but no { */

/*
 * Use the following macros to declare template initializers for structobject
 * arrays.  Writing the equivalents out by hand is error-prone.
 *
 * STRUCTOBJECT takes a structure name s, and a fieldname f in s.
 *
 * STRUCTARRAY takes the name of a structure array, a pointer to a an
 * initializer defining the subobject type, and the address of an integer to
 * store the length in.
 */
#define STRUCTOBJECT(s, f)	.addr.offset = offsetof(s, f)
#define STRUCTARRAY(a, e, n) \
	.addr.array.element_type = t_structobject, \
	.addr.array.arr.objects.subtype = e, \
	.addr.array.arr.objects.base = (char*)a, \
	.addr.array.arr.objects.stride = sizeof(a[0]), \
	.addr.array.count = n, \
	.addr.array.maxlen = NITEMS(a)

/* json.h ends here */
