# Test and regression plan

Status: final

Four new cases per sysfs suite (pulse end failure, pre-exported pins, failed connect with pre-exported pin, zero PWM period), also under ASan/UBSan for aux_rpio. One new hardware case for aux_rpio (pin exported through sysfs before the connection stays exported, verified with `pinctrl`). Full recorded runs of both drivers, hardware-free and hardware, on both Pis.
