#!/bin/bash

#M0	-> GP15
#M1 	-> GP14
#AUX	-> GP17
duo-pinmux -w GP14/GP14
duo-pinmux -w GP16/GP16
duo-pinmux -w GP17/GP17
echo 494 > /sys/class/gpio/export
echo 495 > /sys/class/gpio/export
echo 497 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio495/direction
echo out > /sys/class/gpio/gpio494/direction
echo in > /sys/class/gpio/gpio497/direction
echo 0 > /sys/class/gpio/gpio494/value
echo 0 > /sys/class/gpio/gpio495/value
cat /sys/class/gpio/gpio494/value
cat /sys/class/gpio/gpio495/value
cat /sys/class/gpio/gpio497/value
