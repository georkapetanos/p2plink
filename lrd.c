#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "lrd_shared.h"
#include "json.h"
#include "mqtt.h"
#define MAX_MSG_BUFFER 4096
#define USAGE_MSG "Usage: lrd [<arguments>]\n\n-h\t\tUsage message\n-c\t\tTransmit command\n-d <data>\tTransmit JSON data\n-r\t\tReceive JSON data\n-s </dev/ttyX>\tSet Serial device\n"
#define MAX_STRING_SIZE 8192

/* Transmit procedure
* Parse input from user,
* insert uuid and timestamp in JSON,
* transmit JSON to serial
* TODO checksum
*
*/

void construct_json_command(char *command, char *uuid, char **json_output) {
	json_rootT *root;
	
	root = json_init();
	json_append_object(root, "type", "1");
	json_append_object(root, "exec", command);
	json_append_object(root, "uuid", uuid);
	json_append_object(root, "ts", "16:17:31");
	//*json_output = json_to_string(root, false);
}

int main(int argc, char *argv[]) {
	config_dataT config;
	int i, j;
	char *json_string = NULL, command[256], serial_port[64];
	unsigned char *rx_buf;
	int rx_size;
		
	read_configuration_file(&config);
	strcpy(serial_port, config.serial_device);
	
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	
	if(argc < 2) {
		printf("Usage: ./lrd -options\n");
		return 0;
	}
	for(i = 1; i < argc; i++) {
		if((strncmp(argv[i], "-c", 2) == 0) && (argc - 1 > i)) {
			if(argv[i + 1][0] != '-') {
				printf("command: %s\n", argv[i+1]);
				construct_json_command(argv[i+1], config.uuid, &json_string);
				printf("json:\n%s\n", json_string);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if((strncmp(argv[i], "-d", 2) == 0) && (argc - 1 > i)) {
			if(argv[i + 1][0] != '-') {
				strcpy(command, "stty -F ");
				strcat(command, serial_port);
				strcat(command, " -echo inlcr");
				system(command);
				construct_json_data(argv[i+1], config.uuid, &json_string);
				printf("json: %s\n", json_string);
				serial_transmit((unsigned char *) json_string, strlen(json_string), serial_port);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if(strncmp(argv[i], "-r", 2) == 0) {
			mqtt_setup();
			
			strcpy(command, "stty -F ");
			strcat(command, serial_port);
			strcat(command, " -echo inlcr");
			system(command);
		
			rx_buf = malloc(MAX_MSG_BUFFER*sizeof(unsigned char));
			while(1) {
				serial_receive(rx_buf, &rx_size, serial_port);
				publish_message((char *) rx_buf, rx_size);
				printf("rx_buf: ");
				for(j = 0; j < rx_size; j++) {
					printf("%c", rx_buf[j]);
				}
				printf("\n");
			}
			free(rx_buf);
			i++;
		} else if((strncmp(argv[i], "-s", 2) == 0) && (argc - 1 > i)) {
			strcpy(serial_port, argv[i+1]);
			i++;
		} else if((strncmp(argv[i], "-y", 2) == 0) && (argc - 1 > i)) {
			printf("input: ");
			for(j = i + 4; j < argc; j++) {
				if(argv[j] == NULL) { //after null follows some system information
					break;
				}
				printf("%s ", argv[j]);
			}
			printf("\n");
			//i++;
			mqtt_cleanup();
			break;
		}
		else if(strncmp(argv[i], "-h", 2) == 0) {
			printf(USAGE_MSG);
		}
		else {
			printf("lrd: invalid option -- '%c%c'\n", argv[i][0], argv[i][1]);
			printf(USAGE_MSG);
		}
	}
	
	free(json_string);
	
	return 0;
}
