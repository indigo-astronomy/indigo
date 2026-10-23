"""Allocate reviewed audit findings through the canonical helper."""
from pathlib import Path
import json,subprocess,tempfile
r=Path(__file__).resolve().parent
tid=r.name
proposals=json.loads((r/'finding-proposals.json').read_text())
outputs=[]
for item in proposals:
 record=dict(item, status='open', owner='coordinator', tasks=[{'id':tid,'report':'repository-review.md','revision':None}], resolution=None)
 with tempfile.NamedTemporaryFile(mode='w',suffix='.json',delete=False) as file:
  json.dump(record,file); name=file.name
 try:
  result=subprocess.run(['./.factory/scripts/findings','create','--task',tid,'--record',name],text=True,capture_output=True,check=True)
  outputs.append(result.stdout.strip())
 finally:
  Path(name).unlink()
(r/'finding-allocation.log').write_text('\n'.join(outputs)+'\n')
print('\n'.join(outputs))
