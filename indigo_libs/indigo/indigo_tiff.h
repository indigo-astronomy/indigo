// Copyright (c) 2020 CloudMakers, s. r. o.
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

#ifndef indigo_tiff_h
#define indigo_tiff_h

#include <stdio.h>
#include <tiffio.h>

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

typedef struct {
		unsigned char *data;
		tsize_t size;
		tsize_t file_length;
		toff_t file_offset;
} indigo_tiff_memory_handle;

INDIGO_EXTERN tsize_t indigo_tiff_read(thandle_t handle, tdata_t data, tsize_t size);
INDIGO_EXTERN tsize_t indigo_tiff_write(thandle_t handle, tdata_t data, tsize_t size);
INDIGO_EXTERN toff_t indigo_tiff_seek(thandle_t handle, toff_t off, int whence);
INDIGO_EXTERN int indigo_tiff_close(thandle_t handle);
INDIGO_EXTERN toff_t indigo_tiff_size(thandle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* indigo_tiff_h */
