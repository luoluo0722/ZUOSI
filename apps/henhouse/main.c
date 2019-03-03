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

void receivethread(void *para)
{
	int nread;
	int fd = (int)para;
	char buff[512];
	while(1)    
	{
		if((nread = read(fd,buff,100))>0) //接收数据
		{
			printf("[RECEIVE] Len is %d,content is :\n",nread);
			buff[nread]='\0';
			printf("%s\n",buff);
		}
		usleep(1000);
	} 
 
 return;
}

int send_cmd()
{
}
int send_cmd_with_ret()
{
}
int main(int argc,char **argv){
	fd_set reads, temps;
	int result, str_len;
	char buf[BUF_SIZE];
	struct timeval timeout;
	
	pthread_t receiveid;
    int  c, ctrlbits,cmd=0;
	int fd_tty = -1, fd_fpga = -1;
  

	fd_tty = open(argv[1], O_RDWR);
	if (fd_tty < 0){
		printf("open device %s faild\n", argv[1]);
		exit(0);
	}
	set_speed(fd_tty,115200); 
	set_Parity(fd_tty,8,1,'N');

	fd_fpga = open(FPGA_DEVICE, O_RDWR);
	if(fd_fpga < 0){
		printf("open %s error\n", FPGA_DEVICE);
		return 1;
	}

	pthread_create(&receiveid,NULL,(void*)receivethread,(void *)fd_tty);
	while(1){
		if((nread = read(fd_fpga,buff,100))>0) //接收数据
		{
			printf("[RECEIVE] Len is %d,content is :\n",nread);
			buff[nread]='\0';
			printf("%s\n",buff);
		}
		usleep(1000);
	}
#if 0
	/*
	超时不能在此设置！
	因为调用select后，结构体timeval的成员tv_sec和tv_usec的值将被替换为超时前剩余时间.
	调用select函数前，每次都需要初始化timeval结构体变量.
	timeout.tv_sec = 5;
	timeout.tv_usec = 5000;
	*/
	FD_ZERO(&reads);
	FD_SET(fd_tty, &reads);//监视文件描述符0的变化, 即标准输入的变化
	FD_SET(fd_fpga, &reads);//监视文件描述符0的变化, 即标准输入的变化

	while(1)
	{
		/*将准备好的fd_set变量reads的内容复制到temps变量，因为调用select函数后，除了发生变化的fd对应位外，剩下的所有位
		都将初始化为0，为了记住初始值，必须经过这种复制过程。
		*/
		temps = reads;
		//设置超时
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		//调用select函数. 若有控制台输入数据，则返回大于0的整数，如果没有输入数据而引发超时，返回0.
		result = select(1, &temps, 0, 0, NULL);
		if(result == -1)
		{
			perror("select() error");
			break;
		}
		else if(result == 0)
		{
			puts("timeout");
		}
		else
		{
			//读取数据并输出
			for(i = 0; i < fd_max + 1; i++){
				if(FD_ISSET(i, &cpy_reads)){
					/*发生状态变化时,首先验证服务器端套接字中是否有变化.
					①若是服务端套接字变化，接受连接请求。
					②若是新客户端连接，注册与客户端连接的套接字文件描述符.
					*/
					if(i == serv_sock)
					{
						adr_sz = sizeof(clnt_adr);
						clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
						FD_SET(clnt_sock, &reads);
						if(fd_max < clnt_sock)
							fd_max = clnt_sock;
						printf("connected client: %d \n", clnt_sock);
					}
					else    
					{
						str_len = read(i, buf, BUF_SIZE);
						if(str_len == 0)    //读取数据完毕关闭套接字
						{
							FD_CLR(i, &reads);//从reads中删除相关信息
							close(i);
							printf("closed client: %d \n", i);
						}
						else
						{
							write(i, buf, str_len);//执行回声服务  即echo
						}
					}
				}
			
			}
			}
	}
#endif
	return 0;

}
