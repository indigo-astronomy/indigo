#!/usr/bin/env python
import serial
from time import sleep
import sys
from sys import platform as _platform


def main():

    if len(sys.argv) > 2:
        serial_port_name = sys.argv[2]
    elif _platform == "linux" or _platform == "linux2":
        serial_port_name = '/dev/ttyACM0'
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
    
        #Open the file and read
	file_name = "baader_command_set.txt"
	if len(sys.argv) > 1:
	    file_name = sys.argv[1]

        f = open(file_name, "r")
        command_set = f.readlines()
    
        #For each command in file:
    
        for current_command in command_set:
            #Send command via serial port
            command = current_command.rstrip()
            res = serial_port.write(command)
	    serial_port.flush()
            sleep(1);
            received = serial_port.read(1000)
            print res, " ", command, " -> ", received
    
        serial_port.close()
    
    except serial.serialutil.SerialException:
        print 'Serial port not available!'
    
if __name__ == "__main__":
    main()
