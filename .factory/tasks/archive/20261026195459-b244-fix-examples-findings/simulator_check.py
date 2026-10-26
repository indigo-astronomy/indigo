"""Isolated acceptance checks against the already-built simulator binaries."""
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time

root = Path.cwd()
env = os.environ.copy()
for key in ('DYLD_LIBRARY_PATH', 'LD_LIBRARY_PATH'):
	env[key] = str(root / 'build/lib') + ':' + str(root / 'build/drivers')
with tempfile.TemporaryDirectory(prefix='example-acceptance-') as directory:
	work = Path(directory)
	env['HOME'] = str(work / 'home')
	Path(env['HOME']).mkdir()
	def run(name, arguments, images):
		log = work / (name + '.log')
		with log.open('w') as output:
			command = [str(root / 'indigo_examples' / name)] + arguments
			process = subprocess.Popen(command, cwd=work, env=env, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
			try:
				code = process.wait(timeout=50)
			except subprocess.TimeoutExpired:
				os.killpg(process.pid, signal.SIGKILL)
				process.wait()
				raise
		text = log.read_text()
		assert code == 0, (name, code, text[-3000:])
		files = list(work.glob('*.fits'))
		assert len(files) == images, (name, len(files), text[-3000:])
		for image in files:
			assert image.read_bytes().startswith(b'SIMPLE  ='), image
			image.unlink()
		print(name + ': PASS, exit 0, ' + str(images) + ' FITS files', flush=True)
	for name, images in [('client', 0), ('executable_driver_client', 5)]:
		run(name, [str(root / 'build/drivers/indigo_ccd_simulator')], images)
	run('dynamic_driver_client', [], 5)
	with socket.socket() as sock:
		sock.bind(('127.0.0.1', 0))
		port = sock.getsockname()[1]
	with (work / 'server.log').open('w') as output:
		server = subprocess.Popen([str(root / 'build/bin/indigo_server'), '--', '-p', str(port), '-b-', '-w-', '-c-', str(root / 'build/drivers/indigo_ccd_simulator.dylib'), str(root / 'build/drivers/indigo_mount_simulator.dylib')], cwd=work, env=env, stdout=output, stderr=subprocess.STDOUT)
		try:
			for attempt in range(50):
				try:
					with socket.create_connection(('127.0.0.1', port), timeout=.2):
						break
				except OSError:
					time.sleep(.1)
			else:
				raise AssertionError('server did not listen')
			for name, device, images in [('remote_server_client', 'CCD Imager Simulator @ example', 1), ('remote_server_client_mount', 'Mount Simulator @ example', 0)]:
				run(name, ['127.0.0.1', 'example', device, '35', str(port)], images)
			result = subprocess.run([str(root / 'indigo_examples/remote_server_client'), '127.0.0.1', 'example', 'Missing device @ example', '1', str(port)], cwd=work, env=env, capture_output=True, text=True, timeout=15)
			assert result.returncode == 1 and 'Timed out' in result.stderr + result.stdout
			print('remote missing device: PASS, exit 1 within deadline', flush=True)
		finally:
			server.terminate()
			try:
				server.wait(timeout=10)
			except subprocess.TimeoutExpired:
				server.kill()
				server.wait()
