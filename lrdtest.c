#include <stdio.h>
#include <stdlib.h>
#include "json.h"

int main(int argc, char *argv[]) {
	json_rootT *myjson, *mybranch;
	
	myjson = json_init();
	mybranch = json_init();
	
	json_append_object(myjson, "temperature", "26.71");
	json_append_object(myjson, "humidity", "56.48");
	json_append_object(myjson, "pressure", "1009.41");
	
	json_append_object(mybranch, "dev_uuid", "c563005e-3402-49a6-8630-5d69a3e440ff");
	json_append_object(mybranch, "date", "2022-09-20");
	
	json_append_branch(myjson, mybranch, "info");
	json_append_object(myjson, "version", "1.0.1");
	
	char *json_string = json_to_string(myjson, false);
	printf("%s\n\n", json_string);
	free(json_string);

	json_string = json_to_string(myjson, true);
	printf("%s\n", json_string);
	free(json_string);

	json_free(myjson);

	return 0;
}
