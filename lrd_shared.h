#ifndef LRD_SHARED_H
#define LRD_SHARED_H

typedef struct config_data{
	char hostname[256];
	unsigned short port;
	unsigned short keep_alive;
	char username[64];
	char password[64];
	char uuid[37];
	char serial_device[32];
} config_dataT;

void construct_json_str(char *data, char *uuid, char **json_output);
void read_configuration_file(config_dataT *config);
void construct_json_data(char *data, char *uuid, char **json_output);
void serial_transmit(unsigned char *data, int size, char *serial_port);
void serial_receive(unsigned char *data, int *size, char *serial_port);

#endif
