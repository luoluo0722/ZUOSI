#include <stdio.h>
#include "sqlite3.h"

#define SQLITE_DATABASE_FILE_NAME "/data/mdatabase.db"

static sqlite3 *sqlite_db = NULL;

int exec_sql(){
}

int sqlite_mod_init(){
	int result;
	char *errmsg = NULL;
	if(sqlite_db != NULL){
		return 0;
	}
	result = sqlite3_open(SQLITE_DATABASE_FILE_NAME, &sqlite_db);
	if(result != SQLITE_OK){
		printf("Can't open datebase\n%s\n", sqlite3_errmsg(sqlite_db));
		return -1;
	}
	return 0;
}

void sqlite_mod_deinit(){
	if(sqlite_db != NULL){
		sqlite3_close(sqlite_db);
	}
}

