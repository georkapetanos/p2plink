#!/usr/bin/python3
import pynmea2, serial, time, json, yaml, os
import lrd
import paho.mqtt.publish as publish
from datetime import date

def log_local(json_message):
	curdate = date.today().strftime("%Y-%m-%d.log")

	f = open("./logs/"+curdate, "a")
	f.write(json_message+"\n")
	f.flush()

def main():

	# open config file located in the same directory
	config = open('config.yaml', 'r')
	doc = yaml.load(config, Loader=yaml.SafeLoader)
	username = doc["mqtt_username"]
	password = str(doc["mqtt_password"])
	mqtt_hostname = doc["mqtt_hostname"]
	mqtt_port = str(doc["mqtt_port"])


	serial_port = serial.Serial("/dev/serial0", baudrate=9600)
	serial_port.flushInput()
	serial_port.flushOutput()

	#skip some lines, there are a lot of them
	serial_port.readline()
	serial_port.readline()
	serial_port.readline()

	while (True):
		json_message = lrd.receive_encrypted()
		#print("received: "+json_message)
		json_object = json.loads(json_message)
		payload = json_object["p"]

		#Get current GNSS fix
		while (True):
			nmea_sentence = serial_port.readline().decode('utf-8')
			msg = pynmea2.parse(nmea_sentence)
			if (msg.sentence_type == "GGA"):
				final_data = "{:.7f}".format(msg.latitude)+" "+"{:.7f}".format(msg.longitude)+" "+str(msg.altitude)+" "+str(msg.num_sats)
				break
		#print(final_data)
		new_json = lrd.generate_json_str(final_data + " " + payload)

		#print(lrd.lora_str_translate(new_json))
		log_local(new_json)
		publish.single("lrdlink", new_json, hostname=mqtt_hostname, auth={'username':username,'password':password})

if __name__ == "__main__":
	main()
