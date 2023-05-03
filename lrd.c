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
#define USAGE_MSG "Usage: lrd [<arguments>]\n\n-h\t\tUsage message\n-c\t\tTransmit command\n-s <string>\tTransmit plain string\n-r\t\tReceive JSON data\n-t </dev/ttyX>\tSet Serial device\n-e\t\tEnable encryption\n"

/* Transmit procedure
* Parse input from user,
* insert uuid and timestamp in JSON,
* insert checksum,
* encrypt,
* transmit data to serial
* TODO: command relay receiver implementation
*/

int main(int argc, char *argv[]) {
	config_dataT config;
	int i, j, rx_size, serial, encrypted_text_len = 0, decrypted_text_len = 0;
	char *json_string = NULL, serial_port[64], *checksum = NULL, *ack_string = NULL, *strstrreturn = NULL, *mqtt_message_buf, *uuid_buf;
	unsigned char encrypted_text[MAX_MSG_BUFFER], decrypted_text[MAX_MSG_BUFFER], *rx_buf;
	bool use_encryption = false;

	read_configuration_file(&config);
	strcpy(serial_port, config.serial_device);
	
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	ack_string = (char *) malloc(32 * sizeof(char));
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
		} else if((strncmp(argv[i], "-s", 2) == 0) && (argc - 1 > i)) {
			if(argv[i + 1][0] != '-') {				
				serial_init(serial_port, &serial);
				construct_json_str(argv[i+1], config.uuid, &json_string);
				checksum_generate((unsigned char *) json_string, strlen(json_string), checksum);
				embed_checksum((unsigned char *) json_string, strlen(json_string), checksum);
				printf("json: %s\n", json_string);
				if(use_encryption) {
					encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, config.encryption_mode, (unsigned char *) json_string, encrypted_text, &encrypted_text_len);
					hex_print(encrypted_text, encrypted_text_len);
					//printf("encrypted_text_len = %d\n", encrypted_text_len);
					serial_tx(serial, encrypted_text, encrypted_text_len);
					if(config.acknowledge_packets) {
						//wait approximately 3 seconds for response
						for(j = 0; j < 3; j++) {
							serial_rx_imdreturn(serial, (unsigned char *) ack_string, &rx_size);
							if(rx_size !=0) { //checksum received;
								decrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, config.encryption_mode, decrypted_text, (unsigned char *) ack_string, rx_size, &decrypted_text_len);
								memcpy(ack_string, decrypted_text, decrypted_text_len);
								rx_size = decrypted_text_len - 1; //minus one because of null termination character
								strstrreturn = strstr(ack_string, "\"x\"");
								if((strstrreturn != NULL) && (rx_size > 17)) {
									if(strncmp(strstrreturn + 5, checksum, 2) == 0) {
										printf("ACK received and OK: %s\n", ack_string);
									}
								}
								break;
							}
						}
					}
				} else {
					serial_tx(serial, (unsigned char *) json_string, strlen(json_string));
					if(config.acknowledge_packets) {
						//wait approximately 3 seconds for response
						for(j = 0; j < 3; j++) {
							serial_rx_imdreturn(serial, (unsigned char *) ack_string, &rx_size);
							if(rx_size !=0) { //checksum received;
								strstrreturn = strstr(ack_string, "\"x\"");
								if((strstrreturn != NULL) && (rx_size > 17)) {
									if(strncmp(strstrreturn + 5, checksum, 2) == 0) {
										printf("ACK received and OK: %s\n", ack_string);
									}
								}
								break;
							}
						}
					}
				}
				
				serial_close(&serial);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if(strncmp(argv[i], "-r", 2) == 0) {
			serial_init(serial_port, &serial);
			rx_buf = malloc(MAX_MSG_BUFFER*sizeof(unsigned char));
			
			if(config.enforce_uuid_whitelist == true) {
				read_uuid_whitelist_file(&config);
				print_uuid_whitelist(&config);
			}
			
			if(config.broadcast_to_mqtt) {
				mqtt_setup();
				mqtt_message_buf = malloc(MAX_MSG_BUFFER*sizeof(char));
			}
			
			while(1) {
				if(config.broadcast_to_mqtt) {
					memset(mqtt_message_buf, 0, MAX_MSG_BUFFER);
				}
				serial_rx(serial, rx_buf, &rx_size);
				if(use_encryption) {
					decrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, config.encryption_mode, decrypted_text, rx_buf, rx_size, &decrypted_text_len);
					memcpy(rx_buf, decrypted_text, decrypted_text_len);
					rx_size = decrypted_text_len - 1; //minus one because of null termination character
				}
				if(checksum_integrity_check(rx_buf, rx_size)) {
					//checksum doesn't match, drop packet
					printf("Dropping 1 packet, wrong checksum\n");
					//send not-acknowledgement
					get_checksum(rx_buf, rx_size, checksum);
					format_ack(serial, checksum, &ack_string, false);
					if(use_encryption) {
						encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, config.encryption_mode, (unsigned char *) ack_string, encrypted_text, &encrypted_text_len);
						serial_tx(serial, encrypted_text, encrypted_text_len);
					} else {
						serial_tx(serial, (unsigned char *) ack_string, strlen(ack_string));
					}
					//printf("ack: %s\n", ack_string);
				} else {
					if(config.acknowledge_packets) {
						//send acknowledgement
						get_checksum(rx_buf, rx_size, checksum);
						format_ack(serial, checksum, &ack_string, true);
						if(use_encryption) {
							encrypt((unsigned char *) config.encryption_key, (unsigned char *) config.encryption_iv, config.encryption_mode, (unsigned char *) ack_string, encrypted_text, &encrypted_text_len);
							serial_tx(serial, encrypted_text, encrypted_text_len);
						} else {
							serial_tx(serial, (unsigned char *) ack_string, strlen(ack_string));
						}
						//printf("ack: %s, size=%ld\n", ack_string, strlen(ack_string));
					}
					
					remove_checksum(rx_buf, rx_size);
					rx_size = rx_size - 2;
					rx_buf[rx_size] = '\0';
					if(config.broadcast_to_mqtt) {
						if(config.enforce_uuid_whitelist) {
							uuid_buf = (char *) calloc(1, 37 * sizeof(char *));
							get_msg_uuid((char *) rx_buf, uuid_buf);
							if(uuid_whitelist_query(&config, uuid_buf) == 0) {
								printf("Message UUID: \"%s\" not in whitelist, not forwarding to MQTT.\n", uuid_buf);
								free(uuid_buf);
								continue;
							}
							free(uuid_buf);
						}
						lora_str_to_mqtt_translate((char *) rx_buf, mqtt_message_buf);
						//printf("mqtt_buf: %s\n", mqtt_message_buf);
						publish_message(mqtt_message_buf, strlen(mqtt_message_buf));
						
					}
				}
			}
			free_uuid_whitelist(&config);
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
