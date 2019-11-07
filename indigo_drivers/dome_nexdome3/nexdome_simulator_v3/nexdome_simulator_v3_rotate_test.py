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
                  
        print "Disable battery status"
        serial_port.write("@DisBatStatus\n")
        received = ""
        while 0 > received.find(":BateryStatusDisabled#"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;                   
                  
    
        print "Current state:"
        serial_port.write("@SRR\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;        
        
        print "Rotate to 10 deg"
        serial_port.write("@GAR,10\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;

        print "RESET Xbee";
        serial_port.write("@XBReset\n")
                
        print "Rotate to 180 deg"
        serial_port.write("@GAR,180\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;

        print "Rotate to 359 deg"
        serial_port.write("@GAR,359\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;                
               
        print "Rotate to 120 deg"
        serial_port.write("@GAR,120\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;
                
        print "Rotate to 0 deg"
        serial_port.write("@GAR,0\n")
        received = ""
        while 0 > received.find(":SER"):
            sleep(0.1);
            received = serial_port.read(1000)
            if received:
                print received;
                

        serial_port.close()
    
    except serial.serialutil.SerialException:
        print 'Serial port not available!'
    
if __name__ == "__main__":
    main()
