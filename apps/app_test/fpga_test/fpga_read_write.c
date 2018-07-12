#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define FPGA_DEVICE "/dev/fpga_printer_ctl"
int main(int argc,char **argv)
{
	int fd;
	unsigned char *buf;
	int i;

	buf =malloc(1024*1024);

	for(i = 0;i < 1024*1024;i += 2){
		buf[i] = 0x55;
		buf[i + 1] = 0xAA;
	}

	fd = open(FPGA_DEVICE, O_RDWR);
	if(fd < 0){
		printf("open %s error\n", FPGA_DEVICE);
		return 1;
	}

	buf[0] = 0x4C;
	buf[1] = 0x0;

	if(argc == 1){
		printf("read test\n");
		read(fd, buf, 1024*1024);
	}else if(argc == 2){
		printf("write test\n");
		write(fd, buf, 1024*1024);
	}
	close(fd);
	return 0;
}
