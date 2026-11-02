# Test plan

Use existing test_driver_loader fixtures and synthetic callbacks. Reproduce initialized→remove→inactive replacement, assert same slot, callback counts, empty-entry state and initialized lookup. Also cover failed INIT and refused SHUTDOWN. Existing dynamic longpath/duplicate/capacity suites remain. No hardware/image recognition/external services. Only loopback fixture used by existing capacity test.
