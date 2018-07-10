#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define FPGA_DEVICE "/dev/fpga_printer_ctl"
int main(int argc,char **argv)
{
	int fd;
	unsigned char buf[16]={0};

	fd = open(FPGA_DEVICE, O_RDWR);
	if(fd < 0){
			printf("open %s error\n", FPGA_DEVICE);
			return 1;
	}

	buf[0] = 0x12;
	buf[1] = 0x34;
	if(argc == 1){
		printf("read test\n");
		read(fd, buf, 16);
	}else if(argc == 2){
		printf("write test\n");
		write(fd, buf, 16);
	}
	close(fd);
	return 0;
}
