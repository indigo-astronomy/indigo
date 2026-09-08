// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include "../test_runner.h"

#define DEFINITION "indigo_aux_architecture_test.driver"
#define GENERATED "indigo_aux_architecture_test.c"

static const char *fixture = "\tlabel = \"Architecture test\";\n\tauthor = \"INDIGO tests\";\n\tcopyright = \"Copyright (c) 2026 INDIGO tests\";\n\tversion = 7;\n\tinclude {\n\t\t#include <architecture_test_missing_sdk.h>\n\t}\n\taux {\n\t\tname = \"Architecture test device\";\n\t}\n}\n";

static bool write_text(const char *path, const char *text) {
	FILE *file = fopen(path, "w");
	if (!file) {
		return false;
	}
	bool written = fputs(text, file) >= 0;
	return fclose(file) == 0 && written;
}

static bool read_text(const char *path, char *buffer, size_t size) {
	FILE *file = fopen(path, "r");
	if (!file) {
		return false;
	}
	size_t length = fread(buffer, 1, size - 1, file);
	buffer[length] = 0;
	bool complete = !ferror(file) && fgetc(file) == EOF;
	return fclose(file) == 0 && complete;
}

static bool run(char *const arguments[]) {
	pid_t pid = fork();
	if (pid < 0) {
		return false;
	}
	if (pid == 0) {
		int output = open("command.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (output < 0 || dup2(output, STDOUT_FILENO) < 0 || dup2(output, STDERR_FILENO) < 0) {
			_exit(126);
		}
		close(output);
		execvp(arguments[0], arguments);
		_exit(127);
	}
	int status = 0;
	pid_t result;
	do {
		result = waitpid(pid, &status, 0);
	} while (result < 0 && errno == EINTR);
	bool success = result == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
	if (!success) {
		char output[65536];
		fprintf(stderr, "Command failed: %s\n", arguments[0]);
		if (read_text("command.log", output, sizeof(output))) {
			fputs(output, stderr);
		}
	}
	return success;
}

static bool generate(const char *expression) {
	char definition[4096], attribute[1024] = "";
	if (expression) {
		snprintf(attribute, sizeof(attribute), "\tsupported_architecture = \"%s\";\n", expression);
	}
	snprintf(definition, sizeof(definition), "driver architecture_test {\n%s%s", attribute, fixture);
	char *arguments[] = { TEST_GENERATOR, DEFINITION, NULL };
	return write_text(DEFINITION, definition) && run(arguments);
}

static void conditions_and_reverse_extraction(void) {
	const struct {
		const char *expression;
		const char *platform[3];
		const char *cpu[3];
		bool supported[3];
	} cases[] = {
		{ "!defined(__i386__)", { "INDIGO_LINUX", "INDIGO_LINUX", "INDIGO_MACOS" }, { "__i386__", "__x86_64__", "__aarch64__" }, { false, true, true } },
		{ "!defined(INDIGO_MACOS) || defined(__x86_64__)", { "INDIGO_MACOS", "INDIGO_MACOS", "INDIGO_WINDOWS" }, { "__x86_64__", "__aarch64__", "__aarch64__" }, { true, false, true } },
		{ "!defined(INDIGO_MACOS) || defined(__aarch64__)", { "INDIGO_MACOS", "INDIGO_MACOS", "INDIGO_LINUX" }, { "__x86_64__", "__aarch64__", "__x86_64__" }, { false, true, true } }
	};
	for (int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		ASSERT_TRUE(generate(cases[i].expression));
		char generated[65536], expected[1024], probe[2048];
		ASSERT_TRUE(read_text(GENERATED, generated, sizeof(generated)));
		snprintf(expected, sizeof(expected), "#if %s\n", cases[i].expression);
		ASSERT_TRUE(strstr(generated, expected) != NULL);
		snprintf(probe, sizeof(probe), "%sSUPPORTED\n#else\nUNSUPPORTED\n#endif\n", expected);
		ASSERT_TRUE(write_text("condition.c", probe));
		for (int j = 0; j < 3; j++) {
			char platform[128], cpu[128];
			snprintf(platform, sizeof(platform), "-D%s", cases[i].platform[j]);
			snprintf(cpu, sizeof(cpu), "-D%s", cases[i].cpu[j]);
			char *arguments[] = { TEST_CC, "-E", "-P", "-undef", platform, cpu, "condition.c", "-o", "condition.out", NULL };
			ASSERT_TRUE(run(arguments));
			ASSERT_TRUE(read_text("condition.out", probe, sizeof(probe)));
			char token[32], extra[32];
			ASSERT_EQ_INT(1, sscanf(probe, "%31s %31s", token, extra));
			ASSERT_STREQ(cases[i].supported[j] ? "SUPPORTED" : "UNSUPPORTED", token);
		}
		char *arguments[] = { TEST_GENERATOR, "-c", DEFINITION, NULL };
		ASSERT_TRUE(run(arguments));
		ASSERT_TRUE(read_text(DEFINITION, generated, sizeof(generated)));
		snprintf(expected, sizeof(expected), "supported_architecture = \"%s\";", cases[i].expression);
		ASSERT_TRUE(strstr(generated, expected) != NULL);
	}
}

