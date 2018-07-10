
#include     <stdio.h> 
#include     <stdlib.h>
#include     <string.h>
#include     <unistd.h>
#include     <sys/types.h>
#include     <sys/stat.h>  
#include     <fcntl.h>  
#include     <termios.h>
#include     <errno.h> 
#include     <pthread.h> 
#include     <sys/ioctl.h> 
#define FALSE 1
#define TRUE 0


int fd=-1;

char buff[512];

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




void receivethread(void)
{
  int nread;

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

int main(int argc, char *argv[])
{
    char str[500]="peixiuhui";
    pthread_t receiveid;
    int  c, ctrlbits,cmd=0;
  
	/* 
		参数个数小于1则返回，按如下方式执行:
		./uart_test /dev/ttyAT1
	*/
	if (argc < 2) {
		printf("Useage: %s dev\n", argv[0]);
		exit(0);
    	} 
	if (argc == 3) {
		if (!strcmp(argv[2],"auto"))
			cmd = 1;
	}
	fd = open(argv[1], O_RDWR);
	if (fd < 0){
		printf("open device %s faild\n", argv[1]);
		exit(0);
	}
    
	set_speed(fd,115200); 
	
	set_Parity(fd,8,1,'N');
	
	if (!cmd) {
		pthread_create(&receiveid,NULL,(void*)receivethread,NULL);
		while(1) {
			printf("Please Input string to send to %s\n:",argv[1]);
			scanf("%s", str);
			if(strlen(str)>0){
				write(fd, str, strlen(str));  
				write(fd, "\n", strlen("\n"));
				usleep(200*1000);
			}
 		}
	}
	else {
		while(1) {
			int nread;
			printf("Send data:%s",str);
			write(fd, str, strlen(str));
			usleep(200*1000);
			printf(" <--> Receive data:");
			nread = 0;
        		if((nread = read(fd,buff,100))>0) {
				buff[nread]='\0';
				printf("%s\n",buff);
			}
   		}	
	}

	close(fd);
	exit(0);
}
