#include <stdbool.h>

#ifndef _JSON_H
#define _JSON_H

typedef struct json_root json_rootT;
typedef struct object objectT;

json_rootT *json_init();

void json_to_string(json_rootT *root, char *json_string, bool nl_formatted);

void json_append_object(json_rootT *root, char *name, char *value);

void json_append_branch(json_rootT *root, json_rootT *branch, char *name);

void json_free(json_rootT *root);

void get_json_object_value(json_rootT *root, char *name, char *value);

#endif
