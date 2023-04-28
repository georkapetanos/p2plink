#!/usr/bin/python3
import time, json, yaml, os, sys
import paho.mqtt.publish as publish
import lrd

def main():
	# open config file located in the same directory
	config = open('config.yaml', 'r')
	doc = yaml.load(config, Loader=yaml.SafeLoader)
	username = doc["mqtt_username"]
	password = str(doc["mqtt_password"])
	mqtt_hostname = doc["mqtt_hostname"]
	mqtt_port = str(doc["mqtt_port"])
	
	while (True):
		json_message = lrd.receive_encrypted()
		#print("received: "+json_message)
		if (json_message == "{NULL}"):
			#decryption failed, skip message
			continue
		json_object = json.loads(json_message)
		payload = json_object["p"]
		nofproc, rest_string = payload.split(", ", 1)
		nofproc = nofproc.split("=")[1] # get number of images processed
		print(nofproc + " Image processings, string: \""+rest_string+"\"")
		# process payload with object detections
		while (True):
			detection, rest_string = rest_string.split(", ", 1)
			size, object_name = detection.split(" ")
			print("Object \""+object_name+"\" "+ "{:.2f}".format(float(size) / float(nofproc)))
			if (rest_string.find(", ") == -1):
				size, object_name = rest_string.split(" ")
				print("Object \""+object_name+"\" "+ "{:.2f}".format(float(size) / float(nofproc)))
				break
		
		json_message = lrd.lora_str_translate(json_message)
		publish.single("lrdlink", json_message, hostname=mqtt_hostname, auth={'username':username,'password':password})

if __name__ == "__main__":
	main()
