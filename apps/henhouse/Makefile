all: 
	$(CROSS_COMPILE)gcc -o $(ROOTFS)/bin/henhouse_core -lpthread -lsqlite3 -I$(SQLITE_DESTDIR)/usr/include -L$(SQLITE_DESTDIR)/usr/lib main.c dgus_access.c fpga_access.c sqlite_access.c

clean:
	@rm -vf $(ROOTFS)/bin/henhouse_core

