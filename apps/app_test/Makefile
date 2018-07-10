SQLITE_TEST_SRC := $(APP_TEST_SRC)/sqlite_test
UART_TEST_SRC := $(APP_TEST_SRC)/uart_test
FPGA_TEST_SRC := $(APP_TEST_SRC)/fpga_test

all: sqlite_test_app uart_test_app fpga_read_write


sqlite_test_app:
	@echo =====================================
	@echo     Building the sqlite_test
	@echo =====================================
	$(MAKE) -C $(SQLITE_TEST_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ROOTFS=$(ROOTFS)

uart_test_app:
	@echo =====================================
	@echo     Building the uart_test
	@echo =====================================
	$(MAKE) -C $(UART_TEST_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ROOTFS=$(ROOTFS)

fpga_read_write:
	@echo =====================================
	@echo     Building the fpga_read_write
	@echo =====================================
	$(MAKE) -C $(FPGA_TEST_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ROOTFS=$(ROOTFS)
