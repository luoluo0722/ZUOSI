#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define FPGA_DEVICE "/dev/fpga_printer_ctl"
#define TTY_DEVICE "/dev/ttyS01"

#define FALSE 1
#define TRUE 0



int speed_arr[] = {  B115200, B57600, B38400, B19200, B9600, B4800,
		    B2400, B1200};
int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};

#define debugnum(data,len,prefix)  \
{ \
        unsigned int i;   \
        for (i = 0;i < len;i++) { \
                if(prefix)  \
                        printf("0x%02x ",data[i]); \
                else  \
                        printf("%02x ",data[i]); \
        } \
}

void set_speed(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
    {
	if (speed == name_arr[i])
	{
		tcflush(fd, TCIOFLUSH);
	    cfsetispeed(&Opt, speed_arr[i]);
	    cfsetospeed(&Opt, speed_arr[i]);
	    status = tcsetattr(fd, TCSANOW, &Opt);
	    if (status != 0)
		perror("tcsetattr fd1");
	    return;
	}
	tcflush(fd,TCIOFLUSH);
    }
}

#define HEADER_BYTE1 0x5a
#define HEADER_BYTE2 0xa5
int access_reg(int fd_uart, unsigned char reg, int is_write, unsigned char len, unsigned char *data)
{
	unsigned char *buf;
	int buf_len, cmd_len;
	int nread;
	int cmd_code_len;

	if(reg > 0xff){
		return -1;
	}
	if(is_write != 0 && is_write != 1){
		return -2;
	}
	if(reg + len > 0xff){
		return -3;
	}
	if(fd_uart <= 0){
		return -4;
	}
	cmd_code_len = is_write == 0 ? 6 : 5;
	buf_len = cmd_code_len + len;
	buf = malloc(buf_len);
	if(buf == NULL){
		return -5;
	}
	memset(buf, 0, buf_len);

	buf[0] = HEADER_BYTE1;
	buf[1] = HEADER_BYTE2;
	if(is_write == 0){
		buf[2] = 3;
		buf[3] = 0x81;
		buf[5] = len;
		cmd_len = buf_len - len;
	}else{
		buf[2] = 2 + len;
		buf[3] = 0x80;
		memcpy(buf + 5, data, len);
		cmd_len = buf_len;
	}
	buf[4] = reg;

	write(fd_uart, buf, cmd_len);
	if(is_write == 0){
		nread = read(fd_uart, buf, cmd_len);
		if(nread != cmd_len){
			free(buf);
			return -6;
		}
		memcpy(buf + 6, data, len);
	}
	free(buf);
	return 0;
}
int access_address(int fd_uart, unsigned short addr, int is_write, unsigned short len, unsigned short *data){
	unsigned char *buf, *p;
	int buf_len, cmd_len;
	int nread;
	int i;
	int cmd_code_len;

	if(addr > 0x6FFF){
		return -1;
	}
	if(is_write != 0 && is_write != 1){
		return -2;
	}
	if(addr + len > 0x6FFF){
		return -3;
	}
	if(fd_uart <= 0){
		return -4;
	}

	cmd_code_len = is_write == 0 ? 7 : 6;
	buf_len = cmd_code_len + len * 2;
	buf = malloc(buf_len);
	if(buf == NULL){
		return -5;
	}
	memset(buf, 0, buf_len);

	buf[0] = HEADER_BYTE1;
	buf[1] = HEADER_BYTE1;
	if(is_write == 0){
		buf[2] = 4;
		buf[3] = 0x83;
		buf[6] = len;
		cmd_len = buf_len - len * 2;
	}else{
		buf[2] = 3 + len * 2;
		buf[3] = 0x82;
		i = 0;
		p = buf + 6;
		while(i < len * 2){
			*(p + i) = data[i/2] >> 8;
			*(p + i + 1) = data[i/2] & 0xff;
			i += 2;
		}
		cmd_len = buf_len;
	}
	buf[4] = addr >> 8;
	buf[5] = addr & 0xff;

	write(fd_uart, buf, cmd_len);
	if(is_write == 0){
		nread = read(fd_uart, buf, buf_len);
		if(nread != buf_len){
			free(buf);
			return -6;
		}
		i = 0;
		p = buf + 7;
		while(i < len * 2){
			data[i/2] = *(p + i) << 8 | *(p + i + 1);
			i += 2;
		}
	}
	free(buf);
	return 0;

}

int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
	if  ( tcgetattr( fd,&options)  !=  0)
	{
		perror("SetupSerial 1");
		return(FALSE);
	}
	options.c_cflag &= ~CSIZE;
	switch (databits)
	{
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr,"Unsupported data size\n");
		return (FALSE);
	}
	switch (parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   
		options.c_iflag &= ~INPCK;   
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB); 
		options.c_iflag |= INPCK;           
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;     
		break;
	case 'S':
	case 's':  
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported parity\n");
		return (FALSE);
	}
	switch (stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported stop bits\n");
		return (FALSE);
	}


	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);

	/* Set input parity option */

	if (parity != 'n')
		options.c_iflag |= INPCK;
	options.c_cc[VTIME] = 150; // 15 seconds
	options.c_cc[VMIN] = 0;


	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}

int main(int argc,char **argv){
	fd_set reads, temps;
	int result, str_len;
	struct timeval timeout;
	int is_write;
	unsigned long data_len;
	unsigned long reg_addr;

	pthread_t receiveid;
    int  c, ctrlbits,cmd=0;
	int fd_uart = -1, fd_fpga = -1;

	fd_uart = open(argv[1], O_RDWR); /* argv 1 is tty device */
	if (fd_uart < 0){
		printf("open device %s faild\n", argv[1]);
		exit(0);
	}
	set_speed(fd_uart,115200);
	set_Parity(fd_uart,8,1,'N');

	reg_addr = strtoul(argv[4], 0, 0); /* arg 4 is address or reg */

	if(strcmp(argv[3], "0") == 0){ /* argv 3 is write flag */
		is_write= 0;
		data_len = strtoul(argv[5], 0, 0);
	}else if(strcmp(argv[3], "1") == 0){
		is_write= 1;
		data_len = argc - 5;
	}

	if(strcmp(argv[2], "0") == 0){ /* arg 2 is address flag */
		unsigned char *buf = malloc(data_len);
		if(buf == NULL){
			close(fd_uart);
			return -1;
		}
		if(is_write == 1){
			int i = 0;
			while(i < data_len){
				buf[i] = strtoul(argv[i + 5], 0, 0);
				i++;
			}
		}
		access_reg(fd_uart, reg_addr, is_write, data_len, buf);
		if(is_write == 0){
			int i = 0;
			printf("Read data:\n");
			while(i < data_len){
				printf("%2x, ", buf[i]);
				i++;
			}
		}
		free(buf);
	}else if(strcmp(argv[2], "1") == 0){
		unsigned short *buf = malloc(data_len * sizeof(unsigned short));
		if(buf == NULL){
			close(fd_uart);
			return -1;
		}
		if(is_write == 1){
			int i = 0;
			while(i < data_len){
				unsigned short data = strtoul(argv[i + 5], 0, 0);
				buf[i * 2] = data >> 0x8;
				buf[i * 2 + 1] = data & 0xff;
				i++;
			}
		}
		access_address(fd_uart, reg_addr, is_write, data_len, buf);
		if(is_write == 0){
			int i = 0;
			printf("Read data:\n");
			while(i < data_len){
				printf("%4x, ", buf[i]);
				i++;
			}
		}
		free(buf);
	}

	close(fd_uart);
	return 0;

}
