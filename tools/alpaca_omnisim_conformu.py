#!/usr/bin/env python3

# ConformU through the INDIGO Alpaca bridge, against ASCOM OmniSim.
#
#   OmniSim -> system_alpaca -> agent_alpaca -> ConformU
#
# Every OmniSim device is tested twice with the same ConformU settings: directly
# (the reference) and through the chain. A problem the chain run reports and the
# reference run does not is a loss in the INDIGO path; that is what the script
# looks for. Nothing is recorded and nothing is committed: the logs, the JSON
# results and report.md go to --outdir.
#
# Prerequisites, none of them part of the repository:
# - OmniSim unpacked in 'omnisim/' next to the working tree (system_alpaca
#   REFACTOR.md 4.1) or named by INDIGO_TEST_OMNISIM;
# - ConformU, by default /Applications/ConformU.app (CONFORMU overrides it);
# - a build with build/bin/indigo_server and both Alpaca drivers.
#
# Isolation: OmniSim, indigo_server and every ConformU process get a private
# HOME under --outdir (OmniSim also CFFIXED_USER_HOME and ASCOM_LOGPATH, see
# indigo_test/integration/system_alpaca/omnisim_test_common.h). The bridge
# starts from a written configuration with discovery disabled and OmniSim as
# its only server, so no other Alpaca server on the network is proxied. Ports
# are chosen free on 127.0.0.1.
#
# ConformU settings: the defaults ConformU writes itself, with FocuserTimeout
# and RotatorTimeout of 300 s. The OmniSim focuser moves about 480 steps/s, so
# its full range does not fit into the default 60 s even without INDIGO.
#
# Counting follows agent_alpaca README.md: every OK and INFO result line of the
# conformance log is a passed test, every ISSUE and ERROR line a failed one. The
# summaries at the end of the log repeat lines and are not counted.
#
# Exit code: 0 when the chain has no problem the reference does not have,
# 1 otherwise, 2 when the environment is incomplete.

import argparse
import glob
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import time
import urllib.request

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
INDIGO_SERVER = os.path.join(ROOT, 'build', 'bin', 'indigo_server')
DEFAULT_CONFORMU = '/Applications/ConformU.app/Contents/Resources/conformu'
TIMEOUT_OVERRIDES = {'FocuserTimeout': 300, 'RotatorTimeout': 300}
# the member that tells whether a device still moves after a run, read before the next run starts
MOTION_MEMBER = {'focuser': 'ismoving', 'rotator': 'ismoving', 'dome': 'slewing', 'telescope': 'slewing'}
RESULT_LINE = re.compile(r'^\d\d:\d\d:\d\d\.\d+ (\S.{0,34}?)\s+(OK|INFO|ISSUE|ERROR|WARN)\s+(.*?)\s*$')
END_OF_TESTS = 'Conformance test has finished'
# an abandoned run ends without that line, its summaries start here
SUMMARY_START = ('Further tests abandoned', 'Issue Summary', 'Error Summary')


def log(text):
	print(text, flush=True)


def free_port():
	with socket.socket() as s:
		s.bind(('127.0.0.1', 0))
		return s.getsockname()[1]


def get_json(url, timeout=5):
	with urllib.request.urlopen(url, timeout=timeout) as response:
		return json.load(response)


def wait_for(predicate, timeout, what):
	end = time.time() + timeout
	while time.time() < end:
		try:
			result = predicate()
			if result:
				return result
		except Exception:
			pass
		time.sleep(0.5)
	raise RuntimeError('timed out waiting for ' + what)


def find_omnisim(path):
	if path:
		return path if os.access(path, os.X_OK) else None
	for candidate in sorted(glob.glob(os.path.join(ROOT, '..', 'omnisim', '*', 'ascom.alpaca.simulators'))):
		if os.access(candidate, os.X_OK):
			return os.path.abspath(candidate)
	return None


def private_home(path):
	os.makedirs(path, exist_ok=True)
	return path


