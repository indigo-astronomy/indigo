include Makefile
.PHONY: primaluce-sanitize
primaluce-sanitize:
	$(CC) $(filter-out -arch arm64 x86_64,$(TEST_CFLAGS)) -arch arm64 -I. -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -c ../indigo_drivers/shared/primalucelib/primalucelib.c -o /tmp/primalucelib-sanitize.o
	$(CC) $(filter-out -arch arm64 x86_64,$(TEST_CFLAGS)) -arch arm64 -I. -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -o /tmp/test_primalucelib_sanitize unit/test_primalucelib.c /tmp/primalucelib-sanitize.o $(filter-out -arch arm64 x86_64,$(TEST_LDFLAGS)) -arch arm64 -fsanitize=address,undefined
	$(CC) $(filter-out -arch arm64 x86_64,$(TEST_CFLAGS)) -arch arm64 -I. -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -o /tmp/test_focuser_primaluce_sanitize integration/test_focuser_primaluce_simulator.c ../indigo_drivers/focuser_primaluce/indigo_focuser_primaluce.c /tmp/primalucelib-sanitize.o $(filter-out -arch arm64 x86_64,$(TEST_LDFLAGS)) -arch arm64 -fsanitize=address,undefined -lindigocat
