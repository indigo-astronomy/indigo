# INDIGO Multi-instance support for drivers without plug & play support
Revision: 06.02.2022 (early draft)

Authors: **Peter Polakovic**

e-mail: *peter.polakovic@cloudmakers.eu*

There are two dimensions of relations between devices in INDIGO.
There are slave devices related to master devices, which are usually together represents single hardware. Typical examples are guiders slaved to guider cameras or mounts, integrated filter wheels slaved to imager cameras, etc.
And there are additional devices related to base devices, which usually represent distinct devices of the same type. Typical examples are multiple instances of serial devices like wheels or focusers. For USB devices the plug & play support is implemented, for serial devices, simulators and agents multi-instance support described by this document is used.

## User perspective


On most of the master AND base devices is enabled property ADDITIONAL_INSTANCES (label "Additional instances") and its item COUNT (label "Count") represents the number of additional devices for a given base device. If the value is changed, additional devices are created or freed. If the device is also master device for some slave devices, each additional master device is created together with its slave devices.

Additional device names are suffixed with "#number" where number starts with 2.

ADDITIONAL_INSTANCES is persistent and can be saved together with an active configuration profile.

## Developer perspective

To enable multi-instance support the following line should be added to ..._attach(indigo_device *device) function for the master device:

```
ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
```

The base devices without slave devices should be created by the code based on this template:

```
private_data = indigo_safe_malloc(sizeof(xxx_private_data));
xxx = indigo_safe_malloc_copy(sizeof(indigo_device), &xxx_template);
xxx->private_data = private_data;
indigo_attach_device(xxx);
```

and released by the code based on this template:

```
if (xxx != NULL) {
	indigo_detach_device(xxx);
	free(xxx);
	xxx = NULL;
}
if (private_data != NULL) {
	free(private_data);
	private_data = NULL;
}
```

The base devices with slave devices should be created by the code based on this template:

```
private_data = indigo_safe_malloc(sizeof(xxx_private_data));
xxx = indigo_safe_malloc_copy(sizeof(indigo_device), &xxx_template);
xxx->private_data = private_data;
indigo_attach_device(xxx);
yyy = indigo_safe_malloc_copy(sizeof(indigo_device), &yyy_template);
yyy->private_data = private_data; // master and slave devices share private data instance
yyy->master_device = xxx; // slave device points to its master device
indigo_attach_device(yyy);
```

and released by the code based on this template:

```
if (yyy != NULL) { // release slave devices first
	indigo_detach_device(yyy);
	free(yyy);
	yyy = NULL;
}
if (xxx != NULL) { // release master device last
	indigo_detach_device(xxx);
	free(xxx);
	xxx = NULL;
}
if (private_data != NULL) {
	free(private_data);
	private_data = NULL;
}
```

The agents should be created by the code based on this template:

```
private_data = indigo_safe_malloc(sizeof(agent_private_data));
agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
agent_device->private_data = private_data;
indigo_attach_device(agent_device);
agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
agent_client->client_context = agent_device->device_context;
indigo_attach_client(agent_client);
```

and released by the code based on this template:

```
if (agent_client != NULL) {
	indigo_detach_client(agent_client);
	free(agent_client);
	agent_client = NULL;
}
if (agent_device != NULL) {
	indigo_detach_device(agent_device);
	free(agent_device);
	agent_device = NULL;
}
if (private_data != NULL) {
	free(private_data);
	private_data = NULL;
}
```

Don't underestimate the order in which slave devices and clients are created and freed, it is critical!


Most of the shared code is located in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).



