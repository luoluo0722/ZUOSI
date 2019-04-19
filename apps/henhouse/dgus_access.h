typedef void (*DGUS_REPORT_CALLBACK)(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len);
struct dgus_keypress_callback{
	unsigned short key_addr_offset;
	DGUS_REPORT_CALLBACK callback;
};

typedef void (*DGUS_PAGECHANGE_CALLBACK)(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len);
struct dgus_page_callback{
	unsigned short page_num;
	DGUS_PAGECHANGE_CALLBACK callback;
};

struct dgus_callback{
	struct dgus_keypress_callback *keypress_callback;
	struct dgus_page_callback *page_callback;
};

//int dgus_access_reg(unsigned char reg, int is_write, unsigned char *data, unsigned char len);
//int dgus_access_address(unsigned short addr, int is_write, unsigned short *data, unsigned short len);
int dgus_init(struct dgus_callback *main_callback);
void dgus_deinit();
void dgus_get_now_date(unsigned char *data, int len);
