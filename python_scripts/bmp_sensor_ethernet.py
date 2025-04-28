#!/usr/bin/python3

import time, json, yaml, datetime
from bmp280 import BMP280
from influxdb import InfluxDBClient
import os
import lrd
import SDL_Pi_HDC1000

# Logs the data to your InfluxDB
#def format_json(measurement, station, timestamp, temperature, pressure, humidity):
def format_json(measurement, station, timestamp, temperature, pressure, voltage, temperature2, humidity):
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
                        "voltage": voltage,
                        "hdc_temperature" : temperature2,
                        "hdc_humidity" : humidity
                }
        }
        ]

        return payload

def main():
	try:
        	from smbus2 import SMBus
	except ImportError:
        	from smbus import SMBus

	bus = SMBus(2)
	bmp280 = BMP280(i2c_dev=bus)

	adc_file = open("/sys/class/cvi-saradc/cvi-saradc0/device/cv_saradc", "r")

	hdc1000 = SDL_Pi_HDC1000.SDL_Pi_HDC1000()
	hdc1000.setTemperatureResolution(SDL_Pi_HDC1000.HDC1000_CONFIG_TEMPERATURE_RESOLUTION_14BIT)
	hdc1000.setHumidityResolution(SDL_Pi_HDC1000.HDC1000_CONFIG_HUMIDITY_RESOLUTION_14BIT)

	db = 'mydb'

 	# open config file located in the same directory
	config = open('config.yaml', 'r')
	doc = yaml.load(config, Loader=yaml.SafeLoader)
	username = doc["mqtt_username"]
	password = str(doc["mqtt_password"])
	host = doc["mqtt_hostname"]
	port = str(doc["mqtt_port"])
	uuid = str(doc["uuid"])

	client = InfluxDBClient(host, port, username, password, db)
	measurement = "indoor"

	# sleep waiting for network to come up
	time.sleep(30)

	while True:
		bmp_temp = bmp280.get_temperature()
		bmp_pressure = bmp280.get_pressure()
		voltage = adc_file.read()
		adc_file.seek(0)
		hdc_temp = hdc1000.readTemperature()
		hdc_humidity = hdc1000.readHumidity()

		#print('Temp={:0.2f} C '.format(bmp_temp)+'Pressure={:0.2f} hPa'.format(bmp_pressure))
		final_data = "{:.2f}".format(bmp_temp)+" "+"{:.2f}".format(bmp_pressure)+" "+"{:.3f}".format(float(voltage)/1000)+" "+"{:.2f}".format(hdc_temp)+" "+"{:.2f}".format(hdc_humidity)
		print(final_data)

		#lrd.transmit_encrypted(1, final_data)

		timestamp = datetime.datetime.utcnow()
		json_payload = format_json(measurement, uuid, timestamp, bmp_temp, bmp_pressure, float(voltage)/1000, hdc_temp, hdc_humidity)

		# in case the server/network has been down
		try:
			client.write_points(json_payload)
		except OSError as e:
			print(e)

		time.sleep(30)

if __name__ == "__main__":
        main()
