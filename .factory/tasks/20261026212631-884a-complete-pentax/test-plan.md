# Test plan

AC1..6: initialization rollback/reconnect; native program mapping/control readback; timed/bulb stop+restore; capture ready/busy/deadline; abort/restart/disconnect; valid JPEG/RAW and invalid segment/offset/overflow/short transfer; preserve old RAM bits. Production implementation/public property path where asynchronous behavior matters.

Original17 fake/native pass; one short physical baseline discovers K-m but fails claim(-99), no exposure/settings changes. Final hardware one bounded group: controls with restore, short image, fractional bulb, brief abort/recovery/disconnect. Non-interactive, no physical unplug. Final run tools/run_driver_test.py ccd_pentax --hw; USB claim blocker must be resolved before claiming hardware pass.
