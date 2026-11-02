import sys, types, subprocess, unittest
from pathlib import Path
root=Path('/Users/polakovic/Development/indigo')
for name in ['driver_format','source_map','editor','driver_editor','test_editor']:
 path=root/'indigo_tools/driver_editor'/(name+'.py')
 source=subprocess.check_output(['git','show','b6d3ee192:indigo_tools/driver_editor/'+name+'.py'],cwd=root,text=True)
 module=types.ModuleType(name);module.__file__=str(path);sys.modules[name]=module
 exec(compile(source,str(path),'exec'),module.__dict__)
result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromModule(sys.modules['test_editor']))
sys.exit(not result.wasSuccessful())
