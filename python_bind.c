#include <stdio.h>
#include <string.h>
#include <Python.h>
#include "lrd_shared.h"
#include "json.h"
#include "crypto.h"

static PyObject *method_transmit(PyObject *self, PyObject *args) {
	char *str, *json_string = NULL, *serial_port;
	int count, serial;
	config_dataT config;
	char *checksum = (char *) malloc(3 * sizeof(char));

	if(!PyArg_ParseTuple(args, "iss", &count, &str, &serial_port)) {
		return NULL;
	}
	printf("Argument passed is: %s\n", str);
	read_configuration_file(&config);
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	construct_json_str(str, config.uuid, &json_string);
	checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
	embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
	serial_init(serial_port, &serial);
	serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
	serial_close(&serial);
	
	free(json_string);
	return PyLong_FromLong(0);
}

static PyObject *method_transmit_encrypted(PyObject *self, PyObject *args) {
	char *str, *json_string = NULL, *serial_port;
	int count, serial, encrypted_text_len = 0;
	config_dataT config;
	char *checksum = (char *) malloc(3 * sizeof(char));
	unsigned char encrypted_text[MAX_MSG_BUFFER];

	if(!PyArg_ParseTuple(args, "iss", &count, &str, &serial_port)) {
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
	serial_init(serial_port, &serial);
	serial_tx(serial, encrypted_text, encrypted_text_len);
	serial_close(&serial);
	
	free(json_string);
	return PyLong_FromLong(0);
}

static PyMethodDef LrdMethods[] = {
	{"transmit", method_transmit, METH_VARARGS, "Transmit string message"},
	{"transmit_encrypted", method_transmit_encrypted, METH_VARARGS, "Encrypt and transmit string message"},
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
