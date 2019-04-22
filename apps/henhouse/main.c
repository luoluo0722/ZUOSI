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
#include <sys/select.h>
#include <time.h>

#include "dgus_access.h"
#include "fpga_access.h"
#include "sqlite_access.h"

static int flush_byeqinterval = 0;
static int flush_bydate = 0;
static int flush_afterdosing = 0;

#define FLUSH_DEFAULT_YEAR 2019
#define FLUSH_DEFAULT_MON 4
#define FLUSH_DEFAULT_DAY 24
#define FLUSH_DEFAULT_HOUR 24
#define FLUSH_DEFAULT_MIN 0
static int year_byeqinterval = FLUSH_DEFAULT_YEAR;
static int month_byeqinterval = FLUSH_DEFAULT_MON;
static int day_byeqinterval = FLUSH_DEFAULT_DAY;
static int hour_byeqinterval = FLUSH_DEFAULT_HOUR;
static int minute_byeqinterval = FLUSH_DEFAULT_MIN;

#define FLUSH_DEFAULT_STARTYEAR 2019
#define FLUSH_DEFAULT_STARTMON 4
#define FLUSH_DEFAULT_STOPYEAR 2038
#define FLUSH_DEFAULT_STOPMON 4
static int day_flushbydate[31] = {0};
static int day_flushbydate_index = 0;
static int startyear_flushbydate = FLUSH_DEFAULT_STARTYEAR;
static int startmon_flushbydate = FLUSH_DEFAULT_STARTMON;
static int stopyear_flushbydate = FLUSH_DEFAULT_STOPYEAR;
static int stopmon_flushbydate = FLUSH_DEFAULT_STOPMON;

#define FLUSH_DEFAULT_HOURINTERVAL 1
#define FLUSH_DEFAULT_MININTERVAL 0
static int hourinterval_afterdosing = FLUSH_DEFAULT_HOURINTERVAL;
static int mininterval_afterdosing = FLUSH_DEFAULT_HOURINTERVAL;

static int autoflush_lineselection[64] = {0};
static int manualflush_lineselection[64] = {0};

static int autoflush_min = 10;
static int autoflush_sec = 0;

static int hightempflush_min = 10;
static int hightempflush_sec = 0;

#define DOSING_DEFAULT_YEAR 2019
#define DOSING_DEFAULT_MON 4
#define DOSING_DEFAULT_DAY 24
#define DOSING_DEFAULT_HOUR 0
#define DOSING_DEFAULT_MIN 0
#define DOSING_DEFAULT_SEC 0
static int startyear_dosing = DOSING_DEFAULT_YEAR;
static int startmon_dosing = DOSING_DEFAULT_MON;
static int startday_dosing = DOSING_DEFAULT_DAY;
static int starthour_dosing = DOSING_DEFAULT_HOUR;
static int startmin_dosing = DOSING_DEFAULT_MIN;
static int startsec_dosing = DOSING_DEFAULT_SEC;

static void setTimer(int seconds){
	struct timeval temp;

	temp.tv_sec = seconds;
	temp.tv_usec = 0;

	select(0, NULL, NULL, NULL, &temp);

	return;
}

static time_t get_epoch_for_date(int year, int mon, int day){
	struct tm _tm;

	_tm.tm_sec = 0;
	_tm.tm_min = 0;
	_tm.tm_hour = 0;
	_tm.tm_mday = day;
	_tm.tm_mon = mon - 1;
	_tm.tm_year = year - 1900;
	return mktime(&_tm);
}

struct flush_byeqinterval_para{
	int interval;
	int flush_time;
	int *lineselection;
	int len;
};

static void autoflush_lineselect(){
	int i = 0;
	while(i < 64){
		if(autoflush_lineselection[i] == 1){
			fpga_flushall_ctl_oneline(1, i);/* start flush */
			setTimer(autoflush_min * 60 + autoflush_sec);
			fpga_flushall_ctl_oneline(0, i);/* stop flush */
		}
		i++;
	}
}

static pthread_t henhouse_flush_byeqinterval_thread;
static int enable_flush_byeqinterval = 0;
static pthread_mutex_t eqintervalthd_run_cond_mutex;
static pthread_cond_t eqintervalthd_run_cond;

