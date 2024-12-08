#!/usr/bin/python3

import time
from bmp280 import BMP280
import os
import lrd
import SDL_Pi_HDC1000

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
	lrd.transmit_encrypted(1, final_data)
	time.sleep(30)
