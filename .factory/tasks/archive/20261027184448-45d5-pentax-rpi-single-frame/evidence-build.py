from pathlib import Path
import subprocess,shlex
root=Path('/Users/polakovic/Development/indigo');out=Path('/private/tmp/pentax-protocol-tests');out.mkdir(exist_ok=True)
s=Path('indigo_libs/externals/libusb/libusb/os/darwin_usb.c').read_text();start=s.index('static int darwin_capture_claim_interface(');end=s.index('\nstatic int darwin_capture_release_interface(',start);(out/'pentax_darwin_claim.inc').write_text(s[start:end])
config={line.split(' = ',1)[0]:line.split(' = ',1)[1] for line in (root/'Makefile.inc').read_text().splitlines() if ' = ' in line}
flags=shlex.split(config['CFLAGS']);flags=[x for x in flags if x not in ('-arch','x86_64','arm64')]
cmd=['/usr/bin/clang']+flags+['-D_GNU_SOURCE','-I.','-Iindigo_libs','-I'+str(out),'indigo_test/integration/test_ccd_pentax_usb.c','-o',str(out/'test_ccd_pentax_usb')]+shlex.split(config['LDFLAGS'])+['-Wl,-rpath,'+str(root/'build/lib'),'-lindigo','-lusb-1.0','-ljpeg','-lm','-lpthread']
with Path('/private/tmp/pentax-framing-build.log').open('w') as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
print('compile',r.returncode);raise SystemExit(r.returncode)
