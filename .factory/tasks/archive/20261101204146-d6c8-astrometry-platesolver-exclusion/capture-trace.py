"""Instrument a temporary copy of the unchanged integration observer, never production."""
from pathlib import Path
import os, re, shlex, shutil, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[3]
evidence=Path(__file__).resolve().parent
phase=sys.argv[1]
instrumentation=r'''
static void task_trace(indigo_property *property) {
	if (strcmp(property->device, AGENT) != 0) {
		return;
	}
	FILE *trace = fopen(getenv("ASTRO_TASK_TRACE"), "a");
	if (trace == NULL) {
		abort();
	}
	fprintf(trace, "property %s state=%d count=%d", property->name, property->state, property->count);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		if (property->type == INDIGO_SWITCH_VECTOR) {
			fprintf(trace, " %s=%d", item->name, item->sw.value);
		} else if (property->type == INDIGO_NUMBER_VECTOR) {
			fprintf(trace, " %s=%.6g", item->name, item->number.value);
		} else if (property->type == INDIGO_BLOB_VECTOR) {
			fprintf(trace, " %s.size=%ld", item->name, item->blob.size);
		}
	}
	fputc('\n', trace);
	fclose(trace);
}
'''
with tempfile.TemporaryDirectory(prefix='astro-trace-build-') as folder:
 build=Path(folder)
 source=(root/'indigo_test/integration/test_agent_astrometry.c').read_text().replace('#include "../test_runner.h"', '#include "'+str(root/'indigo_test/test_runner.h')+'"')
 source=source.replace('static indigo_result observe(indigo_property *property) {',instrumentation+'\nstatic indigo_result observe(indigo_property *property) {')
 source=source.replace('observation *entry = find_observation(property->device, property->name);','task_trace(property);\n\tobservation *entry = find_observation(property->device, property->name);',1)
 (build/'trace.c').write_text(source)
 args=['cc','-std=gnu11','-g','-O1','-DINDIGO_ASTROMETRY_TEST','-I'+str(root),'-I'+str(root/'indigo_libs'),'-I'+str(root/'build/include'),str(build/'trace.c'),str(root/'indigo_test/build/integration/agent_astrometry_driver.o'),'-o',str(build/'trace')]
 args += [str(root/'build/lib'/name) for name in ['libindigo.a','libindigocat.a','libjpeg.a','libtiff.a','libraw.a','libhidapi.a','libusb-1.0.dylib']]
 args += ['-lz','-lc++','-lm','-lpthread','-framework','CoreFoundation','-framework','IOKit']
 print(shlex.join(args),flush=True);subprocess.run(args,check=True)
 for number,case in enumerate(['FITS solution and parsed WCS','reported solver failures','repeated driver lifecycle']):
  trace=evidence/(phase+'-'+str(number)+'.trace')
  trace.write_text('')
  env=dict(os.environ,INDIGO_TEST_KEEP_OUTPUT='1',ASTRO_TASK_TRACE=str(trace))
  result=subprocess.run([str(build/'trace'),case],cwd=root/'indigo_test',env=env,check=True,capture_output=True,text=True,timeout=120)
  print(result.stdout,flush=True)
  combined=trace.read_text()
  for path in re.findall(r'Kept test output in (.+)',result.stdout):
   fixture=Path(path)
   commands=fixture/'commands.log'
   combined+='\nordered fake tool commands:\n'+(commands.read_text() if commands.exists() else '(none)\n')
   combined=combined.replace(path,'<fixture>')
   shutil.rmtree(fixture)
  trace.write_text(re.sub(r'image_[0-9A-Fa-f]+_0x[0-9A-Fa-f]+', 'image_<id>', combined))
