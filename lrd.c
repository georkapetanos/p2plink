#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "lrd_shared.h"
#include "json.h"
#include "mqtt.h"
#include "crypto.h"
#define MAX_MSG_BUFFER 4096
#define USAGE_MSG "Usage: lrd [<arguments>]\n\n-h\t\tUsage message\n-c\t\tTransmit command\n-d <data>\tTransmit custom object array\n-s <string>\tTransmit plain string\n-r\t\tReceive JSON data\n-t </dev/ttyX>\tSet Serial device\n-e\t\tEnable encryption\n"

/* Transmit procedure
* Parse input from user,
* insert uuid and timestamp in JSON,
* transmit JSON to serial
* TODO checksum: extra hex outsise JSON at the end of transmission, will be clipped by receive function
* command relay receiver implementation
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
	char *json_string = NULL, command[256], serial_port[64], *checksum = NULL;
	unsigned char encrypted_text[4096], decrypted_text[4096];
	int encrypted_text_len = 0, decrypted_text_len = 0;
	unsigned char *rx_buf;
	int rx_size;
	bool use_encryption = false;
	int serial;
		
	read_configuration_file(&config);
	strcpy(serial_port, config.serial_device);
	
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	checksum = (char *) malloc(3 * sizeof(unsigned char));
	
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
				strcat(command, " -echo");
				system(command);
				construct_json_data(argv[i+1], config.uuid, &json_string);
				checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
				embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
				printf("json: %s\n", json_string);
				serial_transmit((unsigned char *) json_string, strlen(json_string), serial_port);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if((strncmp(argv[i], "-s", 2) == 0) && (argc - 1 > i)) {
			if(argv[i + 1][0] != '-') {
				/*strcpy(command, "stty -F ");
				strcat(command, serial_port);
				strcat(command, " -echo");
				system(command);*/
				
				serial_init(serial_port, &serial);
				
				construct_json_str(argv[i+1], config.uuid, &json_string);
				checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
				//printf("calculated checksum %s.\n", checksum);
				embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
				printf("json: %s\n", json_string);
				//printf("checksum = %s\n", checksum);
				if(use_encryption) {
					encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, (unsigned char *) json_string, encrypted_text, &encrypted_text_len);
					hex_print(encrypted_text, encrypted_text_len);
					//serial_transmit(encrypted_text, encrypted_text_len, serial_port);
					printf("encrypted_text_len = %d\n", encrypted_text_len);
					serial_tx(serial, encrypted_text, encrypted_text_len);
				} else {
					//serial_transmit((unsigned char *) json_string, strlen(json_string), serial_port);
					serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
				}
				
				serial_close(&serial);
				
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if(strncmp(argv[i], "-r", 2) == 0) {
			mqtt_setup();
			
			serial_init(serial_port, &serial);
			
			/*strcpy(command, "stty -F ");
			strcat(command, serial_port);
			strcat(command, " -echo");
			system(command);*/
		
			rx_buf = malloc(MAX_MSG_BUFFER*sizeof(unsigned char));
			while(1) {
				//serial_receive(rx_buf, &rx_size, serial_port);
				serial_rx(serial, rx_buf, &rx_size);
				if(use_encryption) {
					decrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, decrypted_text, rx_buf, rx_size, &decrypted_text_len);
					printf("decrypted size = %d, rx_size = %d\n", decrypted_text_len, rx_size);
					printf("decrypted data: %s\n", decrypted_text);
					memcpy(rx_buf, decrypted_text, decrypted_text_len);
					rx_size = decrypted_text_len;
				}
				//rx_size = preprocess_received_data(rx_buf, rx_size);
				if(checksum_integrity_check(rx_buf, rx_size)) {
					//checksum doesn't match, drop packet
					printf("Dropping 1 packet, wrong checksum\n");
				} else {
					remove_checksum(rx_buf, rx_size);
					rx_size = rx_size - 2;
					publish_message((char *) rx_buf, rx_size);
				}
			}
			free(rx_buf);
				
			serial_close(&serial);
			
			i++;
		} else if((strncmp(argv[i], "-t", 2) == 0) && (argc - 1 > i)) {
			strcpy(serial_port, argv[i+1]);
			i++;
		} else if(strncmp(argv[i], "-e", 2) == 0) {
			use_encryption = true;
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
