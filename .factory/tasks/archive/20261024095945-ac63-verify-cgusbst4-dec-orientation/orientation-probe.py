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
    ra_text = mount.command(':GRH#')
    dec_text = mount.command(':GDH#')
    motors = [int(x) for x in mount.command(':GX94#').split(':')]
    return dict(ra_text=ra_text, dec_text=dec_text, ra_arcsec=coordinate(ra_text)*15, dec_arcsec=coordinate(dec_text), motors=motors, status=mount.command(':GU#'), side=mount.command(':Gm#'))

def target_ra(text):
    seconds = round(coordinate(text)) % 86400
    return f'{seconds//3600:02d}:{seconds//60%60:02d}:{seconds%60:02d}'

mount = Serial('/dev/ttyUSB0', termios.B115200)
adapter = Serial('/dev/ttyACM0', termios.B9600)
started = False
cleanup_errors = []
try:
    assert mount.command(':GVP#') == 'NYX-101'
    mount.command(':GVN#')
    assert adapter.command('\x06') == 'A'
    initial = snapshot()
    log('initial', **initial)
    assert 'P' in initial['status'] and 'n' in initial['status'] and initial['status'].endswith('160')
    assert initial['dec_arcsec'] > 89*3600
    mount.command(':GX90#')
    saved_ra = mount.command(':Gr#')
    saved_dec = mount.command(':Gd#')
    log('saved_target', ra=saved_ra, dec=saved_dec)
    started = True
    assert mount.command(':hR#') == '1'
    assert mount.command(':Te#') == '1'
    mount.command(':R1#', wait=0)
    time.sleep(1)
    departure = snapshot()
    ra = target_ra(departure['ra_text'])
    assert mount.command(':Sr'+ra+'#') == '1'
    assert mount.command(':Sd+70*00:00#') == '1'
    log('goto_target', ra=ra, dec='+70*00:00')
    assert mount.command(':MS#') == '0'
    deadline = time.monotonic()+120
    arrived = False
    while time.monotonic() < deadline:
        current = snapshot()
        log('goto_progress', **current)
        # Bound any unexpected RA-axis travel rather than permitting a long meridian flip.
        assert abs(current['motors'][0]-departure['motors'][0]) < 180000, 'Unexpected RA-axis excursion'
        assert current['dec_arcsec'] > 69*3600, 'DEC exceeded approved excursion'
        if 'N' in current['status'] and abs(current['dec_arcsec']-70*3600) < 36:
            arrived = True
            break
        time.sleep(0.5)
    assert arrived, 'Slew did not settle at +70 degrees'
    time.sleep(2)
    settled = snapshot()
    assert abs(settled['dec_arcsec']-70*3600)<36 and settled['side'] in ['E','W']
    log('settled', **settled)
    trials=[('idle',None,None)]
    for repeat in range(2):
        trials += [(f'direct_n_{repeat}',mount,':Mgn2000#'), (f'adapter_0_{repeat}',adapter,':Mg02000#'), (f'adapter_1_{repeat}',adapter,':Mg12000#'), (f'direct_s_{repeat}',mount,':Mgs2000#')]
    for name, port, command in trials:
        before=snapshot()
        start=time.monotonic()
        if port:
            port.command(command,wait=0)
        time.sleep(0.25)
        during=snapshot()
        time.sleep(max(0,start+2.5-time.monotonic()))
        after=snapshot()
        log('trial',name=name,command=command,duration_ms=2000,before=before,during=during,after=after,delta_dec_arcsec=after['dec_arcsec']-before['dec_arcsec'],delta_motor_steps=[a-b for a,b in zip(after['motors'],before['motors'])])
        assert before['side']==during['side']==after['side']==settled['side'], 'Pier side changed during trial'
        assert abs(after['dec_arcsec']-70*3600)<180, 'Unexpected DEC excursion during guide test'
        time.sleep(0.3)
finally:
    if started:
        # Adapter pulse commands are self-timed and cannot be cancelled through a reply API.
        time.sleep(2.1)
        for command,wait in [(':Q#',0),(':Td#',0.35),(':hP#',0.35)]:
            try:
                reply=mount.command(command,wait=wait)
                log('cleanup_command',command=command,reply=reply)
                if wait and reply!='1':
                    cleanup_errors.append(command+' rejected')
            except Exception as error:
                cleanup_errors.append(command+': '+str(error))
        parked=False
        for attempt in range(120):
            try:
                status=mount.command(':GU#')
                if 'P' in status and 'n' in status:
                    parked=True
                    break
            except Exception as error:
                cleanup_errors.append('park poll: '+str(error))
                break
            time.sleep(0.5)
        try:
            mount.command(':R6#',wait=0)
            final=snapshot()
            log('cleanup_final',parked=parked,errors=cleanup_errors,**final)
            if not parked or not final['status'].endswith('160'):
                cleanup_errors.append('park/rate restoration incomplete')
            if saved_ra and saved_dec:
                # Preserve the pre-experiment target without changing the stored park position.
                log('restore_target',ra_reply=mount.command(':Sr'+saved_ra+'#'),dec_reply=mount.command(':Sd'+saved_dec+'#'))
        except Exception as error:
            cleanup_errors.append('restore: '+str(error))
    adapter.close()
    mount.close()
    if cleanup_errors:
        raise RuntimeError('Cleanup incomplete: '+repr(cleanup_errors))
