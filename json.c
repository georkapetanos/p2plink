#include "json.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define REALLOC_SIZE 4
#define MAX_STRING_SIZE 8192

typedef struct json_root {
	objectT *objects;
	unsigned int nofobjects;
	unsigned int allocated_objects;
} json_rootT;

typedef struct object {
	char *name;
	char *value;
	json_rootT *branch;
} objectT;

json_rootT *json_init() {
	json_rootT *root = (json_rootT *) malloc(sizeof(json_rootT));
	root->objects = NULL;
	root->nofobjects = 0;
	root->allocated_objects = 0;
	
	return root;
}

char *json_to_string(json_rootT *root, bool nl_formatted) {
	int i;
	char *json_string = NULL, *recursed_string = NULL;
	size_t string_size;
	
	json_string = (char *) malloc(MAX_STRING_SIZE * sizeof(char));
	json_string[0] = '\0';
	strcpy(json_string, "{");
	if(nl_formatted) {
		strcat(json_string, "\n");
	}
	
	for(i = 0; i < root->nofobjects; i++) {
		if(i) {
			strcat(json_string, ",");
			if(nl_formatted) {
				strcat(json_string, "\n");
			}
		}
		if(root->objects[i].branch == NULL) {
			strcat(json_string, "\"");
			strcat(json_string, root->objects[i].name);
			strcat(json_string, "\":\"");
			strcat(json_string, root->objects[i].value);
			strcat(json_string, "\"");
		}
		else {
			strcat(json_string, "\"");
			strcat(json_string, root->objects[i].name);
			strcat(json_string, "\":\"");
			recursed_string = json_to_string(root->objects[i].branch, nl_formatted);
			strcat(json_string, recursed_string);
			free(recursed_string);
		}
	}
	if(nl_formatted) {
		strcat(json_string, "\n");
	}
	strcat(json_string, "}");
	string_size = strlen(json_string);
	json_string = (char *) realloc(json_string, (string_size + 1) * sizeof(char));
	
	return json_string;
}

void json_append_object(json_rootT *root, char *name, char *value) {
	if(root->objects == NULL) {
		root->objects = (objectT *) calloc(1, REALLOC_SIZE * sizeof(objectT));
		root->allocated_objects += REALLOC_SIZE;
	}
	else if(root->allocated_objects <= root->nofobjects) {
		root->objects = (objectT *) realloc(root->objects, (root->allocated_objects + REALLOC_SIZE) * sizeof(objectT));
		root->allocated_objects += REALLOC_SIZE;
	}
	
	root->objects[root->nofobjects].name = strdup(name);
	root->objects[root->nofobjects].value = strdup(value);
	root->objects[root->nofobjects].branch = NULL;
	root->nofobjects++;
}

void json_append_branch(json_rootT *root, json_rootT *branch, char *name) {
	if(root->objects == NULL) {
		root->objects = (objectT *) calloc(1, REALLOC_SIZE * sizeof(objectT));
		root->allocated_objects += REALLOC_SIZE;
	}
	else if(root->allocated_objects <= root->nofobjects) {
		root->objects = (objectT *) realloc(root->objects, (root->allocated_objects + REALLOC_SIZE) * sizeof(objectT));
		root->allocated_objects += REALLOC_SIZE;
	}
	
	root->objects[root->nofobjects].name = strdup(name);
	root->objects[root->nofobjects].value = NULL;
	root->objects[root->nofobjects].branch = branch;
	root->nofobjects++;
}

void get_json_object_value(json_rootT *root, char *name, char *value) {
	int i;

	for(i = 0; i < root->nofobjects; i++) {
		if(!strcmp(root->objects[i].name, name)) {
			strcpy(value, root->objects[i].value);
			break;
		}
	}
}

void json_free_recursion(json_rootT *root, bool is_root) {
	int i;
	
	for(i = 0; i < root->nofobjects; i++) {
		if(root->objects[i].branch != NULL) {
			json_free_recursion(root->objects[i].branch, false);
			free(root->objects[i].name);
			free(root->objects[i].branch);
		}
		else {
			free(root->objects[i].name);
			free(root->objects[i].value);
		}
	}
	free(root->objects);
	if(is_root) {
		free(root);
	}
}

void json_free(json_rootT *root) {
	json_free_recursion(root, true);
}
