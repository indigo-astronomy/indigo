"""Bounded CGUSB-ST4 dialect test with independent NYX serial observations."""
import os
import termios
import tty
import select
import time
import json
import sys

class Serial:
    def __init__(self, path, speed):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = termios.tcgetattr(self.fd)
        tty.setraw(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[4] = attrs[5] = speed
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
    def command(self, command, wait=0.35):
        termios.tcflush(self.fd, termios.TCIFLUSH)
        os.write(self.fd, command.encode('ascii'))
        end = time.monotonic() + wait
        data = b''
        while time.monotonic() < end:
            if select.select([self.fd], [], [], max(0, end - time.monotonic()))[0]:
                data += os.read(self.fd, 4096)
                if b'#' in data or command == '\x06':
                    break
        reply = data.decode('ascii', errors='replace').strip('\x00\r\n#')
        log('command', port=self.fd, command=command, reply=reply)
        return reply
    def close(self):
        termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
        os.close(self.fd)

def log(event, **values):
    print(json.dumps(dict(event=event, monotonic=time.monotonic(), **values)), flush=True)

def coordinate(text):
    values = text.replace('*', ':').split(':')
    sign = -1 if values[0].startswith('-') else 1
    return sign * (abs(float(values[0])) * 3600 + float(values[1]) * 60 + float(values[2]))

def snapshot():
    ra = coordinate(mount.command(':GRH#')) * 15
    dec = coordinate(mount.command(':GDH#'))
    motors = [int(x) for x in mount.command(':GX94#').split(':')]
    return dict(ra=ra, dec=dec, motors=motors, status=mount.command(':GU#'))

mount = Serial('/dev/ttyUSB0', termios.B115200)
adapter = Serial('/dev/ttyACM0', termios.B9600)
started = False
try:
    assert mount.command(':GVP#') == 'NYX-101'
    mount.command(':GVN#')
    assert adapter.command('\x06') == 'A'
    initial = snapshot()
    log('initial', **initial)
    assert 'P' in initial['status'] and 'n' in initial['status'], 'Expected initially parked with tracking off'
    mount.command(':GX90#')
    started = True
    assert mount.command(':hR#') == '1', 'Unpark failed'
    assert mount.command(':Te#') == '1', 'Enable tracking failed'
    time.sleep(1)
    active = snapshot()
    assert 'p' in active['status'] and 'n' not in active['status'], 'Not unparked and tracking'
    if len(sys.argv) > 1 and sys.argv[1] == 'full':
        tests = [('idle', None, None)] + [('adapter_' + d, adapter, ':Mg' + d + ' 500#') for d in ['s', '1', 'n', '0', 'e', '2', 'w', '3']] + [('direct_' + d, mount, ':Mg' + d + '500#') for d in ['s', 'n', 'e', 'w']]
    else:
        tests = [('idle', None, None), ('letter_south', adapter, ':Mgs2000#'), ('digit_south', adapter, ':Mg12000#'), ('direct_north', mount, ':Mgn2000#')]
    for name, port, command in tests:
        before = snapshot()
        start = time.monotonic()
        if port:
            port.command(command, wait=0)
        time.sleep(0.1)
        during = snapshot()
        time.sleep(max(0, start + 0.9 - time.monotonic()))
        after = snapshot()
        delta_ra = (after['ra'] - before['ra'] + 648000) % 1296000 - 648000
        log('trial', name=name, before=before, during=during, after=after, elapsed=time.monotonic() - start, delta_ra_arcsec=delta_ra, delta_dec_arcsec=after['dec'] - before['dec'], delta_motor_steps=[a-b for a,b in zip(after['motors'], before['motors'])])
        time.sleep(0.3)
finally:
    if started:
        mount.command(':Q#', wait=0)
        mount.command(':Td#')
        log('cleanup_park_request', reply=mount.command(':hP#'))
        for i in range(20):
            status = mount.command(':GU#')
            if 'P' in status:
                break
            time.sleep(0.5)
        log('cleanup_final', status=status)
    adapter.close()
    mount.close()
