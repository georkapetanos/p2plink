#!/usr/bin/python3
import pynmea2,serial, time
import lrd

def main():
	serial_port = serial.Serial("/dev/serial0", baudrate=9600)
	serial_port.flushInput()
	serial_port.flushOutput()
	
	while (True):
		json_message = receive_encrypted()
		print(json_message)

if __name__ == "__main__":
	main()