def start_omnisim(executable, port, outdir):
	home = private_home(os.path.join(outdir, 'omnisim_home'))
	os.makedirs(os.path.join(home, 'logs'), exist_ok=True)
	env = dict(os.environ)
	env.pop('XDG_CONFIG_HOME', None)
	env.pop('XDG_DATA_HOME', None)
	env.update(HOME=home, CFFIXED_USER_HOME=home, ASCOM_LOGPATH=os.path.join(home, 'logs'))
	output = open(os.path.join(outdir, 'omnisim.out'), 'w')
	process = subprocess.Popen([executable, '--urls=http://127.0.0.1:%d' % port], cwd=home, env=env, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
	wait_for(lambda: get_json('http://127.0.0.1:%d/management/apiversions' % port), 60, 'OmniSim')
	return process


def bridge_config(omnisim_port):
	return (
		"<newSwitchVector device='Alpaca' name='X_ALPACA_DISCOVERY'>\n"
		"<oneSwitch name='ENABLED'>Off</oneSwitch>\n"
		"<oneSwitch name='DISABLED'>On</oneSwitch>\n"
		"</newSwitchVector>\n"
		"<newTextVector device='Alpaca' name='X_ALPACA_SERVERS'>\n"
		"<oneText name='LIST'>127.0.0.1:%d</oneText>\n"
		"<oneText name='ADD'></oneText>\n"
		"<oneText name='REMOVE'></oneText>\n"
		"</newTextVector>\n" % omnisim_port
	)


def start_indigo(port, omnisim_port, outdir, verbosity='-v'):
	home = private_home(os.path.join(outdir, 'indigo_home'))
	os.makedirs(os.path.join(home, '.indigo'), exist_ok=True)
	# the configuration file of a server on another port than 7624 carries the port in its name
	name = 'Alpaca.config' if port == 7624 else 'Alpaca_%d.config' % port
	with open(os.path.join(home, '.indigo', name), 'w') as f:
		f.write(bridge_config(omnisim_port))
	env = dict(os.environ, HOME=home)
	output = open(os.path.join(outdir, 'indigo_server.out'), 'w')
	return subprocess.Popen([INDIGO_SERVER, '--', '-p', str(port), '-b-', '-w-', '-c-', verbosity, 'indigo_system_alpaca', 'indigo_agent_alpaca'], cwd=ROOT, env=env, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)


def stop(process):
	if process is None or process.poll() is not None:
		return
	try:
		os.killpg(process.pid, signal.SIGTERM)
		process.wait(10)
	except Exception:
		try:
			os.killpg(process.pid, signal.SIGKILL)
		except Exception:
			pass


def configured_devices(port):
	return get_json('http://127.0.0.1:%d/management/v1/configureddevices' % port)['Value']


def normalized_name(name):
	return ' '.join(re.sub(r'alpaca', ' ', name, flags=re.IGNORECASE).split()).lower()


# The agent device of an OmniSim device. system_alpaca names a proxy "ALPACA <DeviceName>" without any "alpaca" of the DeviceName
# (system_alpaca proxy_base_name()); the only proxy of the type is taken when no name matches.
def proxy_number(exported, device):
	candidates = [d for d in exported if d['DeviceType'].lower() == device['DeviceType'].lower() and d['DeviceName'].startswith('ALPACA ')]
	matching = [d for d in candidates if normalized_name(d['DeviceName']) == normalized_name(device['DeviceName'])]
	if len(matching) == 1:
		return matching[0]['DeviceNumber']
	if len(candidates) == 1:
		return candidates[0]['DeviceNumber']
	return None


def conformu_settings(conformu, probe_url, outdir, trace=False):
	# ConformU fills a settings file it is given with its defaults (a partial file is replaced), so it writes the
	# complete file during a short probe run; the overrides are applied to that file.
	home = private_home(os.path.join(outdir, 'conformu_probe'))
	path = os.path.join(outdir, 'conformu_settings.json')
	with open(path, 'w') as f:
		f.write('{}\n')
	subprocess.run([conformu, 'conformance', probe_url, '-n', os.path.join(home, 'probe.log'), '-s', path], cwd=home, env=dict(os.environ, HOME=home), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
	with open(path) as f:
		settings = json.load(f)
	overrides = dict(TIMEOUT_OVERRIDES)
	if trace:
		overrides.update(Debug=True, TraceAlpacaCalls=True)
	for key, value in overrides.items():
		if key not in settings:
			raise RuntimeError('ConformU settings have no ' + key)
		settings[key] = value
	with open(path, 'w') as f:
		json.dump(settings, f, indent=2)
	return path


def wait_idle(base_url, device_type):
	member = MOTION_MEMBER.get(device_type)
	if member is None:
		return
	end = time.time() + 600
	while time.time() < end:
		try:
			reply = get_json('%s/%s' % (base_url, member))
			if reply.get('ErrorNumber', 0) != 0 or reply.get('Value') is not True:
				return
		except Exception:
			return
		time.sleep(1)


def run_conformu(conformu, url, name, settings, outdir):
	home = private_home(os.path.join(outdir, 'conformu_' + name))
	run_settings = os.path.join(home, 'settings.json')
	shutil.copyfile(settings, run_settings)
	log_path = os.path.join(outdir, name + '.log')
	started = time.time()
	try:
		subprocess.run([conformu, 'conformance', url, '-n', log_path, '-r', os.path.join(outdir, name + '.json'), '-s', run_settings], cwd=home, env=dict(os.environ, HOME=home), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=3600)
	except subprocess.TimeoutExpired:
		pass
	return log_path, time.time() - started


def parse_log(path):
	result = {'passed': 0, 'failed': 0, 'problems': [], 'interface': None, 'complete': False}
	if not os.path.exists(path):
		return result
	with open(path, encoding='utf-8', errors='replace') as f:
		for line in f:
			if END_OF_TESTS in line:
				result['complete'] = True
				break
			if any(marker in line for marker in SUMMARY_START):
				break
			match = RESULT_LINE.match(line.rstrip('\n'))
			if not match:
				continue
			test, status, message = match.group(1).strip(), match.group(2), match.group(3)
			if status in ('OK', 'INFO'):
				result['passed'] += 1
				if test == 'InterfaceVersion' and status == 'OK':
					result['interface'] = message
			elif status in ('ISSUE', 'ERROR'):
				result['failed'] += 1
				result['problems'].append((test, status, message))
	return result


def compare(reference, chain):
	# problems are matched by test name and status, messages carry positions and times that differ between runs
	def keys(problems):
		counted = {}
		for test, status, _ in problems:
			counted[(test, status)] = counted.get((test, status), 0) + 1
		return counted
	ref, chn = keys(reference['problems']), keys(chain['problems'])
	only_chain = [p for p in chain['problems'] if chn[(p[0], p[1])] > ref.get((p[0], p[1]), 0)]
	only_reference = [p for p in reference['problems'] if ref[(p[0], p[1])] > chn.get((p[0], p[1]), 0)]
	return only_chain, only_reference


def main():
	parser = argparse.ArgumentParser(description='ConformU against OmniSim directly and through system_alpaca and agent_alpaca.')
	parser.add_argument('--outdir', required=True, help='directory for logs, results and report.md (created, must be empty)')
	parser.add_argument('--only', help='comma separated Alpaca device types to test, e.g. focuser,rotator')
	parser.add_argument('--omnisim', default=os.environ.get('INDIGO_TEST_OMNISIM'), help='OmniSim executable')
	parser.add_argument('--conformu', default=os.environ.get('CONFORMU', DEFAULT_CONFORMU), help='ConformU CLI executable')
	parser.add_argument('--no-reference', action='store_true', help='run only the chain')
	parser.add_argument('--trace', action='store_true', help='ConformU logs every Alpaca call and indigo_server logs at debug level')
	args = parser.parse_args()

	outdir = os.path.abspath(args.outdir)
	if os.path.exists(outdir) and os.listdir(outdir):
		log('--outdir %s is not empty' % outdir)
		return 2
	os.makedirs(outdir, exist_ok=True)
	omnisim = find_omnisim(args.omnisim)
	missing = [what for what, ok in (('OmniSim', omnisim), ('ConformU ' + args.conformu, os.access(args.conformu, os.X_OK)), (INDIGO_SERVER, os.access(INDIGO_SERVER, os.X_OK))) if not ok]
	if missing:
		log('missing: ' + ', '.join(missing))
		return 2
	only = set(t.strip().lower() for t in args.only.split(',')) if args.only else None

	omnisim_process = indigo_process = None
	rows, details = [], []
	try:
		omnisim_port, indigo_port = free_port(), free_port()
		omnisim_process = start_omnisim(omnisim, omnisim_port, outdir)
		omnisim_url = 'http://127.0.0.1:%d' % omnisim_port
		version = get_json(omnisim_url + '/management/v1/description')['Value'].get('ManufacturerVersion', '?').split('+')[0]
		devices = all_devices = sorted(configured_devices(omnisim_port), key=lambda d: (d['DeviceType'].lower(), d['DeviceNumber']))
		if only:
			devices = [d for d in devices if d['DeviceType'].lower() in only]
		log('OmniSim %s on port %d: %s' % (version, omnisim_port, ', '.join('%s %s' % (d['DeviceType'], d['DeviceName']) for d in devices)))

		indigo_process = start_indigo(indigo_port, omnisim_port, outdir, '-vv' if args.trace else '-v')
		wait_for(lambda: configured_devices(indigo_port) is not None, 30, 'indigo_server')
		# the bridge attaches every device it proxies; the agent lists the ones it can export, so the list is final when
		# it stops changing
		previous, stable_since = None, time.time()
		end = time.time() + 60
		while time.time() < end:
			current = sorted((d['DeviceType'], d['DeviceName']) for d in configured_devices(indigo_port))
			if current != previous:
				previous, stable_since = current, time.time()
			elif current and time.time() - stable_since > 5:
				break
			time.sleep(0.5)
		exported = configured_devices(indigo_port)
		log('agent_alpaca on port %d exports %d devices' % (indigo_port, len(exported)))

		probe = next((d for d in all_devices if d['DeviceType'].lower() == 'safetymonitor'), all_devices[0])
		settings = conformu_settings(args.conformu, '%s/api/v1/%s/%d' % (omnisim_url, probe['DeviceType'].lower(), probe['DeviceNumber']), outdir, args.trace)

		for device in devices:
			device_type = device['DeviceType'].lower()
			stem = '%s_%s' % (device_type, re.sub(r'[^A-Za-z0-9.-]+', '_', device['DeviceName']).strip('_'))
			reference_url = '%s/api/v1/%s/%d' % (omnisim_url, device_type, device['DeviceNumber'])
			chain_number = proxy_number(exported, device)
			reference = chain = None
			if not args.no_reference:
				wait_idle(reference_url, device_type)
				path, seconds = run_conformu(args.conformu, reference_url, stem + '_reference', settings, outdir)
				reference = parse_log(path)
				log('%-16s reference %3d/%-3d %4.0f s' % (device['DeviceType'], reference['passed'], reference['passed'] + reference['failed'], seconds))
			if chain_number is None:
				log('%-16s chain     not exported by agent_alpaca' % device['DeviceType'])
			else:
				wait_idle(reference_url, device_type)
				path, seconds = run_conformu(args.conformu, 'http://127.0.0.1:%d/api/v1/%s/%d' % (indigo_port, device_type, chain_number), stem + '_chain', settings, outdir)
				chain = parse_log(path)
				log('%-16s chain     %3d/%-3d %4.0f s' % (device['DeviceType'], chain['passed'], chain['passed'] + chain['failed'], seconds))
			rows.append((device, reference, chain))
	finally:
		stop(indigo_process)
		stop(omnisim_process)

	regressions = 0
	lines = ['# ConformU: OmniSim directly and through system_alpaca and agent_alpaca', '', 'OmniSim %s, ConformU settings `%s`.' % (version, ', '.join('%s %s' % kv for kv in TIMEOUT_OVERRIDES.items())), '',
		'| Device | Interface (ref / chain) | Reference | Chain | Only in chain | Only in reference |', '|---|---|---|---|---|---|']
	for device, reference, chain in rows:
		def cell(r):
			if r is None:
				return 'not run'
			return '%d/%d%s' % (r['passed'], r['passed'] + r['failed'], '' if r['complete'] else ' (incomplete)')
		only_chain = only_reference = []
		if reference and chain:
			only_chain, only_reference = compare(reference, chain)
		elif chain:
			only_chain = chain['problems']
		if chain is not None and (only_chain or not chain['complete']):
			regressions += 1
		lines.append('| %s, %s | %s / %s | %s | %s | %d | %d |' % (device['DeviceType'], device['DeviceName'], reference['interface'] if reference else '-', chain['interface'] if chain else '-', cell(reference), 'not exported' if chain is None else cell(chain), len(only_chain), len(only_reference)))
		if only_chain or only_reference:
			details.append('')
			details.append('## %s, %s' % (device['DeviceType'], device['DeviceName']))
			for title, problems in (('Only in the chain', only_chain), ('Only in the reference', only_reference)):
				if problems:
					details.append('')
					details.append(title + ':')
					details.append('')
					for test, status, message in problems:
						details.append('- %s %s: %s' % (test, status, message))
	report = '\n'.join(lines + details) + '\n'
	with open(os.path.join(outdir, 'report.md'), 'w') as f:
		f.write(report)
	log('')
	log(report)
	return 1 if regressions else 0


if __name__ == '__main__':
	sys.exit(main())
