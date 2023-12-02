# ZWO ASIAIR Power Ports AUX Controller driver

https://www.raspberrypi.org

## Supported devices

ZWO Power Ports ASIAIR for RPi Models.

## Supported platforms

This driver is supported on Linux (ARM v6+).

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_asiair

## Status: More testing needed

Driver is developed and tested with:
* ZWO ASIAIR PRO RPi
* ZWO ASIAIR Plus RPi

## GPIO Pin mapping:

This driver supports the 4 power ports on the ASIAIR PRO and Plus RPi models

### Outputs
* Output 1 -> GPIO 12 (PWM0*)
* Output 2 -> GPIO 13
* Output 3 -> GPIO 26
* Output 4 -> GPIO 18 (PWM1*)

*Two-channel PWM is supported. To enable it add "dtoverlay=pwm-2chan" to the bottom of /boot/config.txt:
```
$ sudo echo "dtoverlay=pwm-2chan" >>/boot/config.txt
```
