#!/usr/bin/python3
from picamera2 import Picamera2
from functools import partial
import sys, time, libcamera, signal, torch, re
import lrd

CAPTURED_IMG_PATH = "/media/ramdisk/picam.jpg"
TRANSMIT_INTERVAL = 40

def camera_setup():
	picam = Picamera2()
	config = picam.create_preview_configuration(main={"size": (3840, 2160)})
	config["transform"] = libcamera.Transform(hflip=1, vflip=1)
	picam.configure(config)
	picam.start()
	return picam

def model_setup():
	model = torch.hub.load('ultralytics/yolov5', 'yolov5n')
	return model

def capture_and_process(picam, model):
	picam.capture_file(CAPTURED_IMG_PATH)
	results = model(CAPTURED_IMG_PATH)
	output = str(results.print)
	#print(output)
	output = output.split("\n",5)[1]
	output = output.split(" ", 3)[3]
	print(output)
	#results.save()
	return output

def sigint_handler(sig, frame, picam):
	print('\nQuitting...')
	picam.close()
	sys.exit(0)

def main():
	global picam, output, model, handler, detections
	picam = camera_setup()
	handler = partial(sigint_handler, picam=picam)
	signal.signal(signal.SIGINT, handler)
	model = model_setup()
	detections = ""

	start = time.time()

	while(True):
		output = capture_and_process(picam, model)
		#print(output)
		detections = lrd.process_detections_str(detections, output)
		#print(detections)
		print(detections + " " + str(time.time() - start))
		if (time.time() - start > TRANSMIT_INTERVAL):
			#transmit detections
			detections = re.sub(r"\s+", " ", detections)
			lrd.transmit_encrypted(1, detections)
			detections = ""
			start = time.time()

if __name__ == "__main__":
	main()
