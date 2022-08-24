// Copyright (c) 2021 Rumen G.Bogdanvski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

#ifndef indigo_fits_h
#define indigo_fits_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FITS_HEADER_SIZE
#define FITS_HEADER_SIZE 2880
#endif

typedef enum {
	INDIGO_FITS_NUMBER = 1,
	INDIGO_FITS_STRING,
	INDIGO_FITS_LOGICAL
} indigo_fits_keyword_type;

typedef struct {
	indigo_fits_keyword_type type;
	const char *name;
	union {
		double number;
		const char *string;
		bool logical;
	};
	const char *comment;
} indigo_fits_keyword;

extern indigo_result indigo_raw_to_fits(char *image, char **fits, int *size, indigo_fits_keyword *keywords);

#ifdef __cplusplus
}
#endif

#endif /* indigo_guider_utils_h */
