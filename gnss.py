#!/usr/bin/python3
import pynmea2,serial, time
import lrd

TRANSMIT_INTERVAL = 30

def main():
	serial_port = serial.Serial("/dev/serial0", baudrate=9600)
	serial_port.flushInput()
	serial_port.flushOutput()

	# skip first line
	serial_port.readline()

	start = time.time()

	while (True):
		nmea_sentence = serial_port.readline().decode('utf-8')
		#print(nmea_sentence)
		msg = pynmea2.parse(nmea_sentence)

		if (msg.sentence_type == "GGA"):
			final_data = str(msg.latitude)+" "+str(msg.longitude)+" "+str(msg.altitude)+" "+str(msg.num_sats)
			print(final_data + " "+ str(time.time() - start))
			if (time.time() - start > TRANSMIT_INTERVAL):
				print("Transmitting over LoRa...")
				#lrd.transmit_encrypted(1, final_data, "/dev/ttyUSB0")
				start = time.time()

if __name__ == "__main__":
	main()
