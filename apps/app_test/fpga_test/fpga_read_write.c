#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPGA_DEVICE "/dev/fpga_printer_ctl"
int main(int argc,char **argv)
{
	int fd;
	unsigned char *buf;
	int i;
	int is_write;
	int remain_arg_count;

	buf =malloc(1024*1024);
	if(buf == NULL){
		return -1;
	}
	fd = open(argv[1], O_RDWR);
	if(fd < 0){
		printf("open %s error\n", FPGA_DEVICE);
		return 1;
	}

	buf[0] = strtoul(argv[3], 0, 0);
	buf[1] = 0x0;

	if(strcmp(argv[2], "0") == 0){
		is_write = 0;
	}else if(strcmp(argv[2], "1") == 0){
		is_write = 1;
		remain_arg_count = argc - 4;
		i = 0;
		while(i < remain_arg_count){
			buf[i + 2] = strtoul(argv[i + 4], 0, 0);
			i ++;
		}
	}

	if(is_write == 0){
		printf("read test\n");
		read(fd, buf, 4);
	}else if(is_write == 1){
		printf("write test\n");
		write(fd, buf, 4);
	}
	close(fd);
	return 0;
}
