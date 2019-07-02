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
static unsigned short buf_read[CONTROL_STATUS_MEM_LEN + 1];
static unsigned short buf_write[CONTROL_STATUS_MEM_LEN + 1] = {0, 0xffff, 0xffff, 0xffff, 0xffff};

static int fpga_read_mem(unsigned short addr, int len){

	int ret = -1;
	if(fpga_fd > 0 &&
		len <= CONTROL_STATUS_MEM_LEN &&
		len > 0){
		memset(buf_read, 0, len * 2 + 2);
		buf_read[0] = addr;
		ret = read(fpga_fd, buf_read, len * 2 + 2);
	}
	return ret;
}

static int fpga_write_mem(unsigned short addr, int len){

	int ret = -1;
	if(fpga_fd > 0 &&
		len <= CONTROL_STATUS_MEM_LEN &&
		len > 0){
		buf_write[0] == addr;
		ret = write(fpga_fd, buf_write, len * 2 + 2);
	}
	return ret;
}

void fpga_flushctl_all(unsigned short is_start){
	int i = 0;
	unsigned short data = is_start ? 0x0 : 0xffff;

	buf_write[1] = buf_write[2] = buf_write[3] = buf_write[4] = data;
	fpga_write_mem(0, 4);
}

void fpga_flushctl_oneline(unsigned short is_start, int line){
	int addr = line / 16;
	unsigned short mask = 1 << (line % 16);

	if(is_start == 1){
		buf_write[addr] &= ~mask;
	}else{
		buf_write[addr] |= mask;
	}
	fpga_write_mem(addr << 1, 1);
}

unsigned short fpga_read_ad(int i){
	unsigned short addr = i + 6;

	fpga_read_mem(addr << 1, 1);
	return buf_read[1];
}

unsigned short fpga_read_reedswitch(){
	unsigned short addr = 0x5;

	fpga_read_mem(addr << 1, 1);
	return buf_read[1];
}

unsigned short fpga_read_flow(int i){
	unsigned short addr = i;

	fpga_read_mem(addr << 1, 1);
	return buf_read[1];
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
	fpga_flushctl_all(0);
	return 0;
}

void fpga_deinit(){
	if(fpga_fd > 0){
		close(fpga_fd);
	}
}

