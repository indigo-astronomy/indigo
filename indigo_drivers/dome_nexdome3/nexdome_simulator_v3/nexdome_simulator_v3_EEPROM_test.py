import serial
from time import sleep
import sys
from sys import platform as _platform

def send_command (serial_port, command, exit_conditon1, exit_conditon2):
    print command
    serial_port.write("@"+command+"\n")
    received = ""
    while 0 > received.find(exit_conditon1) and 0 > received.find(exit_conditon2):
        sleep(0.1);
        received = serial_port.read(1000)
        if received:
            print received;

            
def print_rotator_configuration(serial_port):
    send_command(serial_port, "ARR", ":E", ":A") 
    send_command(serial_port, "DRR", ":E", ":D")
    send_command(serial_port, "HRR", ":E", ":H")
    send_command(serial_port, "RRR", ":E", ":R") 
    send_command(serial_port, "VRR", ":E", ":V")
            
def modify_rotator_configuration(serial_port):
    send_command(serial_port, "AWR,123", ":E", ":A") 
    send_command(serial_port, "DWR,456", ":E", ":D")
    send_command(serial_port, "HWR,321", ":E", ":H")
    send_command(serial_port, "RWR,35678", ":E", ":R") 
    send_command(serial_port, "VWR,1357", ":E", ":V")

def print_shutter_configuration(serial_port):
    send_command(serial_port, "ARS", ":E", ":A") 
    send_command(serial_port, "RRS", ":E", ":R") 
    send_command(serial_port, "VRS", ":E", ":V")
    
def modify_shutter_configuration(serial_port):
    send_command(serial_port, "AWS,1234", ":E", ":A") 
    send_command(serial_port, "RWS,23456", ":E", ":R")
    send_command(serial_port, "VWS,3456", ":E", ":V")

    

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
    
    
        # Delete EEPROM
        send_command(serial_port, "DeleteEEPROM", "EEPROMisClear", "EEPROMisClear")

        # Restore default rotator configuration
        print "Restore default rotator configuration"
        send_command(serial_port, "ZDR", ":E", ":Z")
        
        # Restore default shutter configuration
        print "Restore default shutter configuration"
        send_command(serial_port, "ZDS", ":E", ":Z")       

        
        
        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)
        
        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)
        
        # Load rotator configuration
        print "Load rotator configuration"
        send_command(serial_port, "ZRR", ":E", ":Z")

        # Load shutter configuration
        print "Load shutter configuration"
        send_command(serial_port, "ZRS", ":E", ":Z")
        
        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)

        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)
        
        # Modify current rotator configuration
        print "Modify current rotator configuration"
        modify_rotator_configuration(serial_port)
 
        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port) 
       
        # Save rotator configuration
        print "Save rotator configuration"
        send_command(serial_port, "ZWR", ":E", ":Z")        

        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)
        
        # Load shutter configuration
        print "Load shutter configuration"
        send_command(serial_port, "ZRS", ":E", ":Z")
        
        # Restore default rotator configuration
        print "Restore default rotator configuration"
        send_command(serial_port, "ZDR", ":E", ":Z")       

        # Print rotator configuration        
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)

        # Load rotator configuration        
        print "Load rotator configuration"
        send_command(serial_port, "ZRR", ":E", ":Z")

        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)
        
        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)
        
         # Modify current shutter configuration
        print "Modify current shutter configuration"
        modify_shutter_configuration(serial_port)       
 
        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port) 
 
        # Save shutter configuration
        print "Save shutter configuration"
        send_command(serial_port, "ZWS", ":E", ":Z") 
 
        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)  

        # Restore default shutter configuration
        print "Restore default shutter configuration"
        send_command(serial_port, "ZDS", ":E", ":Z")

        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)
        
        # Load shutter configuration
        print "Load shutter configuration"
        send_command(serial_port, "ZRS", ":E", ":Z")
        
        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)

        # Print shutter configuration
        print "Print shutter configuration"
        print_shutter_configuration(serial_port)

        # Restore default rotator configuration
        print "Restore default rotator configuration"
        send_command(serial_port, "ZDR", ":E", ":Z")       

        # Print rotator configuration        
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)

        # Load rotator configuration        
        print "Load rotator configuration"
        send_command(serial_port, "ZRR", ":E", ":Z")

        # Print rotator configuration
        print "Print rotator configuration"
        print_rotator_configuration(serial_port)        
        
        
        serial_port.close()
    
    except serial.serialutil.SerialException:
        print 'Serial port not available!'
    
if __name__ == "__main__":
    main()
