#!/usr/bin/python3

import time
import lrd

TRANSMIT_INTERVAL = 30

def main():
	beacon_id = 0
	start = time.time()
	while (True):
		if (time.time() - start > TRANSMIT_INTERVAL):
			print("Transmitting beacon " + str(beacon_id) +" over Lora...")
			lrd.transmit_encrypted(1, "beac " + str(beacon_id))
			beacon_id = beacon_id + 1;
			start = time.time()
		time.sleep(1)

if __name__ == "__main__":
	main()
