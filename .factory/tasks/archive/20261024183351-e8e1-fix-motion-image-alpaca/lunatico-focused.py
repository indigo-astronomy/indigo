import concurrent.futures,os,subprocess
from pathlib import Path
jobs=[(transport,case) for transport in ['serial','udp'] for case in ['no_op_goto','goto_failure','early_stop','fractional_target','abort_motion','exp_focuser']]
jobs += [('focuser',case) for case in ['single_read_failure','settings_during_motion','disconnect_during_motion']]
def run(job):
    transport,case=job
    binary={'serial':'test_rotator_lunatico_simulator','udp':'test_rotator_lunatico_udp_simulator','focuser':'test_focuser_lunatico_simulator'}[transport]
    env=dict(os.environ,AUX_TEST_FILTER=case)
    path=Path('/private/tmp/indigo-lunatico-focused-'+transport+'-'+case+'.log')
    with path.open('w') as output:
        result=subprocess.run(['./build/integration/'+binary],cwd='indigo_test',env=env,stdout=output,stderr=subprocess.STDOUT)
    print(transport,case,result.returncode,flush=True)
    return result.returncode
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    results=list(pool.map(run,jobs))
raise SystemExit(any(results))
