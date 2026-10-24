# Development plan

Status: complete

## Approach and affected components
Single file change in indigo_optional_drivers/ccd_rpi/indigo_ccd_rpi.cpp plus the C++ standard in its Makefile. RPiCamera gains a generation counter, an operation mutex and persistent settings; the INDIGO part splits connection into ccd_open()/ccd_close() and guards timer callbacks with the master device lock or an exposure sequence number.

## Baseline
With only the build fix every exposure failed ("Can't queue request"); before it the driver did not compile.
