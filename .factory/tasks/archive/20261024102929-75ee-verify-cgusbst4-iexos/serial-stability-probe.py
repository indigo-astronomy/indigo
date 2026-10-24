"""Read-only PMC8 serial continuity check; no movement or setting commands."""
import os, termios, tty, select, time, json
path='/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AK06KTVZ-if00-port0'
fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
try:
    tty.setraw(fd)
    attrs=termios.tcgetattr(fd); attrs[4]=attrs[5]=termios.B115200
    termios.tcsetattr(fd,termios.TCSANOW,attrs)
    for sample in range(120):
        for command in ['ESGp0!','ESGp1!','ESGr0!','ESGr1!']:
            termios.tcflush(fd,termios.TCIFLUSH)
            os.write(fd,command.encode())
            data=b''; deadline=time.monotonic()+1.5
            while time.monotonic()<deadline:
                if select.select([fd],[],[],max(0,deadline-time.monotonic()))[0]:
                    chunk=os.read(fd,4096)
                    if not chunk: raise RuntimeError('Serial EOF')
                    data+=chunk
                    if b'!' in data: break
            reply=data.decode('ascii').strip('\x00\r\n')
            print(json.dumps(dict(sample=sample,monotonic=time.monotonic(),command=command,reply=reply,raw_hex=data.hex(),path=os.readlink(path))),flush=True)
            assert reply.startswith(command[:-1]) and reply.endswith('!'), reply
        time.sleep(0.4)
finally:
    os.close(fd)
