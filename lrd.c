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
* insert checksum,
* encrypt,
* transmit data to serial
* TODO: command relay receiver implementation
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
	int i, j, rx_size, serial, encrypted_text_len = 0, decrypted_text_len = 0;
	char *json_string = NULL, serial_port[64], *checksum = NULL, *ack_string = NULL, *strstrreturn = NULL;
	unsigned char encrypted_text[MAX_MSG_BUFFER], decrypted_text[MAX_MSG_BUFFER], *rx_buf;
	bool use_encryption = false;
		
	read_configuration_file(&config);
	strcpy(serial_port, config.serial_device);
	
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	ack_string = (char *) malloc(48 * sizeof(char));
	checksum = (char *) malloc(3 * sizeof(char));
	
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
				serial_init(serial_port, &serial);
				construct_json_data(argv[i+1], config.uuid, &json_string);
				checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
				embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
				printf("json: %s\n", json_string);
				if(use_encryption) {
					encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, (unsigned char *) json_string, encrypted_text, &encrypted_text_len);
					hex_print(encrypted_text, encrypted_text_len);
					printf("encrypted_text_len = %d\n", encrypted_text_len);
					serial_tx(serial, encrypted_text, encrypted_text_len);
				} else {
					serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
				}
				
				serial_close(&serial);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if((strncmp(argv[i], "-s", 2) == 0) && (argc - 1 > i)) {
			if(argv[i + 1][0] != '-') {				
				serial_init(serial_port, &serial);
				construct_json_str(argv[i+1], config.uuid, &json_string);
				checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
				embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
				printf("json: %s\n", json_string);
				if(use_encryption) {
					encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, (unsigned char *) json_string, encrypted_text, &encrypted_text_len);
					hex_print(encrypted_text, encrypted_text_len);
					//printf("encrypted_text_len = %d\n", encrypted_text_len);
					serial_tx(serial, encrypted_text, encrypted_text_len);
				} else {
					serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
					//wait approximately 3 seconds for response
					for(j = 0; j < 3; j++) {
						serial_rx_imdreturn(serial, (unsigned char *) ack_string, &rx_size);
						if(rx_size !=0) { //checksum received;
							strstrreturn = strstr(ack_string, "chksum");
							if((strstrreturn != NULL) && (rx_size > 24)) {
								if(strncmp(strstrreturn + 9, checksum, 2) == 0) {
									printf("ACK received and OK: %s\n", ack_string);
								}
							}
							break;
						}
					}
				}
				
				serial_close(&serial);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if(strncmp(argv[i], "-r", 2) == 0) {
			mqtt_setup();
			serial_init(serial_port, &serial);
			rx_buf = malloc(MAX_MSG_BUFFER*sizeof(unsigned char));
			while(1) {
				serial_rx(serial, rx_buf, &rx_size);
				if(use_encryption) {
					decrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, decrypted_text, rx_buf, rx_size, &decrypted_text_len);
					memcpy(rx_buf, decrypted_text, decrypted_text_len);
					rx_size = decrypted_text_len - 1; //minus one because of null termination character
				}
				if(checksum_integrity_check(rx_buf, rx_size)) {
					//checksum doesn't match, drop packet
					printf("Dropping 1 packet, wrong checksum\n");
					//send not-acknowledgement
					get_checksum(rx_buf, rx_size, checksum);
					format_ack(serial, checksum, &ack_string, false);
					serial_tx(serial, (unsigned char *) ack_string, strlen(ack_string));
					printf("ack: %s\n", ack_string);
				} else {
					//send acknowledgement
					get_checksum(rx_buf, rx_size, checksum);
					format_ack(serial, checksum, &ack_string, true);
					serial_tx(serial, (unsigned char *) ack_string, strlen(ack_string));
					//printf("ack: %s, size=%ld\n", ack_string, strlen(ack_string));
					
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
		} else if(strncmp(argv[i], "-h", 2) == 0) {
			printf(USAGE_MSG);
		}
		else {
			printf("lrd: invalid option -- '%c%c'\n", argv[i][0], argv[i][1]);
			printf(USAGE_MSG);
		}
	}
	
	free(json_string);
	free(ack_string);
	free(checksum);
	
	return 0;
}
