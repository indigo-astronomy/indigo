import serial
from time import sleep
import sys
from sys import platform as _platform


def main():

    if len(sys.argv) > 1:
        serial_port_name = sys.argv[1]
    elif _platform == "linux" or _platform == "linux2":
        serial_port_name = '/dev/ttyACM0'
        print "Connecting to "
    elif _platform == "win32" or _platform == "win64":
        serial_port_name = 'COM7'

    print "Connecting to " + serial_port_name + "..."
    
    
    try:
        #Connect to serial port
        serial_port = serial.Serial(
                  port = serial_port_name,\
                  baudrate = 9600,\
                  parity = serial.PARITY_NONE,\
                  stopbits = serial.STOPBITS_ONE,\
                  bytesize = serial.EIGHTBITS,\
                  timeout = 0)
    
    
        # Request open
        serial_port.write("@OPS\n")
        received = ""
        i = 0;
        while ((0 > received.find(":SES")) and (20 > i)) :
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;
                i = i + 1;
            if 10 == i:
                #Abort operation
                serial_port.write("@SWS\n");  
        
        i = 0;
        while (10 > i):
            received = serial_port.read(1000)
            if received:
                print received;
                i = i+ 1;
            
        # Request open to open fully        
        serial_port.write("@OPS\n")
        received = ""
        while 0 > received.find(":SES"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;

        # Request close
        serial_port.write("@CLS\n")
        received = ""
        i = 0;
        while ((0 > received.find(":SES")) and (20 > i)) :
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;
                i = i + 1;
            if 10 == i:
                #Abort operation
                serial_port.write("@SWS\n");

        i = 0;
        while (10 > i):
            received = serial_port.read(1000)
            if received:
                print received;
                i = i+ 1;
            
        # Request close to close fully        
        serial_port.write("@CLS\n")
        received = ""
        while 0 > received.find(":SES"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;
    
        serial_port.close()
    
    except serial.serialutil.SerialException:
        print 'Serial port not available!'
    
if __name__ == "__main__":
    main()
