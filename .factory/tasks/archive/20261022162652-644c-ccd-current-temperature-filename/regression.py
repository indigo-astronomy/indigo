"""Compile the real filename parser against synthetic CCD properties."""
from pathlib import Path
import subprocess
import tempfile

repo = Path.cwd()
source = (repo / 'indigo_libs/indigo_ccd_driver.c').read_text()
functions = source[source.index('static void sanitize('):source.index('\nvoid indigo_process_image(')]
headers = '''
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_md5.h>
#define OBJECT_LENGTH 41
'''
harness = r'''
static int failures = 0;
static int cases = 0;
static void check(indigo_device *device, const char *pattern, const char *expected) {
	char prefix[PATH_MAX], result[PATH_MAX];
	strcpy(prefix, pattern);
	bool ok = create_file_name(device, NULL, 0, "/tmp/", prefix, ".fits", result);
	cases++;
	if (!ok || strcmp(result, expected)) {
		fprintf(stderr, "FAIL %s: got %s, expected %s\n", pattern, result, expected);
		failures++;
	}
}
int main(void) {
	indigo_ccd_context context = { 0 };
	indigo_device camera = { 0 };
	camera.device_context = &context;
	indigo_device *device = &camera;
	context.ccd_temperature_property = indigo_init_number_property(NULL, "test", "temperature", "", "", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	context.ccd_cooler_property = indigo_init_switch_property(NULL, "test", "cooler", "", "", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	CCD_TEMPERATURE_ITEM->number.value = -12.375;
	CCD_TEMPERATURE_ITEM->number.target = -20.125;
	CCD_COOLER_ON_ITEM->sw.value = true;
	check(device, "%T_%2T_%9T", "/tmp/-20_-20.12_-20.12500.fits");
	check(device, "%t", "/tmp/-12.fits");
	check(device, "%0t_%1t_%2t_%5t_%9t", "/tmp/-12_-12.4_-12.38_-12.37500_-12.37500.fits");
	check(device, "light_%T_%t_%2T_%2t_tail", "/tmp/light_-20_-12_-20.12_-12.38_tail.fits");
	check(device, "%t_%t", "/tmp/-12_-12.fits");
	CCD_COOLER_ON_ITEM->sw.value = false;
	check(device, "%T_%2T", "/tmp/NA_NA.fits");
	check(device, "%t_%2t", "/tmp/-12_-12.38.fits");
	CCD_COOLER_PROPERTY->hidden = true;
	check(device, "%T_%t", "/tmp/NA_-12.fits");
	CCD_COOLER_ON_ITEM->sw.value = true;
	check(device, "%T_%2t", "/tmp/NA_-12.38.fits");
	CCD_TEMPERATURE_PROPERTY->hidden = true;
	check(device, "%T_%2T_%t_%2t", "/tmp/NA_NA_NA_NA.fits");
	CCD_TEMPERATURE_PROPERTY->hidden = false;
	CCD_COOLER_PROPERTY->hidden = false;
	CCD_TEMPERATURE_ITEM->number.value = 23.25;
	check(device, "%t_%1t_%2t", "/tmp/23_23.2_23.25.fits");
	CCD_TEMPERATURE_ITEM->number.value = 0;
	check(device, "%t_%2t", "/tmp/0_0.00.fits");
	check(device, "indigo_008_temperature_%3S_%t", "/tmp/indigo_008_temperature_001_0.fits");
	free(context.ccd_temperature_property);
	free(context.ccd_cooler_property);
	printf("%d/%d filename cases passed\n", cases - failures, cases);
	return failures ? 1 : 0;
}
'''
with tempfile.TemporaryDirectory(prefix='indigo-filename-') as tmp:
    path = Path(tmp)
    (path / 'test.c').write_text(headers + functions + harness)
    subprocess.run(['cc', '-std=gnu11', '-Iindigo_libs', str(path / 'test.c'),
                    '-Lbuild/lib', '-lindigo', '-Wl,-rpath,' + str(repo / 'build/lib'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