static void henhouse_flush_byeqinterval_thread_func(void *para){
	time_t now;
	time_t future;

	pthread_mutex_lock(&eqintervalthd_run_cond_mutex);
	while(enable_flush_byeqinterval == 0){
		pthread_cond_wait(&eqintervalthd_run_cond,
			&eqintervalthd_run_cond_mutex);
	}
	pthread_mutex_unlock(&eqintervalthd_run_cond_mutex);

	now = time(NULL);
	future = get_epoch_for_date(year_byeqinterval, month_byeqinterval, day_byeqinterval);

	setTimer(future - now); /* wait for the date */

	while(1){
		autoflush_lineselect();
		/* wait for the interval */
		setTimer(hour_byeqinterval * 3600 + minute_byeqinterval * 60);
	}


}

static void henhouse_flush_byeqinterval_init(){
	pthread_mutex_init(&eqintervalthd_run_cond_mutex, NULL);
	pthread_cond_init(&eqintervalthd_run_cond, NULL);
}

static pthread_t henhouse_flush_bydate_thread;
static int enable_flush_bydate = 0;
static pthread_mutex_t bydatethd_run_cond_mutex;
static pthread_cond_t bydatethd_run_cond;

static void henhouse_flush_bydate_thread_func(void *para){
	time_t now;
	time_t future;
	int year = startyear_flushbydate;
	int mon = startmon_flushbydate;

	pthread_mutex_lock(&bydatethd_run_cond_mutex);
	while(enable_flush_bydate == 0){
		pthread_cond_wait(&bydatethd_run_cond,
			&bydatethd_run_cond_mutex);
	}
	pthread_mutex_unlock(&bydatethd_run_cond_mutex);


	while(1){
		int i = 0;
		while(i < 31){
			if(day_flushbydate[i] == 1){
				now = time(NULL);
				future = get_epoch_for_date(year, mon, i + 1);
				setTimer(future - now);
				autoflush_lineselect();
			}
			i++;
		}
		mon++;
		if(mon > 12){
			mon = 1;
			year++;
		}
		if(year == stopyear_flushbydate && stopmon_flushbydate){
			break;
		}
		
	}


}

static void henhouse_flush_bydate_init(){
	pthread_mutex_init(&bydatethd_run_cond_mutex, NULL);
	pthread_cond_init(&bydatethd_run_cond, NULL);
}

static void henhouse_page06_press_onekeyflushctl(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	//fpga_flushall_ctl(key);
	//henhouse_flush_operate(autoflush_min, autoflush_sec);
	if(flush_byeqinterval == 1){
		if(key == 0x1){
			pthread_create(&henhouse_flush_byeqinterval_thread,NULL,(void*)henhouse_flush_byeqinterval_thread_func, (void *)NULL);
			pthread_mutex_lock(&eqintervalthd_run_cond_mutex);
			enable_flush_byeqinterval = 1;
			pthread_cond_broadcast(&eqintervalthd_run_cond);
			pthread_mutex_unlock(&eqintervalthd_run_cond_mutex);
		}else{
			pthread_mutex_lock(&eqintervalthd_run_cond_mutex);
			enable_flush_byeqinterval = 0;
			pthread_cond_broadcast(&eqintervalthd_run_cond);
			pthread_mutex_unlock(&eqintervalthd_run_cond_mutex);
			pthread_cancel(henhouse_flush_byeqinterval_thread);
		}
	}else if (flush_bydate == 1){
		if(key == 0x1){
			pthread_create(&henhouse_flush_bydate_thread,NULL,(void*)henhouse_flush_bydate_thread_func, (void *)NULL);
			pthread_mutex_lock(&bydatethd_run_cond_mutex);
			enable_flush_bydate = 1;
			pthread_cond_broadcast(&bydatethd_run_cond);
			pthread_mutex_unlock(&bydatethd_run_cond_mutex);
		}else{
			pthread_mutex_lock(&bydatethd_run_cond_mutex);
			enable_flush_bydate = 0;
			pthread_cond_broadcast(&bydatethd_run_cond);
			pthread_mutex_unlock(&bydatethd_run_cond_mutex);
			pthread_cancel(henhouse_flush_bydate_thread);
		}

	}
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
		printf("%d, %d, %d, %d, %d\n", year_byeqinterval, month_byeqinterval,
			day_byeqinterval,hour_byeqinterval, minute_byeqinterval);

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
	day_flushbydate[key - 1] = 0;
}

