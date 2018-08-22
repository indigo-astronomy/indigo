# INDIGO protocols

## Introduction

INDIGO client/driver communication is based on the abstraction of INDI messages. As far as software bus and function calls are used
instead of named pipes, it is generally protocol independent. Nevertheless different bus instances can be connected over the network
or pipes and in this case INDIGO protocols are used.

To achieve an interoperability with existing infrastructure and to support new features INDIGO protocol adapters do understand 2 different
protocols:

* INDIGO XML protocol
* INDIGO JSON protocol

## INDIGO XML protocol

INDIGO XML is a backward compatible extension of legacy INDI protocol 1.7 as described by Elwood C. Downey in
<http://www.clearskyinstitute.com/INDI/INDI.pdf> and implemented by INDI framework version 0.7 and later.

Extensions are enabled only in case of succesfull handshake with two possible variants:

1. client request INDIGO protocol:

```
→ <getProperties version='2.0'/>
```
2. client offers INDIGO protocol and server accept it:

```
→ <getProperties version='1.7' switch='2.0'>
← <switchProtocol version='2.0'/>
```
In case of successful handshake for version 2.0 the following extensions can be used:

1. BLOBs in BASE64 representation can use any line length instead of fixed 74 characters for much faster encoding/decoding.

2. BLOBs can be referenced by URL instead of inline BASE64 encoding with url parameter in oneBLOB tag, e.g.

```
← <setBLOBVector device='CCD Simulator' name='CCD_IMAGE' state='Ok'>
    <oneBLOB name='%s' url='http://localhost:7624/blob/0x10381d798.fits?1534933649001'/>
  </setBLOBVector>
```

   Data available on given URL are pure binary image in selected format. Data are available only while the property is in 'Ok' state.

3. Number property items has 'target' attribute to distinguish between current and target item value for properties like CCD_EXPOSURE.


If protocol version 2.0 is used, INDIGO property and item names are used (more gramatically and semantically consistent),
while if version 1.7 is used, names of  commonly used names are maped to their INDI counter parts. 

## INDIGO JSON protocol 

JSON protocol offers the same features as XML version 2.0 protocol, but can be more confortable for a particular purpose,
e.g. javascript client used by web applications or .net client used by ASCOM drivers. Messages can be exchanged either
over TCP or WEB-cocket stream.

JSON protocol offers just BLOBs referenced by URL, no inline data.

The mapping of XML to JSON messages demonstrated on a few examples is as follows:

```
→ <getProperties version='2.0'/>
```
is mapped to
```
→ { "getProperties": { "version": 512 } }
```

```← <defTextVector device='Server' name='LOAD' group='Main' label='Load driver' state='Idle' perm='rw'>
    <defText name='DRIVER' label='Load driver'></defText>
  </defTextVector>
```
is mapped to
```
← { "defTextVector": { "version": 512, "device": "Server", "name": "LOAD", "group": "Main", "label": "Load driver", "perm": "rw", "state": "Idle", "items": [  { "name": "DRIVER", "label": "Load driver", "value": "" } ] } }
```
```
← <defSwitchVector device='Server' name='RESTART' group='Main' label='Restart' rule='AnyOfMany' state='Idle' perm='rw'>
    <defSwitch name='RESTART' label='Restart server'>false</defSwitch>
  </defSwitchVector>
```
is mapped to
```
← { "defSwitchVector": { "version": 512, "device": "Server", "name": "RESTART", "group": "Main", "label": "Restart", "perm": "rw", "state": "Idle", "rule": "AnyOfMany", "items": [  { "name": "RESTART", "label": "Restart server", "value": false } ] } }
```
```
← <defNumberVector device='CCD Imager Simulator' name='CCD_EXPOSURE' group='Camera' label='Start exposure' state='Idle' perm='rw'>
    <defNumber name='EXPOSURE' label='Start exposure' min='0' max='10000'step='1' format='%g' target='0'>0</defNumber>
  </defNumberVector>
```
is mapped to
```
← { "defNumberVector": { "version": 512, "device": "CCD Imager Simulator", "name": "CCD_EXPOSURE", "group": "Camera", "label": "Start exposure", "perm": "rw", "state": "Idle", "items": [  { "name": "EXPOSURE", "label": "Start exposure", "min": 0, "max": 10000, "step": 1, "format": "%g", "target": 0, "value": 0 } ] } }
```
```
← <setSwitchVector device='' name='' state=''>
    <oneSwitch name='CONNECTED'>On</oneSwitch>
    <oneSwitch name='DISCONNECTED'>Off</oneSwitch>
  </setSwitchVector>
```
is mapped to
```
← { "setSwitchVector": { "device": "CCD Imager Simulator", "name": "CONNECTION", "state": "Ok", "items": [  { "name": "CONNECTED", "value": true }, { "name": "DISCONNECTED", "value": false } ] } }
```
```
← <setBLOBVector device='' name='' state=''>
	  <oneBLOB name='IMAGE'>/blob/0x10381d798.fits</oneSwitch>
  </setBLOBVector>
```
is mapped to
```
← { "setBLOBVector": { "device": "CCD Imager Simulator", "name": "CCD_IMAGE", "state": "Ok", "items": [  { "name": "IMAGE", "value": "/blob/0x10381d798.fits" } ] } }
```
```
→ <newNumberVector device='' name=''>
  	<oneNumber name='EXPOSURE'>1</defNumber>
  </newNumberVector>
```
is mapped to
```
→ {"newNumberVector":{"device":"CCD Imager Simulator","name":"CCD_EXPOSURE","items":[{"name":"EXPOSURE","value":1}]}}
```
```
← <deleteProperty device='Mount IEQ (guider)'/>
```
is mapped to
```
← { "deleteProperty": { "device": "Mount IEQ (guider)" } }
```
## References

XML parser is implemented in [indigo_libs/indigo_xml.c] (https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_xml.c).

Driver side protocol adapter is implemented in [indigo_libs/indigo_driver_xml.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver_xml.c).

Client side protocol adapter is implemented in [indigo_libs/indigo_client_xml.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_client_xml.c).

BASE64 encoding/decoding is implemented in [indigo_libs/indigo_base64.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_base64.c).

JSON parser is implemented in [indigo_libs/indigo_json.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_json.c).

Driver side protocol adapter is implemented in [indigo_libs/indigo_driver_json.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver_json.c).

Client side protocol adapter is implemented in [indigo_libs/indigo_client_json.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_client_json.c).
