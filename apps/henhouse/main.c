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

void henhouse_onekey_flush_ctl();

struct dgus_keypress_callback main_keypress_callback_array[] = {
#if 0
	{0x0, NULL},
	{0x1, henhouse_page6_onekey_flush_ctl},
	{0x2, henhouse_page6_set_flush_by_eq_interval},
	{0xffff, NULL},
#endif
};

struct dgus_page_callback main_page_callback_array[] = {
#if 0
	{0x0, NULL},
	{0x1, henhouse_page6_onekey_flush_ctl},
	{0x2, henhouse_page6_set_flush_by_eq_interval},
	{0xffff, NULL},
#endif
};

struct dgus_callback main_callback = {
	.keypress_callback = main_keypress_callback_array,
	.page_callback = main_page_callback_array,
};


void henhouse_page06_onekey_flush_ctl(void *param){
}
void henhouse_page06_set_flush_by_eq_interval(void *param){
}
void henhouse_page06_set_flush_by_date(void *param){
}
void henhouse_page06_set_flush_after_dosing(void *param){
}

void henhouse_page07_press_confirmorcancel(void *param){
}
void henhouse_page08_press_numkey(void *param){
}

void henhouse_page08_press_confirmorcancel(void *param){
}

void henhouse_page09_press_confirmorcancel(void *param){
}

void henhouse_page10_11_press_lineseletion(void *param){
}
void henhouse_page12_press_confirmorcancel(void *param){
}

void henhouse_page13_14_press_lineseletion(void *param){
}

void henhouse_page15_press_confirmorcancel(void *param){
}

void henhouse_page17_press_confirmorcancel(void *param){
}

void henhouse_page18_press_confirmorresetorstop1(void *param){
}
void henhouse_page18_press_confirmorreset2(void *param){
}
void henhouse_page19_20_press_resetzeroorconfirmorcancel(void *param){
}
void henhouse_page21_22_press_lineseletion(void *param){
}

void henhouse_page23_press_cancel(void *param){
}
void henhouse_page24_press_confirmorresetorgenorcal(void *param){
}

void henhouse_page25_press_confirmorresetorgenorcal(void *param){
}

void henhouse_page26_press_confirmorresetorgen(void *param){
}

void henhouse_page27_press_day(void *param){
}

void henhouse_page29_press_day(void *param){
}
void henhouse_page31_press_day(void *param){
}
void henhouse_page33_press_day(void *param){
}

void henhouse_page36_press_nextorprev(void *param){
}
void henhouse_page43_upgrade(void *param){
}

void henhouse_page_change(void *param){
}

int main(int argc,char **argv){
	int ret = 0;
	if((ret = fpga_init()) != 0){
		printf("fpga_init error\n");
		return ret;
	}

	if((ret = sqlite_mod_init()) != 0){
		printf("sqlite_mod_init error\n");
		return ret;
	}

	if((ret = dgus_init(&main_callback)) != 0){
		printf("dgus_init error\n");
		return ret;
	}
	while(1){
	}
	return 0;

}
