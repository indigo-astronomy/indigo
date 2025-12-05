# Saving images from the camera driver

Revision: 05.12.2025 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Configuring camera driver to save image copies

In INDIGO, saving images from the camera driver is controlled by two separate properties of the camera device:

* **CCD_UPLOAD_MODE** - It controls what to do with the acquired images. There are three items:
	- **CLIENT** - do not save local copy, just provide it to the client
	- **LOCAL** - save it locally and do not provide it to the client
	- **BOTH** - save it locally and provide it to the client

	To enable saving image copies by the driver one of **LOCAL** or **BOTH** items should be "ON".

* **CCD_LOCAL_MODE** - This property sets the path and the file name template of the driver saved copy of the image. It has two items:
	- **DIR** - the directory where the image will be saved, this directory should exist and should be writable.
	- **PREFIX** - the file name prefix or the file name template. There are two ways to provide the filename template. The first is the legacy, INDI style, where you can provide a prefix and a file number placeholder. The second is more complex with various placeholders. The image format extension is automatically appended.
	- **OBJECT** - the object name %o expands to this value.

In client-server setup the camera driver saved images are saved on the server. In this context "LOCAL" stands for local for the server and the clients can access them through the INDIGO Imager Agent.

## Legacy file name templates

The legacy file name templates come as INDI standard heritage. Here **PREFIX** item is in format "image_XXX" or "image_XXXX", where "image" is a string literal and "XXX" or "XXXX" suffix is a placeholder for the file number.

### Examples

1. The first FITS image with **PREFIX** = "m31_XXX" will be saved as "m31_001.fits"

2. The second Nikon raw image with **PREFIX** = "m57_XXXX" will be saved as "m57_0002.nef"

## INDIGO style file name templates

INDIGO file name templates support a number of placeholders starting with "%" character, which will be expanded to different image properties in the file name. If "%" is present in the **PREFIX** INDIGO placeholder format is assumed and X-es in the suffix will not be replaced with with the file number.

### Valid placeholders:
* **%o** - will be expanded to the object name in **CCD_LOCAL_MODE.OBJECT**

* **%M** - will be expanded to the MD5 sum of the first 5kb of the file content. It can be used to provide unique file names and to check if the file is already downloaded by the client.

* **%E** or **%nE** - will be expanded to the exposure time. If "n" is provided it will be used as the number of digits after the decimal point.

* **%T** - expands to the sensor temperature in &deg;C

* **%F** - expands to the frame type: "Light", "Bias", "Dark" etc.

* **%D**, **%-D** or **%.D** - expands to the date in formats respectively: YYYYMMDD, YYYY-MM-DD or YYYY.MM.DD

* **%H**, **%-H** or **%.H** - expands to the local time in formats respectively: HHMMSS, HH-MM-SS or HH.MM.SS

* **%C** - expands to the filter name: "R", "G", "B", "Ha", "OIII" etc. This placeholder reads *FILTER* keyword set in the **CCD_FITS_HEADERS** property, if not set it will expand to "nofilter". INDIGO Imager Agent sets this keyword if filer wheel is selected.

* **%nS** - expands to the sequential number of the file with the same name. Where 'n' is the number of digits used to represent the number and can be in the range [1, 5].

* **%nI** (after removing the directory path), and the **extension** is the file extension including the dot. For example, in template "M42_%3I_Light.fits", the prefix is "M42_", the extension is ".fits", and existing files "M42_001_Light.fits", "M42_005_Light.fits" would result in "M42_006_Light.fits". Unlike **%nS**, it handles gaps in sequences, guaranteeing higher numbers always mean newer files. Where 'n' is the number of digits with zero-padding [1-5].

* **%G** - expands to gain

* **%O** - expands to offset

* **%R** - expands to resolution

* **%B** - expands to binning

* **%P** - expands to focuser position

### Examples

1. FITS file with **TEMPLATE** = "m31_%-D_%.H_MDSum_%M" can expand to "m31_2022-10-29_22:38:45_MDSum_71f920fa275127a7b60fa4d4d41432a3.fits"

1. The third XISF image in Ha with **TEMPLATE** = "Triangulum_Galaxy_%C_%3S" will expand to "Triangulum_Galaxy_Ha_003.xisf"

1. The first Dark frame in FITS format with 300s exposure at -10&deg;C with **PREFIX** = "%F_%1Es_%TC_%2S" will expand to "Dark_300.0s_-10C_01.fits"

