#!/usr/bin/python3

import time
from bmp280 import BMP280
import os
import lrd

try:
        from smbus2 import SMBus
except ImportError:
        from smbus import SMBus

bus = SMBus(2)
bmp280 = BMP280(i2c_dev=bus)

adc_file = open("/sys/class/cvi-saradc/cvi-saradc0/device/cv_saradc", "r")

while True:
	temp = bmp280.get_temperature()
	pressure = bmp280.get_pressure()
	voltage = adc_file.read()
	adc_file.seek(0)
	#print('Temp={:0.2f} C '.format(temp)+'Pressure={:0.2f} hPa'.format(pressure))
	final_data = "{:.2f}".format(temp)+" "+"{:.2f}".format(pressure)+" "+"{:.3f}".format(float(voltage)/1000)
	print(final_data)
	lrd.transmit_encrypted(1, final_data)
	time.sleep(30)