void henhouse_page08_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		startyear_flushbydate = data_buf[0];
		startmon_flushbydate = data_buf[1];
		stopyear_flushbydate = data_buf[3];
		stopmon_flushbydate = data_buf[4];
	}else{
		int i = 0;
		data_buf[0] = startyear_flushbydate = FLUSH_DEFAULT_STARTYEAR;
		data_buf[1] = startmon_flushbydate = FLUSH_DEFAULT_STARTMON;
		data_buf[3] = stopyear_flushbydate = FLUSH_DEFAULT_STOPYEAR;
		data_buf[4] = stopmon_flushbydate = FLUSH_DEFAULT_STOPMON;
		*len = 4;
		while(i < 31){
			day_flushbydate[i++] = 0;
		}
		
	}
}

void henhouse_page09_press_confirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		hourinterval_afterdosing = data_buf[0];
		mininterval_afterdosing = data_buf[1];
	}else{
		data_buf[0] = hourinterval_afterdosing = FLUSH_DEFAULT_HOURINTERVAL;
		data_buf[1] = mininterval_afterdosing = FLUSH_DEFAULT_MININTERVAL;
		*len = 2;
	}
}

void henhouse_page10_11_press_lineseletion(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	int index = key_addr_offset - 0x19;

	autoflush_lineselection[index] = key;
}

void henhouse_page12_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		autoflush_min = data_buf[0];
		autoflush_sec = data_buf[1];
	}else{
		data_buf[0] = autoflush_min = 10;
		data_buf[1] = autoflush_sec = 0;
		*len = 2;
	}
}

void henhouse_page13_14_press_lineseletion(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	int index = key_addr_offset - 0x5A;

	manualflush_lineselection[index] = key;
}

void henhouse_page15_press_confirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	/* start auto flush*/
}

void henhouse_page17_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(len == NULL){ /* confirm set */
		hightempflush_min = data_buf[0];
		hightempflush_sec = data_buf[1];
	}else{
		data_buf[0] = hightempflush_min = 10;
		data_buf[1] = hightempflush_sec = 0;
		*len = 2;
	}
}

struct dosing_para{
	int interval;
	int dosing_time;
};

static struct dosing_para para_dose = {0};
static pthread_t henhouse_dosing_thread;
static void henhouse_dosing_thread_func(void *para){


}


