"""Update only Factory/decision/test navigation groups, preserving all build settings and object IDs."""
from pathlib import Path
import hashlib,json,re,subprocess,tempfile
r=Path('/Users/polakovic/Development/indigo');project=r/'indigo.xcodeproj/project.pbxproj'
plan=json.load(open('/tmp/indigo-task-id-plan.json'));task=plan['new_task'];ids={Path(k).name:v['id'] for k,v in plan['mapping'].items()}
s=project.read_text()
for old,new in ids.items():s=re.sub(r'(?<![a-z0-9-])'+re.escape(old)+r'(?![a-z0-9-])',lambda m:new,s)
with tempfile.TemporaryDirectory() as temp:
 path=Path(temp)/'project.pbxproj';path.write_text(s);out=Path(temp)/'project.json';subprocess.run(['plutil','-convert','json','-o',str(out),str(path)],check=True);objects=json.loads(out.read_text())['objects']
roots={'.factory':'41ED317140F6078C54A87EF0','indigo_test/factory':'F559B76B4D9A14829C38BB63','docs/decisions':'0A7925AB3B3F5D199EABF708'}
new_files={};new_groups={};modified=set()
def object_id(path):
 value=hashlib.sha256(('factory-upgrade-navigation:'+path).encode()).hexdigest()[:24].upper()
 if value in objects:raise ValueError('new project object collision: '+path)
 return value
def ensure_group(parent,name,path):
 for child in objects[parent]['children']:
  obj=objects[child]
  if obj.get('isa')=='PBXGroup' and obj.get('path')==name:return child
 key=object_id(path);objects[key]=dict(isa='PBXGroup',children=[],path=name,sourceTree='<group>');new_groups[key]=path;objects[parent]['children'].append(key);modified.add(parent);return key
def ensure_file(parent,name,path):
 if any(objects[c].get('isa')=='PBXFileReference' and objects[c].get('path')==name for c in objects[parent]['children']):return
 key=object_id(path);objects[key]=dict(isa='PBXFileReference',path=name,sourceTree='<group>');new_files[key]=path;objects[parent]['children'].append(key);modified.add(parent)
required=set()
raw=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard','--','.factory','indigo_test/factory','docs/decisions'],cwd=r).decode().split('\0')
for name in raw:
 if not name or not (r/name).is_file():continue
 if name.startswith('.factory/tasks/'+task+'/'):name=name.replace('.factory/tasks/'+task+'/', '.factory/tasks/archive/'+task+'/',1)
 required.add(name)
required.add('.factory/completions/'+task+'.json')
# The legacy aggregate was split; remove only its obsolete navigation object.
stale='998B0F8CBE670F5DC7F8F5DE'
for parent,obj in objects.items():
 if obj.get('isa')=='PBXGroup' and stale in obj['children']:obj['children'].remove(stale);modified.add(parent)
for path in sorted(required):
 root=next(root for root in roots if path.startswith(root+'/'));parent=roots[root];parts=Path(path[len(root)+1:]).parts;prefix=root
 for name in parts[:-1]:prefix+='/'+name;parent=ensure_group(parent,name,prefix)
 ensure_file(parent,parts[-1],path)
def child_lines(key):return ''.join('\t\t\t\t'+c+' /* '+objects[c].get('name',objects[c].get('path',''))+' */,\n' for c in objects[key]['children'])
for key in modified-set(new_groups):
 pattern=r'(\t\t'+key+r' /\*[^\n]*\*/ = \{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = \(\n).*?(\t\t\t\);)'
 s,count=re.subn(pattern,lambda m:m[1]+child_lines(key)+m[2],s,flags=re.S)
 if count!=1:raise ValueError('cannot locate exact parent group '+key)
s=re.sub(r'^\t\t'+stale+r' /\*[^\n]*\*/ = \{[^\n]*\};\n','',s,flags=re.M)
file_blocks=[]
for key,path in sorted(new_files.items(),key=lambda x:x[1]):
 name=Path(path).name;suffix=Path(name).suffix
 kind={'.md':'net.daringfireball.markdown','.json':'text.json','.py':'text.script.python','.sh':'text.script.sh'}.get(suffix,'text')
 file_blocks.append('\t\t'+key+' /* '+name+' */ = {isa = PBXFileReference; lastKnownFileType = '+kind+'; path = '+json.dumps(name)+'; sourceTree = "<group>"; };\n')
s=s.replace('/* End PBXFileReference section */',''.join(file_blocks)+'/* End PBXFileReference section */')
group_blocks=[]
for key,path in sorted(new_groups.items(),key=lambda x:x[1]):
 name=Path(path).name;group_blocks.append('\t\t'+key+' /* '+name+' */ = {\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n'+child_lines(key)+'\t\t\t);\n\t\t\tpath = '+json.dumps(name)+';\n\t\t\tsourceTree = "<group>";\n\t\t};\n')
s=s.replace('/* End PBXGroup section */',''.join(group_blocks)+'/* End PBXGroup section */')
project.write_text(s)
print('Xcode navigation:',len(new_files),'new file references,',len(new_groups),'groups; build phases/settings unchanged.')
(r/'.factory/tasks'/task/'xcode-required-paths.json').write_text(json.dumps(sorted(required),indent=2)+'\n')
