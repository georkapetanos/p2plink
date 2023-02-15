#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
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
	sprintf(calculated_checksum, "%02x", reductor); // add 02 for constant checksum size
	//printf("checksum %s, calculated_checksum %s\n", checksum, calculated_checksum);
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
	//printf("checksum_generate at %d, %d\n", __LINE__, reductor);
	sprintf(checksum, "%02x", reductor); // add 02 for constant checksum size
}

void embed_checksum(unsigned char *data, int size, char *checksum) {
	data[size] = checksum[0];
	data[size + 1] = checksum[1];
	data[size + 2] = '\0';
}

void get_checksum(unsigned char *data, int size, char *checksum) {
	//there is no termination null character, last character is new line
	checksum[0] = data[size - 2];
	checksum[1] = data[size - 1];
	checksum[2] = '\0';
}

void remove_checksum(unsigned char *data, int size) {
	data[size - 2] = '\n';
	//data[size - 1] = '\0';
	//size -= 2;
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
		} else if(strncmp(line, "encryption_key:", 15) == 0) {
			strcpy(config->encryption_key, &line[16]);
			//write null termination at last position from string, so as to overwrite new line character copied from above.
			config->encryption_key[strlen(config->encryption_key) - 1] = '\0';
		} else if(strncmp(line, "encryption_iv:", 14) == 0) {
			strcpy(config->encryption_iv, &line[15]);
			//write null termination at last position from string, so as to overwrite new line character copied from above.
			config->encryption_iv[strlen(config->encryption_iv) - 1] = '\0';
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

void serial_init(char *serial_port, int *serial) {
	*serial = open(serial_port, O_RDWR);
	struct termios tty;
	
	if (*serial < 0) {
		printf("Error opening %s\n", serial_port);
  		exit(1);
	}

	// Read in existing settings, and handle any error
	// NOTE: This is important! POSIX states that the struct passed to tcsetattr()
	// must have been initialized with a call to tcgetattr() overwise behaviour
	// is undefined
	if(tcgetattr(*serial, &tty) != 0) {
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE; // Clear all the size bits
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;
	// Set in/out baud rate to be 9600
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);
	// Save tty settings, also checking for error
	if (tcsetattr(*serial, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}
}

void serial_rx(int serial, unsigned char *data, int *size) {
	int rx_size = 0;	
	
	*size = 0;
	while(1) {
		rx_size = read(serial, &data[*size], 4096);
		//printf("rx_size = %d\n", rx_size);
		if((rx_size == 0) && (*size != 0)) {
			break;
		}
		*size += rx_size;
	}
}

//return immediately rx, used for receiving acks
void serial_rx_imdreturn(int serial, unsigned char *data, int *size) {
	int rx_size = 0;	
	
	*size = 0;
	while(1) {
		rx_size = read(serial, &data[*size], 4096);
		//printf("rx_size = %d\n", rx_size);
		if(rx_size == 0) {
			break;
		}
		*size += rx_size;
	}
}

void serial_tx(int serial, unsigned char *data, int size) {
	write(serial, data, size);
}

void serial_close(int *serial) {
	close(*serial);
}

void format_ack(int serial, char *checksum, char **json_output, bool is_ack) {
	json_rootT *root;
	
	root = json_init();
	json_append_object(root, "type", "0");
	json_append_object(root, "chksum", checksum);
	if(is_ack) {
		json_append_object(root, "ACK", "1");
	} else {
		json_append_object(root, "ACK", "-1");
	}
	json_to_string(root, *json_output, false);
	json_free(root);
}
