#ifndef LRD_SHARED_H
#define LRD_SHARED_H

typedef struct configurationT {
	char uuid[37];
} configurationT;

void read_configuration_file(configurationT *config);
void construct_json_data(char *data, char *uuid, char **json_output);
void serial_transmit(unsigned char *data, int size, char *serial_port);

#endif
