# Saving images from the camera driver

Revision: 05.11.2022 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Enabling camera driver image copies

In INDIGO, saving images from the camera driver is controlled by two separate properties of the camera device:

* **CCD_UPLOAD_MODE** - It controls what to do with the acquired images. There are three items:
	- **CLIENT** - do not save local copy, just provide it to the client
	- **LOCAL** - save it locally and do not provide it to the client
	- **BOTH** - save it locally and provide it to the client

	To enable saving image copies by the driver one of **LOCAL** or **BOTH** items should be "ON".

* **CCD_LOCAL_MODE** - This property sets the path and the file name template of the driver saved copy of the image. It has two items:
	- **DIR** - the directory where the image will be saved, this directory should exist and should be writable.
	- **PREFIX** - the file name prefix or the file name template. There are two ways to provide the filename template. The first is the legacy, INDI style, where you can provide a prefix and a file number placeholder. The second is more complex with various placeholders. The image format extension is automatically appended.

In client-server setup the camera driver saved images are saved on the server. In this context "LOCAL" stands for local for the server and the clients can access them through the INDIGO Imager Agent.

## Legacy file name templates

The legacy file name templates come as INDI standard heritage. Here **PREFIX** item is in format "image_XXX" or "image_XXXX", where "image" is a string literal and "XXX" or "XXXX" suffix is a placeholder for the file number.

### Examples

1. The first FITS image with **PREFIX** = "m31_XXX" will be saved as "m31_001.fits"

2. The second Nikon raw image with **PREFIX** = "m57_XXXX" will be saved as "m57_0002.nef"

## INDIGO style file name templates

INDIGO file name templates support a number of placeholders starting with "%" character, which will be expanded to different image properties in the file name. If "%" is present in the **PREFIX** INDIGO placeholder format is assumed and X-es in the suffix will not be replaced with with the file number.

### Valid placeholders:

* **%M** - will be expanded to the MD5 sum of the first 5kb of the file content. It can be used to provide unique file names and to check if the file is already downloaded by the client.

* **%E** or **%nE** - will be expanded to the exposure time. If "n" is provided it will be used as the number of digits after the decimal point.

* **%T** - expands to the sensor temperature in &deg;C

* **%F** - expands to the frame type: "Light", "Bias", "Dark" etc.

* **%D**, **%-D** or **%.D** - expands to the date in formats respectively: YYYYMMDD, YYYY-MM-DD or YYYY.MM.DD

* **%H**, **%-H** or **%.H** - expands to the local time in formats respectively: HHMMSS, HH-MM-SS or HH.MM.SS

* **%C** - expands to the filter name: "R", "G", "B", "Ha", "OIII" etc.

* **%nS** - expands to the sequential number of the file with the same name. Where 'n' is the number of digits used to represent the number and can be in the range [1, 5].

### Examples

1. FITS file with **PREFIX** = "m31_%-D_%.H_MDSum_%M" can expand to "m31_2022-10-29_22:38:45_MDSum_71f920fa275127a7b60fa4d4d41432a3.fits"

1. The third XISF image in Ha with **PREFIX** = "Triangulum_Galaxy_%C_%3S" will expand to "Triangulum_Galaxy_Ha_003.xisf"

1. The first Dark frame in FITS format with 300s exposure at -10&deg;C with **PREFIX** = "%F_%1Es_%TC_%2S" will expand to "Dark_300.0s_-10C_01.fits"

Clear skies!
