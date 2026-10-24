"""Bounded raw two-link orientation comparison, retaining one PMC8 session."""
import os
import termios
import tty
import select
import time
import json

def log(event, **values):
    print(json.dumps(dict(event=event, monotonic=time.monotonic(), **values)), flush=True)

class Serial:
    def __init__(self, path, speed, terminator):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = termios.tcgetattr(self.fd)
        tty.setraw(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[4] = attrs[5] = speed
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        self.terminator = terminator
    def command(self, command, wait=1.5):
        termios.tcflush(self.fd, termios.TCIFLUSH)
        os.write(self.fd, command.encode('ascii'))
        data = b''
        deadline = time.monotonic() + wait
        while time.monotonic() < deadline:
            if select.select([self.fd], [], [], max(0, deadline - time.monotonic()))[0]:
                data += os.read(self.fd, 4096)
                if self.terminator in data or command == '\x06':
                    break
        reply = data.decode('ascii', errors='replace').strip('\x00\r\n')
        log('command', fd=self.fd, command=command, reply=reply)
        return reply
    def close(self):
        termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
        os.close(self.fd)

def value(command, prefix, length):
    reply = mount.command(command)
    assert reply.startswith(prefix) and reply.endswith('!'), (command, reply)
    payload = reply[len(prefix):-1]
    assert len(payload) == length, (command, reply)
    return int(payload, 16)

def snapshot():
    positions = [value(f'ESGp{axis}!', f'ESGp{axis}', 6) for axis in range(2)]
    positions = [p - 0x1000000 if p & 0x800000 else p for p in positions]
    rates = [value(f'ESGr{axis}!', f'ESGr{axis}', 4) for axis in range(2)]
    directions = [value(f'ESGd{axis}!', f'ESGd{axis}', 1) for axis in range(2)]
    return dict(positions=positions, rates=rates, directions=directions, dec_arcsec=90*3600-abs(positions[1])*0.3125)

def set_command(command):
    reply = mount.command(command)
    expected = {'ESPt': 'ESGt', 'ESSd': 'ESGd', 'ESSr': 'ESGr', 'ESTr': 'ESGx'}[command[:4]] + command[4:]
    if command == 'ESPt300000!':
        expected = 'ESGt3!'
    assert reply == expected, (command, reply, expected)

def wait_position(target, restore=False):
    deadline = time.monotonic()+90
    while time.monotonic() < deadline:
        s = snapshot()
        log('position_progress', target=target, restoring=restore, **s)
        assert abs(s['positions'][1]) < 242000, 'DEC excursion beyond 21 degrees'
        assert abs(s['positions'][0]-initial['positions'][0]) < 30000, 'Unexpected RA excursion'
        if abs(s['positions'][1]-target[1]) <= 2 and (not restore or abs(s['positions'][0]-target[0]) <= 2) and s['rates'][1] == 0 and (not restore or s['rates'][0] == 0):
            return s
        time.sleep(0.25)
    raise RuntimeError('Finite point did not settle')

mount = Serial('/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AK06KTVZ-if00-port0', termios.B115200, b'!')
adapter = None
started = False
initial = None
try:
    firmware = mount.command('ESGv!')
    config = mount.command('ESGi!')
    assert 'ES20A01.1' in firmware and 'ES1A01CB20A01' in firmware, firmware
    assert config == 'ESGi1152000001040040000900060000001!', config
    initial = snapshot()
    log('initial', firmware=firmware, config=config, **initial)
    assert initial['positions'] == [0, 0] and initial['rates'] == [0, 0], 'Initial state differs from approved polar home'
    adapter = Serial('/dev/serial/by-id/usb-Astrogene1000_stuff_USB_to_ST4_Astrogene_1000_000000043B-if00', termios.B9600, b'#')
    assert adapter.command('\x06') == 'A'
    started = True
    for command in ['ESSd01!', 'ESTr0000!', 'ESSr10000!', 'ESPt1000000!']:
        set_command(command)
    assert snapshot()['positions'] == initial['positions']
    set_command('ESPt1038400!')  # +230400 counts = 20 degrees, northern W-side convention.
    wait_position([0, 230400])
    set_command('ESSd01!')
    set_command('ESTr04B3!')  # Sidereal RA tracking, 1203/25 steps per second.
    time.sleep(1)
    settled = snapshot()
    log('settled', **settled)
    assert settled['positions'][1] > 230000 and settled['rates'][0] > 0
    trials = [('idle', None)]
    for repeat in range(2):
        trials += [(f'direct_n_{repeat}', -48), (f'adapter_0_{repeat}', ':Mg02000#'), (f'adapter_1_{repeat}', ':Mg12000#'), (f'direct_s_{repeat}', 48)]
    for name, operation in trials:
        before = snapshot()
        if isinstance(operation, int):
            target = before['positions'][1]+operation
            set_command(f'ESPt1{target & 0xFFFFFF:06X}!')
            wait_position([0, target])
        elif operation:
            adapter.command(operation, wait=0)
        start = time.monotonic()
        time.sleep(0.4)
        during = snapshot()
        time.sleep(max(0, start+2.6-time.monotonic()))
        after = snapshot()
        log('trial', name=name, operation=operation, before=before, during=during, after=after, delta_dec_arcsec=after['dec_arcsec']-before['dec_arcsec'], delta_counts=[a-b for a,b in zip(after['positions'],before['positions'])])
        assert abs(after['positions'][1]-230400) < 1000 and after['rates'][1] == 0, 'Unexpected or continuing DEC motion'
finally:
    cleanup_errors = []
    restored_ok = not started
    if started:
        time.sleep(2.1)
        # Never let a reply assertion skip subsequent physical restoration commands.
        for command in ['ESPt300000!', 'ESTr0000!', 'ESSr10000!']:
            try:
                set_command(command)
            except Exception as error:
                cleanup_errors.append(str(error))
        try:
            log('cleanup_before_return', **snapshot())
        except Exception as error:
            cleanup_errors.append(str(error))
        for axis in range(2):
            try:
                set_command(f"ESPt{axis}{initial['positions'][axis] & 0xFFFFFF:06X}!")
            except Exception as error:
                cleanup_errors.append(str(error))
        try:
            wait_position(initial['positions'], restore=True)
            for axis in range(2):
                try:
                    set_command(f"ESSd{axis}{initial['directions'][axis]}!")
                except Exception as error:
                    cleanup_errors.append(str(error))
            restored = snapshot()
            log('restored', **restored)
            restored_ok = restored['positions'] == initial['positions'] and restored['rates'] == initial['rates'] and restored['directions'] == initial['directions']
        except Exception as error:
            cleanup_errors.append(str(error))
    log('cleanup', errors=cleanup_errors, restored_ok=restored_ok)
    if not restored_ok:
        # Keep the DTR session/counters alive for operator recovery; do not erase evidence.
        log('recovery_required_session_held_open')
        while True:
            time.sleep(10)
    if adapter:
        adapter.close()
    mount.close()
    if cleanup_errors:
        raise RuntimeError(cleanup_errors)
