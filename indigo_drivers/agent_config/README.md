# Configuration agent

Agent for saving and restoring the state of the local INDIGO service

## Supported devices

N/A

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_agent_config

## Status: Under development

## Agent configuration

INDIGO Configuration agent stores information about the loaded drivers and connected devices. Device drivers are responsible for saving the specific device settings.
By default the current device settings are not saved. To enable selected device settings to be saved by the Configuration agent to the selected device profiles, **AUTOSAVE_DEVICE_CONFIGS** item of **AGENT_CONFIG_SETUP** property should be set to **ON**. If the selected device profile (**PROFILE** property of the device) is not changed and the service configuration is saved with a different name, the selected device profile will be overwritten and the two service configurations will share the same device profile. In other words changing the device settings in one service configuration will change the device settings in the other, if they share the same device profile.

When the configuration is loaded, unnecessary drivers will not be unloaded by default. If you need a "clean" service and only used drivers to be loaded **UNLOAD_UNUSED_DRIVERS** item of **AGENT_CONFIG_SETUP** property should be set to **ON**. This is turned off by default for performance reasons and because of the instability in some vendor provided SDKs on which the some drivers depend, which do not handle well multiple load/unload cycles.

## Saving, loading and removing service configurations

The Configuration agent can manage only the local service configuration. If a complex setup is used with multiple services, each service should run its own configuration agent.

### Saving or creating new configurations
New configuration or overwriting existing configuration is achieved by witting the configuration name in **NAME** item of the **AGENT_CONFIG_SAVE** property. If the configuration already exists it will be overwritten.

### Loading saved configuration
Loading the configuration is achieved by selecting the configuration listed in **AGENT_CONFIG_LOAD** property. Each configuration is an item in this property and stetting it to **ON** will load the configuration. Last used configuration (loaded or saved) from which the current state of the service is derived, can be seen in **NAME** item of **AGENT_CONFIG_LAST_CONFIG** property.

### Removing configuration
Removing configuration is achieved by witting the configuration name in **NAME** item of the **AGENT_CONFIG_REMOVE** property. Please note that the associated device profiles will not be removed or changed as they may be used by other configurations.
