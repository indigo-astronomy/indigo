# PrimaluceLab focuser/rotator driver

https://www.primalucelab.com

## Supported devices
* SESTO SENSO 2 focusers
* ESATTO focusers
* ARCO rotators

Single device is present on the first startup (no hot-plug support). Additional devices can be configured on runtime.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_primaluce

## Notes

This driver is tested agains firmware version 3.05!

To calibrate Sesto Senso 2 focuser:

* set the focuser to the innermost position manually,
* navigate to PrimaluceLab focuser > Advanced > Calibrate focuser with a control panel and click either "Start" or "Start inverted",
* once the outhermost position is reached, click "End",
* disconnect and connect again.

## Status: Work in progress

Tested with Sesto Senso 2.

As PrimaluceLab never answered any email both simulator and driver are based only on publicly available information :(