static void unsupported_fallback(void) {
	// The missing SDK header must be excluded without changing the host CPU macros.
	ASSERT_TRUE(generate("0"));
	const char *harness = "#include <assert.h>\n#include \"indigo_aux_architecture_test.h\"\nint main(void) {\n\tindigo_driver_info info;\n\tassert(indigo_aux_architecture_test(INDIGO_DRIVER_INFO, &info) == INDIGO_OK);\n\tassert(info.version == 0x03000007);\n\tassert(strcmp(info.name, \"indigo_aux_architecture_test\") == 0);\n\tassert(indigo_aux_architecture_test(INDIGO_DRIVER_INIT, NULL) == INDIGO_UNSUPPORTED_ARCH);\n\tassert(indigo_aux_architecture_test(INDIGO_DRIVER_INIT, NULL) == INDIGO_UNSUPPORTED_ARCH);\n\tassert(indigo_aux_architecture_test(INDIGO_DRIVER_SHUTDOWN, NULL) == INDIGO_UNSUPPORTED_ARCH);\n\treturn 0;\n}\n";
	ASSERT_TRUE(write_text("fallback.c", harness));
	char *compile[] = { TEST_CC, "-std=gnu11", "-I" TEST_INCLUDE, GENERATED, "fallback.c", "-o", "fallback", NULL };
	ASSERT_TRUE(run(compile));
	char *execute[] = { "./fallback", NULL };
	ASSERT_TRUE(run(execute));
}

static void unrestricted_default(void) {
	ASSERT_TRUE(generate(NULL));
	char generated[65536];
	ASSERT_TRUE(read_text(GENERATED, generated, sizeof(generated)));
	ASSERT_TRUE(strstr(generated, "supported_architecture:") == NULL);
	ASSERT_TRUE(strstr(generated, "INDIGO_UNSUPPORTED_ARCH") == NULL);
}

int main(void) {
	char folder[] = "/tmp/indigo_architecture_XXXXXX";
	char original[PATH_MAX];
	if (!getcwd(original, sizeof(original)) || !mkdtemp(folder)) {
		perror("architecture fixture");
		return 1;
	}
	if (chdir(folder) != 0) {
		rmdir(folder);
		return 1;
	}
	const indigo_test_case tests[] = {
		{ "nine platform/CPU conditions and three reverse extractions", conditions_and_reverse_extraction },
		{ "unsupported fallback without SDK headers or linkage", unsupported_fallback },
		{ "unrestricted default", unrestricted_default }
	};
	int result = indigo_run_tests("Generator architecture", tests, sizeof(tests) / sizeof(tests[0]));
	DIR *directory = opendir(".");
	if (directory) {
		struct dirent *entry;
		while ((entry = readdir(directory))) {
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && unlink(entry->d_name) != 0) {
				result = 1;
			}
		}
		closedir(directory);
	} else {
		result = 1;
	}
	if (chdir(original) != 0 || rmdir(folder) != 0) {
		result = 1;
	}
	return result;
}
