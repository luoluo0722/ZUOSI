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

#include "dgus_access.h"
#include "fpga_access.h"
#include "sqlite_access.h"


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


int main(int argc,char **argv){
	int ret = 0;
	if((ret = fpga_init()) != 0){
		printf("fpga_init error\n");
		return ret;
	}

	if((ret = dgus_init()) != 0){
		printf("dgus_init error\n");
		return ret;
	}

	if((ret = sqlite_mod_init()) != 0){
		printf("sqlite_mod_init error\n");
		return ret;
	}
	while(1){
	}
	return 0;

}
