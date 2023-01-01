#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lrd_shared.h"
#define MAX_STRING_SIZE 8192

int main(int argc, char *argv[]) {
	configurationT config;
	char str[256] = "boat,2,cat,1";
	char *json_string = NULL;

	printf("Argument passed is: %s\n", str);
	read_configuration_file(&config);
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	construct_json_data(str, config.uuid, &json_string);
	serial_transmit((unsigned char *) json_string, strlen(json_string), "/dev/ttyUSB0");
	
	free(json_string);

	return 0;
}
