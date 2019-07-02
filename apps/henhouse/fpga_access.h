
int fpga_init();
void fpga_deinit();
void fpga_flushctl_all(unsigned short is_start);
void fpga_flushctl_oneline(unsigned short is_start, int line);
unsigned short fpga_read_reedswitch();
unsigned short fpga_read_flow(int i);
unsigned short fpga_read_ad(int i);
