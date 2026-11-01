"""Verify corrected local curl peer against the original production driver."""
from pathlib import Path
import os, shlex, subprocess, tempfile
root=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='astro-original-') as tmp:
 build=Path(tmp);source=build/'original.c'
 source.write_bytes(subprocess.run(['git','show','HEAD:indigo_drivers/agent_astrometry/indigo_agent_astrometry.c'],cwd=root,capture_output=True,check=True).stdout)
 flags=['-std=gnu11','-O3','-g','-DINDIGO_ASTROMETRY_TEST','-I'+str(root),'-I'+str(root/'indigo_libs'),'-I'+str(root/'build/include'),'-I'+str(root/'indigo_drivers/agent_astrometry')]
 commands=[['cc',*flags,'-c',str(source),'-o',str(build/'original.o')],['cc',*flags,str(root/'indigo_test/integration/test_agent_astrometry.c'),str(build/'original.o'),'-o',str(build/'test')]]
 commands[1]+=[str(root/'build/lib'/name) for name in ['libindigo.a','libindigocat.a','libjpeg.a','libtiff.a','libraw.a','libhidapi.a','libusb-1.0.dylib']]
 commands[1]+=['-lz','-lc++','-lm','-lpthread','-framework','CoreFoundation','-framework','IOKit']
 for args in commands:
  print(shlex.join(args),flush=True);subprocess.run(args,cwd=root,check=True)
 for i in range(10):
  print('Original driver / corrected peer repeat',i+1,flush=True)
  subprocess.run([str(build/'test'),'index abort'],cwd=root/'indigo_test',check=True,timeout=70)
