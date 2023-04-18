#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lrd_shared.h"
#define MAX_STRING_SIZE 8192

int main(int argc, char *argv[]) {
	config_dataT config;
	char str[256] = "boat,2,cat,1";
	char *json_string = NULL;
	int serial;

	printf("Argument passed is: %s\n", str);
	read_configuration_file(&config);
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	construct_json_str(str, config.uuid, &json_string);
	serial_init("/dev/ttyUSB0", &serial);
	serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
	serial_close(&serial);
	
	free(json_string);

	return 0;
}
