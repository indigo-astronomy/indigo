//
//  indigo_tiff.c
//  indigo
//
//  Created by Peter Polakovic on 26/10/2020.
//  Copyright © 2020 CloudMakers, s. r. o. All rights reserved.
//

#include <string.h>
#include <stdlib.h>

#include <indigo/indigo_tiff.h>

tsize_t indigo_tiff_read(thandle_t handle, tdata_t data, tsize_t size) {
	indigo_tiff_memory_handle *memory_handle = (indigo_tiff_memory_handle *)handle;
	tsize_t length;
	if ((memory_handle->file_offset + size) <= memory_handle->file_length)
		length = size;
	else
		length = memory_handle->file_length - memory_handle->file_offset;
	memcpy(data, memory_handle->data + memory_handle->file_offset, length);
	memory_handle->file_offset += length;
	return length;
}

tsize_t indigo_tiff_write(thandle_t handle, tdata_t data, tsize_t size) {
	indigo_tiff_memory_handle *memory_handle = (indigo_tiff_memory_handle *)handle;
	if ((memory_handle->file_offset + size) > memory_handle->size) {
		memory_handle->data = (unsigned char *) realloc(memory_handle->data, memory_handle->file_offset + size);
		memory_handle->size = memory_handle->file_offset + size;
	}
	memcpy(memory_handle->data + memory_handle->file_offset, data, size);
	memory_handle->file_offset += size;
	if (memory_handle->file_offset > memory_handle->file_length)
		memory_handle->file_length = memory_handle->file_offset;
	return size;
}

toff_t indigo_tiff_seek(thandle_t handle, toff_t off, int whence) {
	indigo_tiff_memory_handle *memory_handle = (indigo_tiff_memory_handle *)handle;
	switch (whence) {
		case SEEK_SET: {
			if ((tsize_t) off > memory_handle->size)
				memory_handle->data = (unsigned char *) realloc(memory_handle->data, memory_handle->size += off);
			memory_handle->file_offset = off;
			break;
		}
		case SEEK_CUR: {
			if ((tsize_t)(memory_handle->file_offset + off) > memory_handle->size)
				memory_handle->data = (unsigned char *) realloc(memory_handle->data, memory_handle->size = memory_handle->file_offset + off);
			memory_handle->file_offset += off;
			break;
		}
		case SEEK_END: {
			if ((tsize_t) (memory_handle->file_length + off) > memory_handle->size)
				memory_handle->data = (unsigned char *) realloc(memory_handle->data, memory_handle->size += off);
			memory_handle->file_offset = memory_handle->file_length + off;
			break;
		}
	}
	if (memory_handle->file_offset > memory_handle->file_length)
		memory_handle->file_length = memory_handle->file_offset;
	return memory_handle->file_offset;
}

int indigo_tiff_close(thandle_t handle) {
	return 0;
}

toff_t indigo_tiff_size(thandle_t handle) {
	indigo_tiff_memory_handle *memory_handle = (indigo_tiff_memory_handle *)handle;
	return memory_handle->file_length;
}
