#!/usr/bin/env python3
import json, datetime, yaml
import lrd
from influxdb import InfluxDBClient

# Logs the data to your InfluxDB
#def format_json(measurement, station, timestamp, temperature, pressure, humidity):
def format_json(measurement, station, timestamp, temperature, pressure, voltage):
	payload = [
		{"measurement": measurement,
		"tags": {
			"station": station,
		},
		"time": timestamp,
		"fields": {
			"temperature" : temperature,
			"humidity": -1,
			"pressure": pressure,
			"voltage": voltage
		}
	}
	]

	return payload

def main():

	db = 'mydb'

	# open config file located in the same directory
	config = open('config.yaml', 'r')
	doc = yaml.load(config, Loader=yaml.SafeLoader)
	username = doc["mqtt_username"]
	password = str(doc["mqtt_password"])
	host = doc["mqtt_hostname"]
	port = str(doc["mqtt_port"])

	client = InfluxDBClient(host, port, username, password, db)
	measurement = "indoor"

	while (True):
		json_message = lrd.receive_encrypted()
		if (json_message == "{NULL}"):
			#decryption failed, skip message
			continue
		json_object = json.loads(json_message)
		station = json_object["u"]
		payload = json_object["p"]
		temp = payload.split(' ')
		if len(temp) < 2:
			print("Invalid payload")
		else:
			print(station+": "+temp[0] + " and "+ temp[1]+" and "+temp[2])
			timestamp = datetime.datetime.utcnow()
			json_payload = format_json(measurement, station, timestamp, float(temp[0]), float(temp[1]), float(temp[2]))
			client.write_points(json_payload)

if __name__ == "__main__":
	main()
