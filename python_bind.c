#include <stdio.h>
#include <string.h>
#include <Python.h>
#include "lrd_shared.h"
#include "json.h"

static PyObject *method_transmit(PyObject *self, PyObject *args) {
	char *str, *json_string = NULL, *serial_port;
	int count, serial;
	config_dataT config;
	char *checksum = (char *) malloc(3 * sizeof(unsigned char));

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

static PyMethodDef LrdMethods[] = {
	{"transmit", method_transmit, METH_VARARGS, "Python interface for the p2plink library function"},
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
