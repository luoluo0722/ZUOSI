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

#define TTY_DEVICE "/dev/ttyS01"

#define FALSE 1
#define TRUE 0
#define HEADER_BYTE1 0x5a
#define HEADER_BYTE2 0xa5

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

static int dgus_fd = -1;
static int speed_arr[] = {  B115200, B57600, B38400, B19200, B9600, B4800,
		    B2400, B1200};
static int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};

static void set_speed(int fd, int speed)
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

static int set_Parity(int fd,int databits,int stopbits,int parity)
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

int dgus_access_reg(unsigned char reg, int is_write, unsigned char *data, unsigned char len){
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
	if(dgus_fd <= 0){
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

	write(dgus_fd, buf, cmd_len);
	if(is_write == 0){
		nread = read(dgus_fd, buf, cmd_len);
		if(nread != cmd_len){
			free(buf);
			return -6;
		}
		memcpy(buf + 6, data, len);
	}
	free(buf);
	return 0;
}

int dgus_access_address(unsigned short addr, int is_write, unsigned short *data, unsigned short len){
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
	if(dgus_fd <= 0){
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

	write(dgus_fd, buf, cmd_len);
	if(is_write == 0){
		nread = read(dgus_fd, buf, buf_len);
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

int dgus_init(){
	if(dgus_fd > 0){
		return 0;
	}
	dgus_fd = open(TTY_DEVICE, O_RDWR);
	if (dgus_fd < 0){
		printf("open device %s faild\n", TTY_DEVICE);
		return -1;
	}
	set_speed(dgus_fd,115200);
	set_Parity(dgus_fd,8,1,'N');
	/*
	 *	to be done
	 */
	return 0;
}

void dgus_deinit(){
	if(dgus_fd > 0){
		close(dgus_fd);
	}
}

int dgus_wait_for_info(unsigned char *buf, int len){
	int ret = 0;
	if(dgus_fd > 0 &&
		buf != NULL &&
		len > 0){
		ret = read(dgus_fd, buf, len);
	}
	return ret;
}


