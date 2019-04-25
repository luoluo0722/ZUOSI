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

	printf("addr = %x, data = %x\n", addr, *data);
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

void fpga_flushall_ctl(unsigned short is_start){
	unsigned short data = is_start ? 0 : 0xffff;
	int i = 0;
	while(i < 1){
		fpga_write_mem(i << 1, &data, 1);
		i++;
	}
}

void fpga_flushall_ctl_oneline(unsigned short is_start, int line){
	int addr = line/16;
	unsigned short mask = 1 << (line % 16);
	unsigned short data = is_start ? ~mask : 0xffff;

	fpga_write_mem(addr << 1, &data, 1);
}

unsigned short fpga_read_temp(int i){
	unsigned short addr = i + 4;
	unsigned short data;

	fpga_read_mem(addr << 1, &data, 1);
	return data;
}

unsigned short fpga_read_pressure(int i){
	unsigned short addr = i + 6;
	unsigned short data;

	fpga_read_mem(addr << 1, &data, 1);
	return data;
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
	fpga_flushall_ctl(0);
	return 0;
}

void fpga_deinit(){
	if(fpga_fd > 0){
		close(fpga_fd);
	}
}