void henhouse_page18_press_confirmorresetorstop(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	if(key == 0x1){ /* confirm */
		startyear_dosing = data_buf[0];
		startmon_dosing = data_buf[1];
		startday_dosing = data_buf[2];
		starthour_dosing = data_buf[3];
		startmin_dosing = data_buf[4];
		startsec_dosing = data_buf[5];
	}else if(key == 0x2){ /* reset */
		startyear_dosing = data_buf[0] = DOSING_DEFAULT_YEAR;
		startmon_dosing = data_buf[1] = DOSING_DEFAULT_MON;
		startday_dosing = data_buf[2] = DOSING_DEFAULT_DAY;
		starthour_dosing = data_buf[3] = DOSING_DEFAULT_HOUR;
		startmin_dosing = data_buf[4] = DOSING_DEFAULT_MIN;
		startsec_dosing = data_buf[5] = DOSING_DEFAULT_SEC;
	}else if(key == 0x3){ /* stop */
		/*stop dosing */
	}
}
void henhouse_page18_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}
void henhouse_page19_20_press_setzeroorconfirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}
void henhouse_page21_22_press_lineselect(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page23_press_cancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}
void henhouse_page24_press_confirmorresetorgenorcal(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page25_press_confirmorresetorgenorcal(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page26_press_confirmorresetorgen(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page27_29_31_33_press_daypos(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page36_press_nextorprev(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}
void henhouse_page43_press_upgrade(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

void henhouse_page_change(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
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

	{9, henhouse_page10_11_press_lineseletion},
	{10, henhouse_page10_11_press_lineseletion},
	{11, henhouse_page10_11_press_lineseletion},
	{12, henhouse_page10_11_press_lineseletion},
	{13, henhouse_page10_11_press_lineseletion},
	{14, henhouse_page10_11_press_lineseletion},
	{15, henhouse_page10_11_press_lineseletion},
	{16, henhouse_page10_11_press_lineseletion},
	{17, henhouse_page10_11_press_lineseletion},
	{18, henhouse_page10_11_press_lineseletion},
	{19, henhouse_page10_11_press_lineseletion},
	{20, henhouse_page10_11_press_lineseletion},
	{21, henhouse_page10_11_press_lineseletion},
	{22, henhouse_page10_11_press_lineseletion},
	{23, henhouse_page10_11_press_lineseletion},
	{24, henhouse_page10_11_press_lineseletion},
	{25, henhouse_page10_11_press_lineseletion},
	{26, henhouse_page10_11_press_lineseletion},
	{27, henhouse_page10_11_press_lineseletion},
	{28, henhouse_page10_11_press_lineseletion},
	{29, henhouse_page10_11_press_lineseletion},
	{30, henhouse_page10_11_press_lineseletion},
	{31, henhouse_page10_11_press_lineseletion},
	{32, henhouse_page10_11_press_lineseletion},
	{33, henhouse_page10_11_press_lineseletion},
	{34, henhouse_page10_11_press_lineseletion},
	{35, henhouse_page10_11_press_lineseletion},
	{36, henhouse_page10_11_press_lineseletion},
	{37, henhouse_page10_11_press_lineseletion},
	{38, henhouse_page10_11_press_lineseletion},
	{39, henhouse_page10_11_press_lineseletion},
	{40, henhouse_page10_11_press_lineseletion},

	{41, henhouse_page10_11_press_lineseletion},
	{42, henhouse_page10_11_press_lineseletion},
	{43, henhouse_page10_11_press_lineseletion},
	{44, henhouse_page10_11_press_lineseletion},
	{45, henhouse_page10_11_press_lineseletion},
	{46, henhouse_page10_11_press_lineseletion},
	{47, henhouse_page10_11_press_lineseletion},
	{48, henhouse_page10_11_press_lineseletion},
	{49, henhouse_page10_11_press_lineseletion},
	{50, henhouse_page10_11_press_lineseletion},
	{51, henhouse_page10_11_press_lineseletion},
	{52, henhouse_page10_11_press_lineseletion},
	{53, henhouse_page10_11_press_lineseletion},
	{54, henhouse_page10_11_press_lineseletion},
	{55, henhouse_page10_11_press_lineseletion},
	{56, henhouse_page10_11_press_lineseletion},
	{57, henhouse_page10_11_press_lineseletion},
	{58, henhouse_page10_11_press_lineseletion},
	{59, henhouse_page10_11_press_lineseletion},
	{60, henhouse_page10_11_press_lineseletion},
	{61, henhouse_page10_11_press_lineseletion},
	{62, henhouse_page10_11_press_lineseletion},
	{63, henhouse_page10_11_press_lineseletion},
	{64, henhouse_page10_11_press_lineseletion},
	{65, henhouse_page10_11_press_lineseletion},
	{66, henhouse_page10_11_press_lineseletion},
	{67, henhouse_page10_11_press_lineseletion},
	{68, henhouse_page10_11_press_lineseletion},
	{69, henhouse_page10_11_press_lineseletion},
	{70, henhouse_page10_11_press_lineseletion},
	{71, henhouse_page10_11_press_lineseletion},
	{72, henhouse_page10_11_press_lineseletion},

	{73, henhouse_page12_press_confirmorreset},

	{74, henhouse_page13_14_press_lineseletion},
	{75, henhouse_page13_14_press_lineseletion},
	{76, henhouse_page13_14_press_lineseletion},
	{77, henhouse_page13_14_press_lineseletion},
	{78, henhouse_page13_14_press_lineseletion},
	{79, henhouse_page13_14_press_lineseletion},
	{80, henhouse_page13_14_press_lineseletion},
	{81, henhouse_page13_14_press_lineseletion},
	{82, henhouse_page13_14_press_lineseletion},
	{83, henhouse_page13_14_press_lineseletion},
	{84, henhouse_page13_14_press_lineseletion},
	{85, henhouse_page13_14_press_lineseletion},
	{86, henhouse_page13_14_press_lineseletion},
	{87, henhouse_page13_14_press_lineseletion},
	{88, henhouse_page13_14_press_lineseletion},
	{89, henhouse_page13_14_press_lineseletion},
	{90, henhouse_page13_14_press_lineseletion},
	{91, henhouse_page13_14_press_lineseletion},
	{92, henhouse_page13_14_press_lineseletion},
	{93, henhouse_page13_14_press_lineseletion},
	{94, henhouse_page13_14_press_lineseletion},
	{95, henhouse_page13_14_press_lineseletion},
	{96, henhouse_page13_14_press_lineseletion},
	{97, henhouse_page13_14_press_lineseletion},
	{98, henhouse_page13_14_press_lineseletion},
	{99, henhouse_page13_14_press_lineseletion},
	{100, henhouse_page13_14_press_lineseletion},
	{101, henhouse_page13_14_press_lineseletion},
	{102, henhouse_page13_14_press_lineseletion},
	{103, henhouse_page13_14_press_lineseletion},
	{104, henhouse_page13_14_press_lineseletion},
	{105, henhouse_page13_14_press_lineseletion},

	{106, henhouse_page13_14_press_lineseletion},
	{107, henhouse_page13_14_press_lineseletion},
	{108, henhouse_page13_14_press_lineseletion},
	{109, henhouse_page13_14_press_lineseletion},
	{110, henhouse_page13_14_press_lineseletion},
	{111, henhouse_page13_14_press_lineseletion},
	{112, henhouse_page13_14_press_lineseletion},
	{113, henhouse_page13_14_press_lineseletion},
	{114, henhouse_page13_14_press_lineseletion},
	{115, henhouse_page13_14_press_lineseletion},
	{116, henhouse_page13_14_press_lineseletion},
	{117, henhouse_page13_14_press_lineseletion},
	{118, henhouse_page13_14_press_lineseletion},
	{119, henhouse_page13_14_press_lineseletion},
	{120, henhouse_page13_14_press_lineseletion},
	{121, henhouse_page13_14_press_lineseletion},
	{122, henhouse_page13_14_press_lineseletion},
	{123, henhouse_page13_14_press_lineseletion},
	{124, henhouse_page13_14_press_lineseletion},
	{125, henhouse_page13_14_press_lineseletion},
	{126, henhouse_page13_14_press_lineseletion},
	{127, henhouse_page13_14_press_lineseletion},
	{128, henhouse_page13_14_press_lineseletion},
	{129, henhouse_page13_14_press_lineseletion},
	{130, henhouse_page13_14_press_lineseletion},
	{131, henhouse_page13_14_press_lineseletion},
	{132, henhouse_page13_14_press_lineseletion},
	{133, henhouse_page13_14_press_lineseletion},
	{134, henhouse_page13_14_press_lineseletion},
	{135, henhouse_page13_14_press_lineseletion},
	{136, henhouse_page13_14_press_lineseletion},
	{137, henhouse_page13_14_press_lineseletion},

	{138, henhouse_page15_press_confirmorcancel},

	{139, henhouse_page17_press_confirmorreset},

	{140, henhouse_page18_press_confirmorresetorstop},
	{141, henhouse_page18_press_confirmorreset},

	{142, henhouse_page19_20_press_setzeroorconfirmorcancel},

	{143, henhouse_page21_22_press_lineselect},
	{144, henhouse_page21_22_press_lineselect},
	{145, henhouse_page21_22_press_lineselect},
	{146, henhouse_page21_22_press_lineselect},
	{147, henhouse_page21_22_press_lineselect},
	{148, henhouse_page21_22_press_lineselect},
	{149, henhouse_page21_22_press_lineselect},
	{150, henhouse_page21_22_press_lineselect},
	{151, henhouse_page21_22_press_lineselect},
	{152, henhouse_page21_22_press_lineselect},
	{153, henhouse_page21_22_press_lineselect},
	{154, henhouse_page21_22_press_lineselect},
	{155, henhouse_page21_22_press_lineselect},
	{156, henhouse_page21_22_press_lineselect},
	{157, henhouse_page21_22_press_lineselect},
	{158, henhouse_page21_22_press_lineselect},
	{159, henhouse_page21_22_press_lineselect},
	{160, henhouse_page21_22_press_lineselect},
	{161, henhouse_page21_22_press_lineselect},
	{162, henhouse_page21_22_press_lineselect},
	{163, henhouse_page21_22_press_lineselect},
	{164, henhouse_page21_22_press_lineselect},
	{165, henhouse_page21_22_press_lineselect},
	{166, henhouse_page21_22_press_lineselect},
	{167, henhouse_page21_22_press_lineselect},
	{168, henhouse_page21_22_press_lineselect},
	{169, henhouse_page21_22_press_lineselect},
	{170, henhouse_page21_22_press_lineselect},
	{171, henhouse_page21_22_press_lineselect},
	{172, henhouse_page21_22_press_lineselect},
	{173, henhouse_page21_22_press_lineselect},
	{174, henhouse_page21_22_press_lineselect},

	{175, henhouse_page21_22_press_lineselect},
	{176, henhouse_page21_22_press_lineselect},
	{177, henhouse_page21_22_press_lineselect},
	{178, henhouse_page21_22_press_lineselect},
	{179, henhouse_page21_22_press_lineselect},
	{180, henhouse_page21_22_press_lineselect},
	{181, henhouse_page21_22_press_lineselect},
	{182, henhouse_page21_22_press_lineselect},
	{183, henhouse_page21_22_press_lineselect},
	{184, henhouse_page21_22_press_lineselect},
	{185, henhouse_page21_22_press_lineselect},
	{186, henhouse_page21_22_press_lineselect},
	{187, henhouse_page21_22_press_lineselect},
	{188, henhouse_page21_22_press_lineselect},
	{189, henhouse_page21_22_press_lineselect},
	{190, henhouse_page21_22_press_lineselect},
	{191, henhouse_page21_22_press_lineselect},
	{192, henhouse_page21_22_press_lineselect},
	{193, henhouse_page21_22_press_lineselect},
	{194, henhouse_page21_22_press_lineselect},
	{195, henhouse_page21_22_press_lineselect},
	{196, henhouse_page21_22_press_lineselect},
	{197, henhouse_page21_22_press_lineselect},
	{198, henhouse_page21_22_press_lineselect},
	{199, henhouse_page21_22_press_lineselect},
	{200, henhouse_page21_22_press_lineselect},
	{201, henhouse_page21_22_press_lineselect},
	{202, henhouse_page21_22_press_lineselect},
	{203, henhouse_page21_22_press_lineselect},
	{204, henhouse_page21_22_press_lineselect},
	{205, henhouse_page21_22_press_lineselect},
	{206, henhouse_page21_22_press_lineselect},

	{207, henhouse_page23_press_cancel},

	{208, henhouse_page24_press_confirmorresetorgenorcal},

	{209, henhouse_page25_press_confirmorresetorgenorcal},

	{210, henhouse_page26_press_confirmorresetorgen},

	{211, henhouse_page27_29_31_33_press_daypos},
	{212, henhouse_page27_29_31_33_press_daypos},
	{213, henhouse_page27_29_31_33_press_daypos},
	{214, henhouse_page27_29_31_33_press_daypos},

	{215, henhouse_page36_press_nextorprev},

	{216, NULL},

	{217, henhouse_page43_press_upgrade},

	{218, NULL},
	{219, NULL},
	{220, NULL},
	{221, NULL},
	{222, NULL},
	{223, henhouse_page_change},

	{0xffff, NULL},
};

static void henhouse_page05_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	*len = 4;
}

static void henhouse_page09_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	data_buf[0] = hourinterval_afterdosing;
	data_buf[1] = mininterval_afterdosing;
	*len = 2;
}

static void henhouse_page12_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	data_buf[0] = autoflush_min;
	data_buf[1] = autoflush_sec;
	*len = 2;
}

static void henhouse_page16_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	data_buf[0] = hightempflush_min;
	data_buf[1] = hightempflush_sec;
	*len = 2;
}

static void henhouse_page17_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	data_buf[0] = hightempflush_min;
	data_buf[1] = hightempflush_sec;
	*len = 2;
}

