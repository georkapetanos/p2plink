#!/usr/bin/env python3
import json, datetime
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

	host = '192.168.2.101'
	port = 8086
	username = 'grafana'
	password = 'grafana'
	db = 'mydb'

	client = InfluxDBClient(host, port, username, password, db)
	measurement = "indoor"
	station = "milkv"

	while (True):
		json_message = lrd.receive_encrypted()
		#print("received: "+json_message)
		if (json_message == "{NULL}"):
			#decryption failed, skip message
			continue
		json_object = json.loads(json_message)
		#payload = json_object["p"]
		payload = json_object["p"]
		temp = payload.split(' ')
		print(temp[0] + " and "+ temp[1]+" and "+temp[2])
		
		timestamp = datetime.datetime.utcnow()
		json_payload = format_json(measurement, station, timestamp, float(temp[0]), float(temp[1]), float(temp[2]))
		client.write_points(json_payload)
		
		
if __name__ == "__main__":
	main()
