
int fpga_read_mem(unsigned short addr, unsigned short *data, int len);
int fpga_write_mem(unsigned short addr, unsigned short *data, int len);
int fpga_init();
void fpga_deinit();