static void henhouse_page18_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	data_buf[0] = startyear_dosing;
	data_buf[1] = startmon_dosing;
	data_buf[2] = startday_dosing;
	data_buf[3] = starthour_dosing;
	data_buf[4] = startmin_dosing;
	data_buf[5] = startsec_dosing;
	*len = 6;
}

static struct dgus_page_callback main_page_callback_array[] = {
	{0, NULL},
	{1, NULL},
	{2, NULL},
	{3, NULL},
	{4, NULL},
	{5, henhouse_page05_display},
	{6, NULL},
	{7, NULL},
	{8, henhouse_page09_display},
	{9, NULL},
	{10, NULL},
	{11, NULL},
	{12, henhouse_page12_display},
	{13, NULL},
	{14, NULL},
	{15, NULL},
	{16, henhouse_page16_display},
	{17, henhouse_page17_display},
	{18, henhouse_page18_display},
	{19, NULL},
	{20, NULL},
	{21, NULL},
	{22, NULL},
	{23, NULL},
	{24, NULL},
	{25, NULL},
	{26, NULL},
	{27, NULL},
	{28, NULL},
	{29, NULL},
	{30, NULL},
	{31, NULL},
	{32, NULL},
	{33, NULL},
	{34, NULL},

	{0xffff, NULL},
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
		/* do not exit */
		//return ret;
	}

	if((ret = dgus_init(argv[1], &main_callback)) != 0){
		printf("dgus_init error\n");
		return ret;
	}
	henhouse_flush_byeqinterval_init();
	henhouse_flush_bydate_init();
	while(1){
	}
	return 0;

}
