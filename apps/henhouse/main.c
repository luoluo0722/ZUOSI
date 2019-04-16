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

static int flush_byeqinterval = 0;
static int flush_bydate = 0;
static int flush_afterdosing = 0;

#define FLUSH_DEFAULT_YEAR 2000
#define FLUSH_DEFAULT_MON 1
#define FLUSH_DEFAULT_DAY 1
#define FLUSH_DEFAULT_HOUR 24
#define FLUSH_DEFAULT_MIN 0
static int year_byeqinterval = FLUSH_DEFAULT_YEAR;
static int month_byeqinterval = FLUSH_DEFAULT_MON;
static int day_byeqinterval = FLUSH_DEFAULT_DAY;
static int hour_byeqinterval = FLUSH_DEFAULT_HOUR;
static int minute_byeqinterval = FLUSH_DEFAULT_MIN;

#define FLUSH_DEFAULT_STARTHOUR 0
#define FLUSH_DEFAULT_STARTMIN 0
static int day_flushbydate[31] = {0};
static int day_flushbydate_index = 0;
static int starthour_flushbydate = FLUSH_DEFAULT_STARTHOUR;
static int startmin_flushbydate = FLUSH_DEFAULT_STARTMIN;

#define FLUSH_DEFAULT_HOURINTERVAL 1
#define FLUSH_DEFAULT_MININTERVAL 0
static int hourinterval_afterdosing = FLUSH_DEFAULT_HOURINTERVAL;
static int mininterval_afterdosing = FLUSH_DEFAULT_HOURINTERVAL;

static void henhouse_page06_press_onekeyflushctl(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	fpga_flushall_ctl(key);
}

static void henhouse_page06_press_byeqinterval(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	flush_byeqinterval = key;
}
static void henhouse_page06_press_bydate(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	flush_bydate = key;
}
static void henhouse_page06_press_afterdosing(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	flush_afterdosing = key;
}

void henhouse_page07_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		year_byeqinterval = data_buf[0];
		month_byeqinterval = data_buf[1];
		day_byeqinterval = data_buf[2];
		hour_byeqinterval = data_buf[3];
		minute_byeqinterval = data_buf[4];
	}else{
		data_buf[0] = year_byeqinterval = FLUSH_DEFAULT_YEAR;
		data_buf[1] = month_byeqinterval = FLUSH_DEFAULT_MON;
		data_buf[2] = day_byeqinterval = FLUSH_DEFAULT_DAY;
		data_buf[3] = hour_byeqinterval = FLUSH_DEFAULT_HOUR;
		data_buf[4] = minute_byeqinterval = FLUSH_DEFAULT_MIN;
		*len = 5;
	}
}
void henhouse_page08_press_numkey(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	day_flushbydate[day_flushbydate_index++] = key;
}

void henhouse_page08_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		starthour_flushbydate = data_buf[0];
		startmin_flushbydate = data_buf[1];
	}else{
		data_buf[0] = starthour_flushbydate = FLUSH_DEFAULT_HOURINTERVAL;
		data_buf[1] = startmin_flushbydate = FLUSH_DEFAULT_MININTERVAL;
		*len = 2;
		while(--day_flushbydate_index >= 0){
			day_flushbydate[day_flushbydate_index] = 0;
		}
		day_flushbydate_index = 0;
		
	}
}

void henhouse_page09_press_confirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		hourinterval_afterdosing = data_buf[0];
		mininterval_afterdosing = data_buf[1];
	}else{
		data_buf[0] = hourinterval_afterdosing = FLUSH_DEFAULT_STARTHOUR;
		data_buf[1] = mininterval_afterdosing = FLUSH_DEFAULT_STARTMIN;
		*len = 2;
	}
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

static struct dgus_keypress_callback main_keypress_callback_array[] = {
	{0, NULL},

	{1, henhouse_page06_press_onekeyflushctl},
	{2, henhouse_page06_press_byeqinterval},
	{3, henhouse_page06_press_bydate},
	{4, henhouse_page06_press_afterdosing},

	{5, henhouse_page07_press_confirmorreset},

	{6, henhouse_page08_press_numkey},
	{7, henhouse_page08_press_confirmorreset},

	{8, henhouse_page09_press_confirmorcancel},

	{9, NULL},
};

static struct dgus_page_callback main_page_callback_array[] = {
#if 0
	{0x0, NULL},
	{0x1, henhouse_page6_onekey_flush_ctl},
	{0x2, henhouse_page6_set_flush_by_eq_interval},
	{0xffff, NULL},
#endif
};

static struct dgus_callback main_callback = {
	.keypress_callback = main_keypress_callback_array,
	.page_callback = main_page_callback_array,
};




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
