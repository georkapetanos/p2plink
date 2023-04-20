#include <stdio.h>
#include <string.h>
#include <Python.h>
#include "lrd_shared.h"
#include "json.h"
#include "crypto.h"

static PyObject *method_transmit(PyObject *self, PyObject *args) {
	char *str, *json_string = NULL;
	int count, serial;
	config_dataT config;
	char *checksum = (char *) malloc(3 * sizeof(char));

	if(!PyArg_ParseTuple(args, "iss", &count, &str)) {
		return NULL;
	}
	printf("Argument passed is: %s\n", str);
	read_configuration_file(&config);
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	construct_json_str(str, config.uuid, &json_string);
	checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
	embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
	serial_init(config.serial_device, &serial);
	serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
	serial_close(&serial);
	
	free(json_string);
	return PyLong_FromLong(0);
}

static PyObject *method_transmit_encrypted(PyObject *self, PyObject *args) {
	char *str, *json_string = NULL;
	int count, serial, encrypted_text_len = 0;
	config_dataT config;
	char *checksum = (char *) malloc(3 * sizeof(char));
	unsigned char encrypted_text[MAX_MSG_BUFFER];

	if(!PyArg_ParseTuple(args, "iss", &count, &str)) {
		return NULL;
	}
	printf("Argument passed is: %s\n", str);
	read_configuration_file(&config);
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	construct_json_str(str, config.uuid, &json_string);
	checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
	embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
	//printf("%s %s .%s.\n", config.encryption_key, config.encryption_iv, json_string);
	encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, (unsigned char *) json_string, encrypted_text, &encrypted_text_len);
	hex_print(encrypted_text, encrypted_text_len);
	printf("encrypted_text_len = %d\n", encrypted_text_len);
	serial_init(config.serial_device, &serial);
	serial_tx(serial, encrypted_text, encrypted_text_len);
	serial_close(&serial);
	
	free(json_string);
	return PyLong_FromLong(0);
}

static PyObject *method_process_detections_str(PyObject *self, PyObject *args) {
	char *token, *old_str, *new_str, returned_str[512], value[4], new_value[4], *occur;
	int iterations, old_iterations;

	if(!PyArg_ParseTuple(args, "ss", &old_str, &new_str)) {
		return NULL;
	}
	
	strcpy(returned_str, old_str);
	
	token = strtok(new_str, ",");
	if(!strncmp(token, "(no detections)", 15)) {
		//no detections do nothing
		return PyUnicode_FromString(old_str);
	}
	while (token != NULL) {
		if(!strncmp(token, " ", 1)) {
			//workaround first character space
			token++;
		}
		iterations = atoi(token);
		//object string is at strstr(token, " ") + 1
		//printf("iter: %d, string: %s.\n", iterations, strstr(token, " ") + 1);
		occur = strstr(returned_str, strstr(token, " ") + 1);
		if(occur != NULL) {
			old_iterations = atoi(occur - 4);
			//printf("old_iterations = %d\n", old_iterations);
			sprintf(new_value, "%3d", old_iterations + iterations);
			*(occur - 4) = new_value[0];
			*(occur - 3) = new_value[1];
			*(occur - 2) = new_value[2];
		}
		else {
			if(strlen(returned_str) != 0) {
				strcat(returned_str, ", ");
			}
			sprintf(value, "%3d", iterations);
			strcat(returned_str, value);
			strcat(returned_str, " ");
			strcat(returned_str, strstr(token, " ") + 1);
		}
		token = strtok(NULL, ",");
	}
	return PyUnicode_FromString(returned_str);
}

static PyMethodDef LrdMethods[] = {
	{"transmit", method_transmit, METH_VARARGS, "Transmit string message"},
	{"transmit_encrypted", method_transmit_encrypted, METH_VARARGS, "Encrypt and transmit string message"},
	{"process_detections_str", method_process_detections_str, METH_VARARGS, "Process detections str"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef lrdmodule = {
	PyModuleDef_HEAD_INIT,
	"lrd",
	"Python interface for the p2plink library function",
	-1,
	LrdMethods
};

PyMODINIT_FUNC PyInit_lrd(void) {
	return PyModule_Create(&lrdmodule);
}
