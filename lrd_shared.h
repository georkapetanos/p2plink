#ifndef LRD_SHARED_H
#define LRD_SHARED_H

#include <stdbool.h>
#define MAX_MSG_BUFFER 4096

typedef struct uuid_whitelist{
	char uuid[37];
} uuid_whitelistT;

typedef struct config_data{		// Default values
	char hostname[256];
	unsigned short port;
	unsigned short keep_alive;
	char username[64];
	char password[64];
	char uuid[37];				// random UUID
	char serial_device[32];		// /dev/ttyUSB0
	bool acknowledge_packets;	// false
	bool enforce_uuid_whitelist;	// false
	bool broadcast_to_mqtt;		// true
	int encryption_mode;		// 0 (ctr)
	char encryption_key[64];
	char encryption_iv[32];
	uuid_whitelistT *uuid_whitelist;
	int uuid_whitelist_max;
	int uuid_whitelist_size;
} config_dataT;

void checksum_generate(unsigned char *data, int size, char *checksum);
int checksum_integrity_check(unsigned char *data, int size);
void embed_checksum(unsigned char *data, int size, char *checksum);
void get_checksum(unsigned char *data, int size, char *checksum);
void remove_checksum(unsigned char *data, int size);
void construct_json_str(char *data, char *uuid, char **json_output);
void construct_json_command(char *command, char *uuid, char **json_output);
void read_configuration_file(config_dataT *config);
void read_uuid_whitelist_file(config_dataT *config);
int uuid_whitelist_query(config_dataT *config, char *uuid);
void print_uuid_whitelist(config_dataT *config);
void free_uuid_whitelist(config_dataT *config);
void serial_init(char *serial_port, int *serial);
void serial_rx(int serial, unsigned char *data, int *size);
void serial_tx(int serial, unsigned char *data, int size);
void serial_close(int *serial);
void format_ack(int serial, char *checksum, char **json_output, bool is_ack);
void serial_rx_imdreturn(int serial, unsigned char *data, int *size);
void lora_str_to_mqtt_translate(char *lora_message, char *mqtt_message);
void get_msg_uuid(char *lora_message, char *uuid);

#endif
