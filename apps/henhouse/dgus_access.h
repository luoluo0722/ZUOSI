
struct dgus_keypress_callback{
	unsigned short key_addr_offset;
	void (*callback)(void *param);
};

struct dgus_page_callback{
	unsigned short page_num;
	void (*callback)(void *param);
};

struct dgus_callback{
	struct dgus_keypress_callback *keypress_callback;
	struct dgus_page_callback *page_callback;
};

//int dgus_access_reg(unsigned char reg, int is_write, unsigned char *data, unsigned char len);
//int dgus_access_address(unsigned short addr, int is_write, unsigned short *data, unsigned short len);
int dgus_init(struct dgus_callback *main_callback);
void dgus_deinit();
