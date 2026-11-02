import sys,importlib.util,tempfile,shutil,unittest,os
from pathlib import Path
r=Path('/private/tmp/indigo-tool-io')
spec=importlib.util.spec_from_file_location('tool_tests',r/'indigo_test/unit/test_tool_io_failures.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory(prefix='indigo-tool-old-source-') as tmp:
 p=Path(tmp);shutil.copytree('/private/tmp/indigo-tool-baseline/source',p/'indigo_tools');shutil.copyfile(r/'indigo_tools/tool_file_io.h',p/'indigo_tools/tool_file_io.h');m.ROOT=p;os.environ['CC']='cc -I'+str(r/'indigo_libs')
 suite=unittest.TestSuite(m.ToolIOTests(name) for name in ['test_upload_exact_bytes_rewind_termination_and_cleanup','test_raw_file_partial_write_interrupt_and_truncation','test_make_inventory_is_consistent','test_generator_failures_and_valid_definition'])
 result=unittest.TextTestRunner(verbosity=2).run(suite);sys.exit(not result.wasSuccessful())
