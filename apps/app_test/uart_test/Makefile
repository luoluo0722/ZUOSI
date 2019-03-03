target: 
	$(CROSS_COMPILE)gcc -o $(ROOTFS)/bin/uart_test uart_test.c -lpthread
	$(CROSS_COMPILE)gcc -o $(ROOTFS)/bin/dgus_uart_test dgus_uart_test.c -lpthread
clean:
	@rm -vf uart_test
	@rm -vf dgus_uart_test
