
int fpga_read_mem(unsigned short addr, unsigned short *data, int len);
int fpga_write_mem(unsigned short addr, unsigned short *data, int len);
int fpga_init();
void fpga_deinit();
void fpga_flushall_ctl(unsigned short is_start);
void fpga_flushall_ctl_oneline(unsigned short is_start, int line);
unsigned short fpga_read_reedswitch();
unsigned short fpga_read_flow(int i);
unsigned short fpga_read_ad(int i);
