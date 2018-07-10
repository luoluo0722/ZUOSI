all: sqlite_test
sqlite_test: sqlite_test.c
	$(CROSS_COMPILE)gcc -o $(ROOTFS)/bin/sqlite_test  sqlite_test.c -lsqlite3 -I$(DIR)include -L$(DIR)lib
clean:
	@rm -vf sqlite_test

