"""NYX same-ST4-cable comparison with independent coordinates and motor telemetry."""
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

mount = Serial('/dev/serial/by-id/usb-Pegasus_Astro_NYX-101_NYX467edc0c-if00-port0', termios.B115200)
try:
    for sample in range(4):
        log('park_confirmation', sample=sample, **snapshot())
        time.sleep(0.3)
finally:
    mount.close()
