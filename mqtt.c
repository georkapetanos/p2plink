#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mqtt.h"
#include "lrd_shared.h"
#define CONFIGURATION_FILE "config.yaml"
#define MQTT_TOPIC "lrdlink"
#define IMPORT_LINE_SIZE 256

struct mosquitto *mosq;
config_dataT parsed_data;

void parse_configuration_file(config_dataT *parsed_data) {
	FILE *fp;
	char line[256];
	char *chr;
	
	fp = fopen(CONFIGURATION_FILE, "r");
	if(fp == NULL) {
		printf("Error opening configuration file %s.\n", CONFIGURATION_FILE);
		exit(1);
	}
	
	while(1) {
		if(fgets(line, IMPORT_LINE_SIZE, fp) == NULL) { //read file line by line
			break;
		}
	
		if(line[0] == '#') { //ignore lines starting with '#'
			continue;
		}
		
		//remove new line character
		chr = strchr(line, '\n');
		*chr = '\0';
		
		chr = strchr(line, ':');
		if(chr == NULL) {
			continue;
		}

		if(!strncmp(line, "mqtt_hostname", 13)) {
			strcpy(parsed_data->hostname, chr+2);
		} else if(!strncmp(line, "mqtt_port", 9)) {
			parsed_data->port = atoi(chr+2);
		} else if(!strncmp(line, "mqtt_keep_alive", 15)) {
			parsed_data->keep_alive = atoi(chr+2);
		} else if(!strncmp(line, "mqtt_password", 13)) {
			strcpy(parsed_data->password, chr+2);
		} else if(!strncmp(line, "mqtt_username", 13)) {
			strcpy(parsed_data->username, chr+2);
		} else if(!strncmp(line, "uuid", 4)) {
			strcpy(parsed_data->uuid, chr+2);
		}
	}
	
	fclose(fp);
}

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	printf("Mosquitto MQTT: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	/* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto *mosq, void *obj, int mid) {
	printf("Message with mid %d has been published.\n", mid);
}

void publish_message(char *message, int size) {
	int rc;
	
	rc = mosquitto_publish(mosq, NULL, MQTT_TOPIC, size, message, 2, false);
	
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
	}

}

int mqtt_setup() {
	mosquitto_lib_init();
	int rc;
	
	parse_configuration_file(&parsed_data);
	//printf("%s, %d, %d, %s, %s, %s\n", parsed_data.hostname, parsed_data.port, parsed_data.keep_alive, parsed_data.username, parsed_data.password, parsed_data.id);
	
	mosq = mosquitto_new(NULL, true, NULL);
	if(mosq == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_publish_callback_set(mosq, on_publish);
	
	mosquitto_username_pw_set(mosq, parsed_data.username, parsed_data.password);
	rc = mosquitto_connect(mosq, parsed_data.hostname, parsed_data.port, parsed_data.keep_alive);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}
	
	rc = mosquitto_loop_start(mosq);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}
	
	return 0;
}

void mqtt_cleanup() {
	mosquitto_lib_cleanup();
}
