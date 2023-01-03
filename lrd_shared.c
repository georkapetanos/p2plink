#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <uuid/uuid.h>
#include "lrd_shared.h"
#include "json.h"
#define CONFIGURATION_FILENAME "./config.yaml"
#define DEFAULT_SERIAL "/dev/ttyUSB0"

char *current_timestamp() {
	time_t now;
	time(&now);
	struct tm *local = localtime(&now);
	int hours, minutes, seconds;
	char static timestamp[9];
	
	hours = local->tm_hour;
	minutes = local->tm_min;
	seconds = local->tm_sec;
	
	sprintf(timestamp, "%02d:%02d:%02d", hours, minutes, seconds);
	
	return(timestamp);
}

void uuid_generate_string(char *uuid) {
	uuid_t binuuid;
	
	uuid_generate_random(binuuid);
	uuid_unparse_lower(binuuid, uuid);
	printf("Generated uuid: %s\n", uuid); 
}

int checksum_integrity_check(unsigned char *data, int size) {
	short reductor = 0;
	int i;
	char calculated_checksum[3], checksum[3];
	//there is no termination null character, last character is new line
	checksum[0] = data[size - 2];
	checksum[1] = data[size - 1];
	checksum[2] = '\0';
	
	printf("rx_buf:\n");
	for(i = 0; i < size; i++) {
		printf("%c", data[i]);
	}
	printf("\n");
	
	for(i = 0; i < size - 2; i++) {
		reductor ^= data[i];
		//printf("%x -- %x -- %c\n", reductor, temp, data[i]);
	}
	sprintf(calculated_checksum, "%x", reductor);
	printf("checksum %s, calculated_checksum %s\n", checksum, calculated_checksum);
	if(strcmp(calculated_checksum, checksum) == 0) {
		return 0;
	} else {
		return 1;
	}
}

void checksum_generate(unsigned char *data, int size, char *checksum) {
	short reductor = 0;
	int i;
	
	for(i = 0; i < size; i++) {
		reductor ^= data[i];
	}
	sprintf(checksum, "%x", reductor);
}

void embed_checksum(unsigned char *data, int size, char *checksum) {
	data[size] = checksum[0];
	data[size + 1] = checksum[1];
	data[size + 2] = '\0';
}

void remove_checksum(unsigned char *data, int size) {
	data[size - 2] = '\n';
	//data[size - 1] = '\0';
	//size -= 2;
}

//remove possible carriage return (D hex character) at the start of data stream
//add other preprocess options here
int preprocess_received_data(unsigned char *data, int size) {
	int i;
	
	if(data[0] == 0xD) {
		for(i = 1; i < size; i++) {
			data[i - 1] = data[i];
		}
		//return new size
		return (size - 1);
	}

	return size;
}

void read_configuration_file(config_dataT *config) {
	FILE *filestream = NULL;
	char line[256];
	char *uuid;

	filestream = fopen(CONFIGURATION_FILENAME, "r");

	if (filestream == 0) {
		printf("File %s doesn't exist.\n", CONFIGURATION_FILENAME);
		uuid = (char *) malloc(37*sizeof(char));
		uuid_generate_string(uuid);
		filestream = fopen(CONFIGURATION_FILENAME, "w+");
		fwrite("#LRD configuration file\nuuid: ", 30, 1, filestream);
		fwrite(uuid, 36, 1, filestream);
		fwrite("\nserial: ", 9, 1, filestream);
		fwrite(DEFAULT_SERIAL, 12, 1, filestream);
		fwrite("\n", 1, 1, filestream);
		rewind(filestream);
		free(uuid);
	}
	
	while(1) {
		if(fgets(line, 256, filestream) == NULL) { //read file line by line
			break;
		}
		//printf("%s", line);
		if(strncmp(line, "uuid:", 5) == 0) {
			strncpy(config->uuid, &line[6], 36);
			config->uuid[36] = '\0';
		} else if(strncmp(line, "serial:", 7) == 0) {
			strcpy(config->serial_device, &line[8]);
			//write null termination at last position from string, so as to overwrite new line character copied from above.
			config->serial_device[strlen(config->serial_device) - 1] = '\0';
		}
	}

	fclose(filestream);

}

void construct_json_data(char *data, char *uuid, char **json_output) {
	int count = 0, i;
	char *cur = data, *prev_cur = data;
	char name[256], value[256], count_string[4];
	json_rootT *root, *objects;
	
	root = json_init();
	objects = json_init();
	json_append_object(root, "type", "2");
	
	//count json parameters
	for(i = 0; i < strlen(data); i++) {
		if(data[i] == ',') {
			count++;
		}
	}
	count = (count / 2) + 1;
	sprintf(count_string, "%d", count);
	json_append_object(root, "size", count_string);
	
	do {
		cur = strchr(cur, ',');
		//printf("%s, %ld\n", cur, cur-prev_cur);
		if(cur == NULL) {
			break;
		}
		strncpy(name, prev_cur, cur-prev_cur);
		name[cur-prev_cur] = '\0';
		prev_cur = cur + 1;
		cur = strchr(cur + 1, ',');
		if(cur == NULL) {
			strcpy(value, prev_cur);
			json_append_object(objects, name, value);
			break;
		}
		else {
			strncpy(value, prev_cur, cur-prev_cur);
			value[cur-prev_cur] = '\0';
			prev_cur = cur + 1;
			cur = cur + 1;
			json_append_object(objects, name, value);
		}
	} while(cur != NULL);

	json_append_branch(root, objects, "objs");
	json_append_object(root, "ts", current_timestamp());
	json_append_object(root, "uuid", uuid);
	json_to_string(root, *json_output, false);
	json_free(root);
}

void construct_json_str(char *data, char *uuid, char **json_output) {
	json_rootT *root, *objects;
	
	root = json_init();
	objects = json_init();
	json_append_object(root, "type", "3");
	
	json_append_object(objects, "objs", data);

	json_append_branch(root, objects, "data");
	json_append_object(root, "ts", current_timestamp());
	json_append_object(root, "uuid", uuid);
	json_to_string(root, *json_output, false);
	json_free(root);
}

void serial_transmit(unsigned char *data, int size, char *serial_port) {
	int flags, fd;
	
	//add new line character so that tty will release buffer
	data[size] = '\n';
	flags = O_RDWR | O_NOCTTY;
	if ((fd = open(serial_port, flags)) == -1) {
  		printf("Error opening %s\n", serial_port);
  		exit(1);
	}
	if (write(fd, data, size + 1) < 0 ) {
  		printf("Error while sending data\n");
 		exit(1);
	}
	close(fd);
}

void serial_receive(unsigned char *data, int *size, char *serial_port) {
	int flags, fd, i = 0;
	
	flags = O_RDWR | O_NOCTTY;
	if ((fd = open(serial_port, flags)) == -1) {
  		printf("Error opening %s\n", serial_port);
  		exit(1);
	}
	while(1) {
		read(fd, &data[i], 1);
		if(data[i] == '\n') {
			break;
		}
		i++;
	}
	
	*size = i;
	
	close(fd);
}
