#include <stdio.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <unistd.h>
#include <fcntl.h>
#include "json.h"
#define SAMPLE_UUID "55aa7fcc-3986-11ed-a261-0242ac120002"
#define CONFIGURATION_FILENAME "./config.yaml"
#define PUBLIC_KEY_FILE "./public.pem"
#define PRIVATE_KEY_FILE "./private.pem"
#define MAX_MSG_BUFFER 4096
#define SERIAL "/dev/ttyUSB0"

/* Transmit procedure
* Parse input from user, generate JSON array,
* insert uuid in JSON, encrypt JSON array,
* transmit to serial
* TODO checksum
*
*/

typedef struct configurationT {
	char uuid[37];
} configurationT;

void read_configuration_file(configurationT *config) {
	FILE *filestream = NULL;
	char line[256];

	filestream = fopen(CONFIGURATION_FILENAME, "r");

	if (filestream == 0) {
		printf("File %s doesn't exist.\n", CONFIGURATION_FILENAME);
		filestream = fopen(CONFIGURATION_FILENAME, "w+");
		fwrite("#LRD configuration file\nuuid: ", 23, 1, filestream);
		fwrite(SAMPLE_UUID, 36, 1, filestream);
		fwrite("\n", 1, 1, filestream);
		rewind(filestream);
	}
	
	while(1) {
		if(fgets(line, 256, filestream) == NULL) { //read file line by line
			break;
		}
		//printf("%s", line);
		if(strncmp(line, "uuid:", 5) == 0) {
			strncpy(config->uuid, &line[6], 36);
			config->uuid[36] = '\0';
		}
	}

	fclose(filestream);

}

void construct_json_command(char *command, char *uuid, char **json_output) {
	json_rootT *root;
	
	root = json_init();
	json_append_object(root, "type", "1");
	json_append_object(root, "exec", command);
	json_append_object(root, "uuid", uuid);
	json_append_object(root, "ts", "16:17:31");
	*json_output = json_to_string(root, false);
}
/* data format name0,value0,name1,value1,...
*/
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
	json_append_object(root, "uuid", uuid);
	//json_append_object(root, "ts", "16:17:31");
	*json_output = json_to_string(root, false);
}

void print_openssl_error() {
	unsigned long error;
	char error_string[512];
	
	error = ERR_get_error();
	ERR_error_string(error, error_string);
	printf("Error = %s\n", error_string);
}

void encrypt_json_data(char *plain_json, unsigned char **encrypted_json, int *encrypted_json_length) {
	FILE *pfile = NULL;
	RSA* pPubKey  = NULL;
	int plain_json_length = strlen(plain_json);
	*encrypted_json = (unsigned char *) calloc(1, MAX_MSG_BUFFER * sizeof(unsigned char));
	
	pfile = fopen(PUBLIC_KEY_FILE,"r");
	if (pfile == 0) {
		printf("File %s doesn't exist.\n", PUBLIC_KEY_FILE);
	}
	pPubKey = PEM_read_RSA_PUBKEY(pfile,NULL,NULL,NULL);
	fclose(pfile);
	*encrypted_json_length = RSA_public_encrypt(plain_json_length, (unsigned char *) plain_json, *encrypted_json, pPubKey, RSA_PKCS1_PADDING);
	printf("encrypted_length = %d\n", *encrypted_json_length);
}

void decrypt_json_data(unsigned char *encrypted_json, unsigned char **plain_json, int encrypted_json_length) {
	FILE *pfile = NULL;
	RSA* pPubKey  = NULL;
	int decrypted_length;
	*plain_json = (unsigned char *) calloc(1, MAX_MSG_BUFFER * sizeof(unsigned char));
	
	pfile = fopen(PRIVATE_KEY_FILE,"r");
	if (pfile == 0) {
		printf("File %s doesn't exist.\n", PRIVATE_KEY_FILE);
	}
	pPubKey = PEM_read_RSAPrivateKey(pfile,NULL,NULL,NULL);
	fclose(pfile);
	decrypted_length = RSA_private_decrypt(encrypted_json_length, encrypted_json, *plain_json, pPubKey, RSA_PKCS1_PADDING);
	printf("decrypted_length = %d\n", decrypted_length);
	//plain_json[decrypted_length] = '\0';
}

void serial_transmit(unsigned char *data, int size, char *serial_port) {
	char command[256];
	int flags, fd;
	
	strcpy(command, "stty -F ");
	strcat(command, serial_port);
	strcat(command, " -echo inlcr");
	printf("%s\n", command);
	system(command);
	
	//add new line character so that tty will release buffer
	data[size] = '\n';
	flags = O_RDWR | O_NOCTTY;
	if ((fd = open(serial_port, flags)) == -1) {
  		printf("Error opening %s\n", SERIAL);
  		return ;
	}
	if (write(fd, data, size + 1) < 0 ) {
  		printf("Error while sending data\n");
 		return ;
	}
	close(fd);
}

void serial_receive(unsigned char *data, int *size, char *serial_port) {
	int flags, fd, i = 0;
	
	flags = O_RDWR | O_NOCTTY;
	if ((fd = open(serial_port, flags)) == -1) {
  		printf("Error opening %s\n", SERIAL);
  		return ;
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

int main(int argc, char *argv[]) {
	configurationT config;
	int i, j;
	char *json_string = NULL;
	unsigned char *encrypted_json = NULL, *plain_json = NULL;
	int encrypted_json_length;
	unsigned char *rx_buf;
	int rx_size;
		
	read_configuration_file(&config);
	//printf("uuid = %s\n", config.uuid);
	
	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_ciphers();
	ERR_load_crypto_strings();
	
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
				printf("command: %s\n", argv[i+1]);
				construct_json_data(argv[i+1], config.uuid, &json_string);
				printf("json:\n%s\n", json_string);
				encrypt_json_data(json_string, &encrypted_json, &encrypted_json_length);
				for(j = 0; j < encrypted_json_length; j++) {
					printf("%x", encrypted_json[j]);
				}
				printf("\n");
				decrypt_json_data(encrypted_json, &plain_json, encrypted_json_length);
				printf("Plain JSON follows:\n%s\n", plain_json);
				//serial_transmit(encrypted_json, encrypted_json_length, SERIAL);
				serial_transmit((unsigned char *) json_string, strlen(json_string), SERIAL);
				i++;
			} else {
				printf("Invalid Syntax\n");
			}
		} else if(strncmp(argv[i], "-r", 2) == 0) {
			rx_buf = malloc(MAX_MSG_BUFFER*sizeof(unsigned char));
			serial_receive(rx_buf, &rx_size, SERIAL);
			printf("rx_buf: ");
			for(i = 0; i < rx_size; i++) {
				printf("%c", rx_buf[i]);
			}
			printf("\n");
			free(rx_buf);
		} else if(strncmp(argv[i], "-h", 2) == 0) {
			printf("Usage message\n");
		}
		else {
			printf("Usage message\n");
		}
	}
	
	return 0;
}
