# INDIGO Raw Image Format
Revision: 13.01.2024 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Overview
INDIGO raw image format is intended to be used internally by INDIGO mostly by the agents, for focusing, guiding etc. It is uncompressed binary format that supports several pixel formats.

## Data structure
The INDIGO raw format consists of three sections **Image Header**, **Image data** and optionally **Metadata**. The image header and the image data are in little endian binary format and the extension is in ASCII text format. Here are the possible RAW image layouts:

* <**Image Header**><**Image data**><**Metadata**><EOF>

* <**Image Header**><**Image data**><EOF>


### Image Header
Image header is mandatory and is 12 bytes long. It contains the pixel format info and the image dimensions as follows:
```C
typedef struct {
	uint32_t signature;
	uint32_t width;
	uint32_t height;
} indigo_raw_header;
```
The *signature* interpreted in ASCII starts with "RAW" and the 4th byte is information about the pixel format. Next 8 bytes are image *width* and *height*.

### Image Data
Image data is mandatory with length determined by *width*, *height* and the pixel depth. Image data format and pixel depth is determined by the *signature*.

Valid signatures are:
```C
typedef enum {
	INDIGO_RAW_MONO8 = 0x31574152,
	INDIGO_RAW_MONO16 = 0x32574152,
	INDIGO_RAW_RGB24 = 0x33574152,
	INDIGO_RAW_RGBA32 = 0x36424752,
	INDIGO_RAW_ABGR32 = 0x36524742,
	INDIGO_RAW_RGB48 = 0x36574152
} indigo_raw_type;
```

* **INDIGO_RAW_MONO8** - Unsigned 1 byte per pixel array. Data size is *width x height x 1* bytes.
* **INDIGO_RAW_MONO16** - Little endian, unsigned 2 bytes per pixel array. Data size is *width x height x 2* bytes.
* **INDIGO_RAW_RGB24** - Unsigned 3 bytes per pixel array. Byte 1 is Red, byte 2 is Green and byte 3 is Blue. Data size is *width x height x 3* bytes.
* **INDIGO_RAW_RGBA32** - Unsigned 4 bytes per pixel array. Byte 1 is Red, byte 2 is Green, byte 3 is Blue and byte 4 is Alpha. Data size is *width x height x 4* bytes.
* **INDIGO_RAW_ABGR32** - Unsigned 4 bytes per pixel array. Byte 1 is Alpha, byte 2 is Blue, byte 3 is Green and byte 4 is Red. Data size is *width x height x 4* bytes.
* **INDIGO_RAW_RGB48** - Little endian, unsigned 6 bytes per pixel array. The first 2 bytes represent the Red value, the second 2 bytes represent Green, and the third 2 bytes represent Green. Data size is *width x height x 6* bytes.

### Image Metadata
Image metadata is optional. It is an ASCII extension to store additional information using standard FITS keywords. Its presence is indicated by "SIMPLE=T" card right after the end of the **Image data**. Keywords are separated by semicolon (;) and do not need to follow strictly the FITS card format. Heading and trailing spaces are skipped and cards are variable length.

For example:
```
<Image Header><Image data>SIMPLE=T;BAYERPAT='RGGB';<EOF>
```  
