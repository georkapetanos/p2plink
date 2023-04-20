#!/bin/bash
# Find what keeps GPIO pins high/low
#sudo cat /sys/kernel/debug/gpio

echo 54 >/sys/class/gpio/export
echo 51 >/sys/class/gpio/export
echo 50 >/sys/class/gpio/export
echo out >/sys/class/gpio/gpio54/direction
echo out >/sys/class/gpio/gpio51/direction
echo in >/sys/class/gpio/gpio50/direction
echo 0 > /sys/class/gpio/gpio54/value
echo 0 > /sys/class/gpio/gpio51/value
