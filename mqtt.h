#ifndef MQTT_H
#define MQTT_H

void publish_message(char *message, int size);
int mqtt_setup();
void mqtt_cleanup();

#endif
