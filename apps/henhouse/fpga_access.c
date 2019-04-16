#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPGA_DEVICE "/dev/fpga_printer_ctl"
#define CONTROL_STATUS_MEM_LEN 4

static int fpga_fd = -1;
static unsigned short buf[CONTROL_STATUS_MEM_LEN + 1];

int fpga_read_mem(unsigned short addr, unsigned short *data, int len){

	int ret = 0;

	if(fpga_fd > 0 &&
		data != NULL &&
		len <= CONTROL_STATUS_MEM_LEN &&
		len > 0){
		memset(buf, 0, sizeof(buf));
		buf[0] == addr;
		ret = read(fpga_fd, buf, len * 2 + 2);
		if(ret > 0){
			memcpy(data, buf + 1, len * 2);				
		}
	}
	return ret;
}

int fpga_write_mem(unsigned short addr, unsigned short *data, int len){

	int ret = 0;

	if(fpga_fd > 0 &&
		data != NULL &&
		len <= CONTROL_STATUS_MEM_LEN &&
		len > 0){
		memset(buf, 0, sizeof(buf));
		buf[0] == addr;
		memcpy(buf + 1, data, len * 2);	
		ret = write(fpga_fd, buf, len * 2 + 2);
	}
	return ret;
}

int fpga_init(){
	if(fpga_fd > 0){
		return 0;
	}
	fpga_fd = open(FPGA_DEVICE, O_RDWR);
	if(fpga_fd < 0){
		printf("open %s error\n", FPGA_DEVICE);
		return -1;
	}
	return 0;
}

void fpga_deinit(){
	if(fpga_fd > 0){
		close(fpga_fd);
	}
}

void fpga_flushall_ctl(unsigned short is_start){
	unsigned short data = is_start ? 0xffff : 0;
	int i = 0;
	while(i < 4){
		fpga_write_mem(i << 1, &data, 1);
	}
}

