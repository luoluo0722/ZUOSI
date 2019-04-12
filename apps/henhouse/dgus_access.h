
int dgus_access_reg(unsigned char reg, int is_write, unsigned char *data, unsigned char len);
int dgus_access_address(unsigned short addr, int is_write, unsigned short *data, unsigned short len);
int dgus_init();
void dgus_deinit();
