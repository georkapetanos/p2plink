#!/usr/bin/python3
from picamera2 import Picamera2
from functools import partial
import sys, time, libcamera, signal, torch, re, threading
import lrd

CAPTURED_IMG_PATH = "/media/ramdisk/picam0.jpg"
CAPTURED_IMG_PATH_ALT = "/media/ramdisk/picam1.jpg"
TRANSMIT_INTERVAL = 30

def camera_setup():
	picam = Picamera2()
	config = picam.create_preview_configuration(main={"size": (3840, 1920)})
	config["transform"] = libcamera.Transform(hflip=1, vflip=1)
	picam.configure(config)
	picam.start()
	return picam

def model_setup():
	model = torch.hub.load('ultralytics/yolov5', 'yolov5n')
	return model

def capture_image(picam, path):
	picam.capture_file(path)

def process_image(model, path):
	results = model(path)
	output = str(results.print)
	output = output.split("\n",5)[1]
	output = output.split(" ", 3)[3]
	print(output)
	return output

def sigint_handler(sig, frame, picam):
	print('\nQuitting...')
	picam.close()
	sys.exit(0)

def main():
	global picam, output, model, handler, detections
	select_image = True #This toggles image for multithreading, False -> save to ALT_image
	picam = camera_setup()
	#capture first image
	capture_image(picam, CAPTURED_IMG_PATH_ALT)
	handler = partial(sigint_handler, picam=picam)
	signal.signal(signal.SIGINT, handler)
	model = model_setup()
	detections = ""

	start = time.time()
	processings = 0

	while(True):
		#dt_cam = time.time()
		if select_image:
			cam_thread = threading.Thread(target=capture_image, args=(picam, CAPTURED_IMG_PATH,))
		else:
			cam_thread = threading.Thread(target=capture_image, args=(picam, CAPTURED_IMG_PATH_ALT,))
		cam_thread.start()
		#dt_cam = time.time() - dt_cam
		#print(output)
		dt_detect = time.time()
		if select_image:
			output = process_image(model, CAPTURED_IMG_PATH_ALT)
		else:
			output = process_image(model, CAPTURED_IMG_PATH)
		select_image = not select_image
			
		detections = lrd.process_detections_str(detections, output)
		dt_detect = time.time() - dt_detect
		processings = processings + 1
		#print(detections)
		print("dt_detect = "+"{:.2f}".format(dt_detect))
		print("dt = " + "{:.2f}".format(time.time() - start) + ", p = " + str(processings) + " || " + detections)
		if (time.time() - start > TRANSMIT_INTERVAL):
			#transmit detections
			detections = re.sub(r"\s+", " ", detections)	#replace multiple consecutive white characters
			detections = "p=" + str(processings) + "," + detections
			print("{:.2f}".format(processings / (time.time() - start)) + " Frames / second")
			lrd.transmit_encrypted(1, detections)
			detections = ""
			processings = 0
			start = time.time()

if __name__ == "__main__":
	main()
