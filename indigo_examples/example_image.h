// Copyright (c) 2020-2025 CloudMakers, s. r. o. & Rumen G. Bogdanovski
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
//      & Rumen G. Bogdanovski <rumenastro@gmail.com>

#ifndef EXAMPLE_IMAGE_H
#define EXAMPLE_IMAGE_H
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <indigo/indigo_bus.h>

static bool example_save_image(const char *name, const void *data, size_t size) {
	FILE *file = fopen(name, "wb");
	if (file == NULL) {
		indigo_error("Cannot open %s: %s", name, strerror(errno));
		return false;
	}
	errno = 0;
	bool saved = fwrite(data, 1, size, file) == size;
	int error = errno;
	if (fclose(file) != 0) {
		saved = false;
		error = errno;
	}
	if (!saved) {
		indigo_error("Cannot save %s: %s", name, strerror(error ? error : EIO));
		return false;
	}
	indigo_log("image saved to %s...", name);
	return true;
}
#endif