1. Files with **TEMPLATE** = "NGC7000_%C_%3I.fits" and filter "Ha". If existing files are "NGC7000_Ha_001.fits", "NGC7000_Ha_002.fits", "NGC7000_Ha_005.fits", the next file will be "NGC7000_Ha_006.fits" (skipping the gap at 003-004).

1. Files with **TEMPLATE** = "NGC7000_%3I_%C.fits" and existing files are "NGC7000_001_Ha.fits", "NGC7000_002_Ha.fits". When we change the filter to SII, the next file will be "NGC7000_003_SII.fits" because "%C" is between "%3I" and ".fits", so it is neither part of the **prefix** nor the **extension**.


## Downloading images saved by the driver
Files saved by the camera driver are available for download from the INDIGO Imager Agent. There are four properties that must be used:

* **AGENT_IMAGER_DOWNLOAD_IMAGE** is a BLOB property containing the requested image data updated when image download is requested.

* **AGENT_IMAGER_DOWNLOAD_FILES** property provides a list of the files available for download in the folder pointed by **CCD_LOCAL_MODE.DIR**. The first item is **REFRESH**. Setting it to "ON" refreshes the file list. Next items are files available for download. Setting any item to "ON" will provide the file content in  **AGENT_IMAGER_DOWNLOAD_IMAGE** property for download.

* **AGENT_IMAGER_DOWNLOAD_FILE** - another way to download a remote file is by setting **FILE** item of this property to the file name. This way the data will be provided in **AGENT_IMAGER_DOWNLOAD_IMAGE** property.

* **AGENT_IMAGER_DELETE_FILE** - when **FILE** item of this property is set to a file name the file will be removed from the file system and removed from **AGENT_IMAGER_DOWNLOAD_FILES** list.

### Example workflow: Download file and remove it from server
1. We have the file list available for download in **AGENT_IMAGER_DOWNLOAD_FILES**:
	```
	-> AGENT_IMAGER_DOWNLOAD_FILES = OK
	-> AGENT_IMAGER_DOWNLOAD_FILES.REFRESH = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_001.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_002.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_003.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_004.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_005.fits = OFF
	```
2. We request download of "M16_2022-10-30_Light_001.fits" by sending **AGENT_IMAGER_DOWNLOAD_FILES** property change request:
	```
	<- AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_001.fits = ON
	```
3. We receive property update of **AGENT_IMAGER_DOWNLOAD_FILES** and **AGENT_IMAGER_DOWNLOAD_IMAGE**
	```
	-> AGENT_IMAGER_DOWNLOAD_FILE = OK
	-> AGENT_IMAGER_DOWNLOAD_FILES = OK
	-> AGENT_IMAGER_DOWNLOAD_IMAGE = OK
	-> AGENT_IMAGER_DOWNLOAD_IMAGE.IMAGE = <M16_2022-10-30_Light_001.fits DATA>
	```
4. Now as we have the image and we have saved it locally, we want to remove it from the server. We need to send **AGENT_IMAGER_DELETE_FILE** change request:
	```
	<- AGENT_IMAGER_DELETE_FILE.FILE = "M16_2022-10-30_Light_001.fits"
	```
5. After that the file is deleted and we receive:
	```
	-> AGENT_IMAGER_DELETE_FILE = OK
	-> AGENT_IMAGER_DOWNLOAD_FILES = OK
	-> AGENT_IMAGER_DOWNLOAD_FILES.REFRESH = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_002.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_003.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_004.fits = OFF
	-> AGENT_IMAGER_DOWNLOAD_FILES.M16_2022-10-30_Light_005.fits = OFF
	```

Alternatively we can request image download by using **AGENT_IMAGER_DOWNLOAD_FILE** property. In this case we need to replace the request in step 2. with:
```
<- AGENT_IMAGER_DOWNLOAD_FILE.FILE = "M16_2022-10-30_Light_001.fits"
```

### Error handling
For clarity we show only the "good weather" scenario in the example above. In case of an error the failed property update request will result in **ALERT** property state.

For example, if we request removal of the non existing file "M16_2022-10-30_Light_006.fits", the AGENT_IMAGER_DELETE_FILE property state will be set to **ALERT**:
```
<- AGENT_IMAGER_DELETE_FILE.FILE = "M16_2022-10-30_Light_006.fits"
-> AGENT_IMAGER_DELETE_FILE = ALERT
```
Similarly, if we want to download the same non existing file it will result in **ALERT**:
```
<- AGENT_IMAGER_DOWNLOAD_FILE.FILE = "M16_2022-10-30_Light_006.fits"
-> AGENT_IMAGER_DOWNLOAD_FILE = ALERT
```

Clear skies!
