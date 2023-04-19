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

/* message types
 *
 * 0 -> acknowledgement
 * 1 -> command
 * 2 -> objects (deprecated)
 * 3 -> string
 */

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
	
	printf("rx_buf: ");
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
}

void read_configuration_file(config_dataT *config) {
	FILE *filestream = NULL;
	char line[256];
	char *uuid;
	char read_bool[6];

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
		fwrite("\nacknowledge_packets: false", 27, 1, filestream);
		fwrite("\nbroadcast_to_mqtt: true", 24, 1, filestream);
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
		} else if(strncmp(line, "acknowledge_packets:", 20) == 0) {
			strncpy(read_bool, &line[21], 5);
			if(strncmp(read_bool, "true", 4) == 0) {
				config->acknowledge_packets = true;
			} else if(strncmp(read_bool, "false", 4) == 0){
				config->acknowledge_packets = false;
			} else {
				config->acknowledge_packets = false;
			}
		} else if(strncmp(line, "broadcast_to_mqtt:", 18) == 0) {
			strncpy(read_bool, &line[19], 5);
			if(strncmp(read_bool, "true", 4) == 0) {
				config->broadcast_to_mqtt = true;
			} else if(strncmp(read_bool, "false", 4) == 0){
				config->broadcast_to_mqtt = false;
			} else {
				config->broadcast_to_mqtt = true;
			}
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

void construct_json_str(char *data, char *uuid, char **json_output) {
	json_rootT *root;
	
	root = json_init();
	
	json_append_object(root, "c", "3");
	json_append_object(root, "u", uuid);
	json_append_object(root, "t", current_timestamp());
	json_append_object(root, "p", data);
	json_to_string(root, *json_output, false);
	json_free(root);
}

void construct_json_command(char *command, char *uuid, char **json_output) {
	json_rootT *root;
	
	root = json_init();
	
	json_append_object(root, "c", "1");
	json_append_object(root, "u", uuid);
	json_append_object(root, "t", current_timestamp());
	json_append_object(root, "p", command);
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
	json_append_object(root, "c", "0");
	json_append_object(root, "x", checksum);
	if(is_ack) {
		json_append_object(root, "a", "1");
	} else {
		json_append_object(root, "a", "-1");
	}
	json_to_string(root, *json_output, false);
	json_free(root);
}

/*static char *last_strstr(const char *haystack, const char *needle)
{
    if (*needle == '\0')
        return (char *) haystack;

    char *result = NULL;
    for (;;) {
        char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }

    return result;
}*/

/* JSON Initials
 * c -> class, message class type
 * u -> uuid
 * t -> timestamp, timestamp from transmitter
 * p -> payload, message data payload
 *
 * x -> checksum, on acknowlegdement packets
 * a -> ack status, on acknowlegdement packets
 */

void lora_str_to_mqtt_translate(char *lora_message, char *mqtt_message) {
	char *occur, *last_occur;
	
	//failsafe in case message class is not "c", string type
	if(strncmp(lora_message, "{\"c\":\"3\"", 8) == 0) {
		occur = strstr(lora_message, "\"c\"");
		strncpy(mqtt_message, lora_message, occur - lora_message);
		strcat(mqtt_message, "\"class\"");
		last_occur = strstr(lora_message, "\"u\"");
		strncat(mqtt_message, occur + 3, last_occur - (occur + 3));
		strcat(mqtt_message, "\"uuid\"");
		occur = strstr(lora_message, "\"t\"");
		strncat(mqtt_message, last_occur + 3, occur - (last_occur + 3));
		strcat(mqtt_message, "\"timestamp\"");
		last_occur = strstr(lora_message, "\"p\"");
		strncat(mqtt_message, occur + 3, last_occur - (occur + 3));
		strcat(mqtt_message, "\"payload\"");
		strcat(mqtt_message, last_occur + 3);
	}
	else {
		strcpy(mqtt_message, lora_message);
	}
}
