import argparse,ctypes,hashlib,json,pathlib,re,struct,tempfile
parser=argparse.ArgumentParser(description='Reconstruct mean and summed/clamped Rosetta fixtures on Darwin.')
parser.add_argument('--root',type=pathlib.Path,default=pathlib.Path.cwd(),help='INDIGO repository root (default: current directory)')
parser.add_argument('--output',type=pathlib.Path,help='Output directory (default: a newly created temporary directory)')
args=parser.parse_args()
root=args.root.resolve(); out=args.output.resolve() if args.output else pathlib.Path(tempfile.mkdtemp(prefix='indigo-rosetta-fixtures-'))
out.mkdir(parents=True,exist_ok=True)
src=root/'indigo_drivers/ccd_simulator/indigo_ccd_simulator_mono.h'
pix=[int(v,16) for v in re.findall(r'0x[0-9a-fA-F]+',src.read_text())]
assert len(pix)==1600*1200
sums=[pix[y*1600+x]+pix[y*1600+x+1]+pix[(y+1)*1600+x]+pix[(y+1)*1600+x+1] for y in range(0,1200,2) for x in range(0,1600,2)]
libc=ctypes.CDLL('/usr/lib/libSystem.B.dylib'); manifest={'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'blocks_exceeding_65535':sum(v>65535 for v in sums),'frames':[]}
for seed in [None,1,5859]:
 noise=[0]*len(sums)
 if seed is not None:
  libc.srand(seed); noise=[libc.rand()&127 for _ in sums]
 for method in ['mean','sum']:
  values=[min(65535,(v//4 if method=='mean' else min(v,65535))+n) for v,n in zip(sums,noise)]
  data=struct.pack('<III',0x32574152,800,600)+struct.pack('<480000H',*values)
  name=f'{method}-'+('no-noise' if seed is None else f'seed-{seed}')+'.raw'; (out/name).write_bytes(data)
  manifest['frames'].append({'name':name,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'method':method,'noise_seed':seed})
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(out)
