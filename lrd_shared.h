#ifndef LRD_SHARED_H
#define LRD_SHARED_H

#include <stdbool.h>

typedef struct config_data{
	char hostname[256];
	unsigned short port;
	unsigned short keep_alive;
	char username[64];
	char password[64];
	char uuid[37];
	char serial_device[32];
	char encryption_key[64];
	char encryption_iv[64];
} config_dataT;

void checksum_generate(unsigned char *data, int size, char *checksum);
int checksum_integrity_check(unsigned char *data, int size);
void embed_checksum(unsigned char *data, int size, char *checksum);
void get_checksum(unsigned char *data, int size, char *checksum);
void remove_checksum(unsigned char *data, int size);
void construct_json_str(char *data, char *uuid, char **json_output);
void read_configuration_file(config_dataT *config);
void construct_json_data(char *data, char *uuid, char **json_output);
void serial_init(char *serial_port, int *serial);
void serial_rx(int serial, unsigned char *data, int *size);
void serial_tx(int serial, unsigned char *data, int size);
void serial_close(int *serial);
void format_ack(int serial, char *checksum, char **json_output, bool is_ack);
void serial_rx_imdreturn(int serial, unsigned char *data, int *size);

#endif
