all: fpga_read_write
fpga_read_write: fpga_read_write.c
	$(CROSS_COMPILE)gcc --static fpga_read_write.c -o $(ROOTFS)/bin/fpga_read_write
	