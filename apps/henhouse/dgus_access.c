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
#include <pthread.h>
#include <time.h>

#include "dgus_access.h"

#define TTY_DEVICE "/dev/ttyS1"

#define FALSE 1
#define TRUE 0
#define HEADER_BYTE1 0x5a
#define HEADER_BYTE2 0xa5

#define DATA_BUF_LEN 256

#define REG_ADDRES_VERSION 0x0

#define VAR_ADDRES_REPORT_BASE       VAR_ADDRES_PAGE05_SETZERO
#define VAR_ADDRES_PAGE05_SETZERO                       0x0010

#define VAR_ADDRES_PAGE_ONEKEYFLUSHCONTROL              0x0011

#define VAR_ADDRES_PAGE06_FLUSHBYEQINTERVAL             0x0012
#define VAR_ADDRES_PAGE06_FLUSHBYDATE                   0x0013
#define VAR_ADDRES_PAGE06_FLUSHAFTERDOSING              0x0014

#define VAR_ADDRES_PAGE07_CONFIRMORRESET                0x0015

#define VAR_ADDRES_PAGE08_NUMKEY                        0x0016
#define VAR_ADDRES_PAGE08_CONFIRMORRESET                0x0017

#define VAR_ADDRES_PAGE09_CONFIRMORRESET                0x0018

#define VAR_ADDRES_PAGE10_LINESELECTION1_BASE           0x0019
#define VAR_ADDRES_PAGE11_LINESELECTION2_BASE           0x0039

#define VAR_ADDRES_PAGE12_CONFIRMORRESET                0x0059

#define VAR_ADDRES_PAGE13_LINESELECTION1_BASE           0x005A
#define VAR_ADDRES_PAGE14_LINESELECTION2_BASE           0x007A

#define VAR_ADDRES_PAGE15_CONFIRMORRESET                0x009A

#define VAR_ADDRES_PAGE17_CONFIRMORRESET                0x009B

#define VAR_ADDRES_PAGE18_CONFIRMORRESETORSTOP1         0x009C
#define VAR_ADDRES_PAGE18_CONFIRMORRESET2               0x009D

#define VAR_ADDRES_PAGE19_20_RESETZEROORCONFIRMORCANCEL 0x009E

#define VAR_ADDRES_PAGE21_LINESELECTION1_BASE           0x009F
#define VAR_ADDRES_PAGE22_LINESELECTION2_BASE           0x00BF

#define VAR_ADDRES_PAGE23_CANCEL                        0x00DF

#define VAR_ADDRES_PAGE24_CONFIRMORRESETORGENORCAL      0x00E0

#define VAR_ADDRES_PAGE25_CONFIRMORRESETORGENORCAL      0x00E1

#define VAR_ADDRES_PAGE26_CONFIRMORRESETORGEN           0x00E2

#define VAR_ADDRES_PAGE27_DAY                           0x00E3

#define VAR_ADDRES_PAGE29_DAY                           0x00E4

#define VAR_ADDRES_PAGE31_DAY                           0x00E5

#define VAR_ADDRES_PAGE33_DAY                           0x00E6

#define VAR_ADDRES_PAGE36_NEXTORPREV                    0x00E7

#define VAR_ADDRES_PAGE43_UPGRADE                       0x00E9

#define VAR_ADDRES_PAGE_CHANGE_NO                       0x00EF



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
static unsigned char  buf_byte[DATA_BUF_LEN];
static unsigned short buf_word[DATA_BUF_LEN/2];
static unsigned short callback_buf_word[DATA_BUF_LEN/2];
static pthread_t dgus_thread;
static int speed_arr[] = {  B115200, B57600, B38400, B19200, B9600, B4800,
		    B2400, B1200};
static int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};
static unsigned short current_page = -1;
static struct dgus_callback internal_callback;

unsigned short date_eqinterval[5] = {0};

static int dgus_access_address(unsigned short addr, int is_write,
		unsigned short *data, unsigned short len);


/*
 * main callback table
 */
static struct dgus_callback *p_main_callback = NULL;

static void dgus_page05_presss_resetzero(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page06_press_onekeyflushctl(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, NULL, 0, NULL);
}
static void dgus_page06_press_byeqinterval(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, NULL, 0, NULL);
}
static void dgus_page06_press_bydate(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, NULL, 0, NULL);
}

static void dgus_page06_press_afterdosing(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, NULL, 0, NULL);
}

static void dgus_page07_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x1008, 0, callback_buf_word, 5);
		//memcpy(callback_buf_word, date_eqinterval, sizeof(date_eqinterval));
		
		printf("%d, %d, %d, %d, %d\n", callback_buf_word[0], callback_buf_word[1],
				callback_buf_word[2],callback_buf_word[3], callback_buf_word[4]);
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 5, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 5, &ret_len);
		printf("%d, %d, %d, %d, %d\n", callback_buf_word[0], callback_buf_word[1],
				callback_buf_word[2],callback_buf_word[3], callback_buf_word[4]);
		dgus_access_address(0x1008, 1, callback_buf_word, 5);
	}
}

static void dgus_page08_press_numkey(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, NULL, 0, NULL);
}
static void dgus_page08_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x100d, 0, callback_buf_word, 2);
		dgus_access_address(0x102f, 0, callback_buf_word + 2, 2);
		dgus_access_address(0x0240, 0, callback_buf_word + 4, 31);
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 35, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 2, &ret_len);
		dgus_access_address(0x100d, 1, callback_buf_word, 2);
	}
}

static void dgus_page09_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x100f, 0, callback_buf_word, 2);
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 2, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		p_main_callback->keypress_callback[key_addr_offset].callback(key_addr_offset, key, callback_buf_word, 2, &ret_len);
		dgus_access_address(0x100f, 1, callback_buf_word, 2);
	}
}

static void dgus_page10_11_press_lineselect(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;
	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;

	if(report_callback != NULL){
		report_callback(key_addr_offset, key, NULL, 0, NULL);
	}
}

static void dgus_page12_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;

	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;
	if(report_callback == NULL){
		return;
	}

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x1011, 0, callback_buf_word, 2);
		report_callback(key_addr_offset, key, callback_buf_word, 2, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		report_callback(key_addr_offset, key, callback_buf_word, 2, &ret_len);
		dgus_access_address(0x1011, 1, callback_buf_word, 2);
	}
}

static void dgus_page13_14_press_lineselect(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;
	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;

	if(report_callback != NULL){
		report_callback(key_addr_offset, key, NULL, 0, NULL);
	}
}

static void dgus_page15_press_confirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;

	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;
	if(report_callback == NULL){
		return;
	}
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x1013, 0, callback_buf_word, 2);
		report_callback(key_addr_offset, key, callback_buf_word, 2, NULL);
	}else if(key == 0x2){ /* cancel */
		report_callback(key_addr_offset, key, NULL, 0, NULL);
	}
}

static void dgus_page17_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;

	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;
	if(report_callback == NULL){
		return;
	}

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x1015, 0, callback_buf_word, 2);
		report_callback(key_addr_offset, key, callback_buf_word, 2, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		report_callback(key_addr_offset, key, callback_buf_word, 2, &ret_len);
		dgus_access_address(0x1016, 1, callback_buf_word, 2);
	}
}

static void dgus_page18_press_confirmorresetorstop(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	DGUS_REPORT_CALLBACK report_callback;

	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;
	if(report_callback == NULL){
		return;
	}

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	if(key == 0x1){ /* confirm */
		dgus_access_address(0x1017, 0, callback_buf_word, 6);
		report_callback(key_addr_offset, key, callback_buf_word, 6, NULL);
	}else if(key == 0x2){ /* reset */
		int ret_len;
		report_callback(key_addr_offset, key, callback_buf_word, 6, &ret_len);
		dgus_access_address(0x1017, 1, callback_buf_word, 6);
	}else if(key == 0x3){ /* stop */
		report_callback(key_addr_offset, key, NULL, 0, NULL);
	}
}
static void dgus_page18_press_confirmorreset(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page19_20_press_setzeroorconfirmorcancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page21_22_press_lineselect(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}


static void dgus_page23_press_cancel(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page24_press_confirmorresetorgenorcal(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page25_press_confirmorresetorgenorcal(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page26_press_confirmorresetorgen(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page27_29_31_33_press_daypos(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page36_press_nextorprev(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page43_press_upgrade(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
}

static void dgus_page_change(unsigned short key_addr_offset, unsigned short key, 
	unsigned short *data_buf, int buf_len, int *len){
	current_page = key;
	DGUS_REPORT_CALLBACK report_callback;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	/* main call back */
	report_callback = p_main_callback->keypress_callback[key_addr_offset].callback;
	if(report_callback != NULL){
		report_callback(key_addr_offset, key, NULL, 0, NULL);
	}

	/* page call back */
	pagechange_callback = internal_callback.page_callback[current_page].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(current_page, NULL, 0, NULL);
	}
}

static struct dgus_keypress_callback internal_keypress_callback_array[] = {
	{0, dgus_page05_presss_resetzero},

	{1, dgus_page06_press_onekeyflushctl},
	{2, dgus_page06_press_byeqinterval},
	{3, dgus_page06_press_bydate},
	{4, dgus_page06_press_afterdosing},

	{5, dgus_page07_press_confirmorreset},

	{6, dgus_page08_press_numkey},
	{7, dgus_page08_press_confirmorreset},

	{8, dgus_page09_press_confirmorreset},

	{9, dgus_page10_11_press_lineselect},
	{10, dgus_page10_11_press_lineselect},
	{11, dgus_page10_11_press_lineselect},
	{12, dgus_page10_11_press_lineselect},
	{13, dgus_page10_11_press_lineselect},
	{14, dgus_page10_11_press_lineselect},
	{15, dgus_page10_11_press_lineselect},
	{16, dgus_page10_11_press_lineselect},
	{17, dgus_page10_11_press_lineselect},
	{18, dgus_page10_11_press_lineselect},
	{19, dgus_page10_11_press_lineselect},
	{20, dgus_page10_11_press_lineselect},
	{21, dgus_page10_11_press_lineselect},
	{22, dgus_page10_11_press_lineselect},
	{23, dgus_page10_11_press_lineselect},
	{24, dgus_page10_11_press_lineselect},
	{25, dgus_page10_11_press_lineselect},
	{26, dgus_page10_11_press_lineselect},
	{27, dgus_page10_11_press_lineselect},
	{28, dgus_page10_11_press_lineselect},
	{29, dgus_page10_11_press_lineselect},
	{30, dgus_page10_11_press_lineselect},
	{31, dgus_page10_11_press_lineselect},
	{32, dgus_page10_11_press_lineselect},
	{33, dgus_page10_11_press_lineselect},
	{34, dgus_page10_11_press_lineselect},
	{35, dgus_page10_11_press_lineselect},
	{36, dgus_page10_11_press_lineselect},
	{37, dgus_page10_11_press_lineselect},
	{38, dgus_page10_11_press_lineselect},
	{39, dgus_page10_11_press_lineselect},
	{40, dgus_page10_11_press_lineselect},

	{41, dgus_page10_11_press_lineselect},
	{42, dgus_page10_11_press_lineselect},
	{43, dgus_page10_11_press_lineselect},
	{44, dgus_page10_11_press_lineselect},
	{45, dgus_page10_11_press_lineselect},
	{46, dgus_page10_11_press_lineselect},
	{47, dgus_page10_11_press_lineselect},
	{48, dgus_page10_11_press_lineselect},
	{49, dgus_page10_11_press_lineselect},
	{50, dgus_page10_11_press_lineselect},
	{51, dgus_page10_11_press_lineselect},
	{52, dgus_page10_11_press_lineselect},
	{53, dgus_page10_11_press_lineselect},
	{54, dgus_page10_11_press_lineselect},
	{55, dgus_page10_11_press_lineselect},
	{56, dgus_page10_11_press_lineselect},
	{57, dgus_page10_11_press_lineselect},
	{58, dgus_page10_11_press_lineselect},
	{59, dgus_page10_11_press_lineselect},
	{60, dgus_page10_11_press_lineselect},
	{61, dgus_page10_11_press_lineselect},
	{62, dgus_page10_11_press_lineselect},
	{63, dgus_page10_11_press_lineselect},
	{64, dgus_page10_11_press_lineselect},
	{65, dgus_page10_11_press_lineselect},
	{66, dgus_page10_11_press_lineselect},
	{67, dgus_page10_11_press_lineselect},
	{68, dgus_page10_11_press_lineselect},
	{69, dgus_page10_11_press_lineselect},
	{70, dgus_page10_11_press_lineselect},
	{71, dgus_page10_11_press_lineselect},
	{72, dgus_page10_11_press_lineselect},

	{73, dgus_page12_press_confirmorreset},

	{74, dgus_page13_14_press_lineselect},
	{75, dgus_page13_14_press_lineselect},
	{76, dgus_page13_14_press_lineselect},
	{77, dgus_page13_14_press_lineselect},
	{78, dgus_page13_14_press_lineselect},
	{79, dgus_page13_14_press_lineselect},
	{80, dgus_page13_14_press_lineselect},
	{81, dgus_page13_14_press_lineselect},
	{82, dgus_page13_14_press_lineselect},
	{83, dgus_page13_14_press_lineselect},
	{84, dgus_page13_14_press_lineselect},
	{85, dgus_page13_14_press_lineselect},
	{86, dgus_page13_14_press_lineselect},
	{87, dgus_page13_14_press_lineselect},
	{88, dgus_page13_14_press_lineselect},
	{89, dgus_page13_14_press_lineselect},
	{90, dgus_page13_14_press_lineselect},
	{91, dgus_page13_14_press_lineselect},
	{92, dgus_page13_14_press_lineselect},
	{93, dgus_page13_14_press_lineselect},
	{94, dgus_page13_14_press_lineselect},
	{95, dgus_page13_14_press_lineselect},
	{96, dgus_page13_14_press_lineselect},
	{97, dgus_page13_14_press_lineselect},
	{98, dgus_page13_14_press_lineselect},
	{99, dgus_page13_14_press_lineselect},
	{100, dgus_page13_14_press_lineselect},
	{101, dgus_page13_14_press_lineselect},
	{102, dgus_page13_14_press_lineselect},
	{103, dgus_page13_14_press_lineselect},
	{104, dgus_page13_14_press_lineselect},
	{105, dgus_page13_14_press_lineselect},

	{106, dgus_page13_14_press_lineselect},
	{107, dgus_page13_14_press_lineselect},
	{108, dgus_page13_14_press_lineselect},
	{109, dgus_page13_14_press_lineselect},
	{110, dgus_page13_14_press_lineselect},
	{111, dgus_page13_14_press_lineselect},
	{112, dgus_page13_14_press_lineselect},
	{113, dgus_page13_14_press_lineselect},
	{114, dgus_page13_14_press_lineselect},
	{115, dgus_page13_14_press_lineselect},
	{116, dgus_page13_14_press_lineselect},
	{117, dgus_page13_14_press_lineselect},
	{118, dgus_page13_14_press_lineselect},
	{119, dgus_page13_14_press_lineselect},
	{120, dgus_page13_14_press_lineselect},
	{121, dgus_page13_14_press_lineselect},
	{122, dgus_page13_14_press_lineselect},
	{123, dgus_page13_14_press_lineselect},
	{124, dgus_page13_14_press_lineselect},
	{125, dgus_page13_14_press_lineselect},
	{126, dgus_page13_14_press_lineselect},
	{127, dgus_page13_14_press_lineselect},
	{128, dgus_page13_14_press_lineselect},
	{129, dgus_page13_14_press_lineselect},
	{130, dgus_page13_14_press_lineselect},
	{131, dgus_page13_14_press_lineselect},
	{132, dgus_page13_14_press_lineselect},
	{133, dgus_page13_14_press_lineselect},
	{134, dgus_page13_14_press_lineselect},
	{135, dgus_page13_14_press_lineselect},
	{136, dgus_page13_14_press_lineselect},
	{137, dgus_page13_14_press_lineselect},

	{138, dgus_page15_press_confirmorcancel},

	{139, dgus_page17_press_confirmorreset},

	{140, dgus_page18_press_confirmorresetorstop},
	{141, dgus_page18_press_confirmorreset},

	{142, dgus_page19_20_press_setzeroorconfirmorcancel},

	{143, dgus_page21_22_press_lineselect},
	{144, dgus_page21_22_press_lineselect},
	{145, dgus_page21_22_press_lineselect},
	{146, dgus_page21_22_press_lineselect},
	{147, dgus_page21_22_press_lineselect},
	{148, dgus_page21_22_press_lineselect},
	{149, dgus_page21_22_press_lineselect},
	{150, dgus_page21_22_press_lineselect},
	{151, dgus_page21_22_press_lineselect},
	{152, dgus_page21_22_press_lineselect},
	{153, dgus_page21_22_press_lineselect},
	{154, dgus_page21_22_press_lineselect},
	{155, dgus_page21_22_press_lineselect},
	{156, dgus_page21_22_press_lineselect},
	{157, dgus_page21_22_press_lineselect},
	{158, dgus_page21_22_press_lineselect},
	{159, dgus_page21_22_press_lineselect},
	{160, dgus_page21_22_press_lineselect},
	{161, dgus_page21_22_press_lineselect},
	{162, dgus_page21_22_press_lineselect},
	{163, dgus_page21_22_press_lineselect},
	{164, dgus_page21_22_press_lineselect},
	{165, dgus_page21_22_press_lineselect},
	{166, dgus_page21_22_press_lineselect},
	{167, dgus_page21_22_press_lineselect},
	{168, dgus_page21_22_press_lineselect},
	{169, dgus_page21_22_press_lineselect},
	{170, dgus_page21_22_press_lineselect},
	{171, dgus_page21_22_press_lineselect},
	{172, dgus_page21_22_press_lineselect},
	{173, dgus_page21_22_press_lineselect},
	{174, dgus_page21_22_press_lineselect},

	{175, dgus_page21_22_press_lineselect},
	{176, dgus_page21_22_press_lineselect},
	{177, dgus_page21_22_press_lineselect},
	{178, dgus_page21_22_press_lineselect},
	{179, dgus_page21_22_press_lineselect},
	{180, dgus_page21_22_press_lineselect},
	{181, dgus_page21_22_press_lineselect},
	{182, dgus_page21_22_press_lineselect},
	{183, dgus_page21_22_press_lineselect},
	{184, dgus_page21_22_press_lineselect},
	{185, dgus_page21_22_press_lineselect},
	{186, dgus_page21_22_press_lineselect},
	{187, dgus_page21_22_press_lineselect},
	{188, dgus_page21_22_press_lineselect},
	{189, dgus_page21_22_press_lineselect},
	{190, dgus_page21_22_press_lineselect},
	{191, dgus_page21_22_press_lineselect},
	{192, dgus_page21_22_press_lineselect},
	{193, dgus_page21_22_press_lineselect},
	{194, dgus_page21_22_press_lineselect},
	{195, dgus_page21_22_press_lineselect},
	{196, dgus_page21_22_press_lineselect},
	{197, dgus_page21_22_press_lineselect},
	{198, dgus_page21_22_press_lineselect},
	{199, dgus_page21_22_press_lineselect},
	{200, dgus_page21_22_press_lineselect},
	{201, dgus_page21_22_press_lineselect},
	{202, dgus_page21_22_press_lineselect},
	{203, dgus_page21_22_press_lineselect},
	{204, dgus_page21_22_press_lineselect},
	{205, dgus_page21_22_press_lineselect},
	{206, dgus_page21_22_press_lineselect},

	{207, dgus_page23_press_cancel},

	{208, dgus_page24_press_confirmorresetorgenorcal},

	{209, dgus_page25_press_confirmorresetorgenorcal},

	{210, dgus_page26_press_confirmorresetorgen},

	{211, dgus_page27_29_31_33_press_daypos},
	{212, dgus_page27_29_31_33_press_daypos},
	{213, dgus_page27_29_31_33_press_daypos},
	{214, dgus_page27_29_31_33_press_daypos},

	{215, dgus_page36_press_nextorprev},

	{216, NULL},

	{217, dgus_page43_press_upgrade},

	{218, NULL},
	{219, NULL},
	{220, NULL},
	{221, NULL},
	{222, NULL},
	{223, dgus_page_change},

	{0xffff, NULL},
};

#if 0
static void dgus_page7_report_var(unsigned short key_addr_offset, 
	unsigned short key, unsigned short *data_buf, int buf_len, int *len){
	date_eqinterval[key_addr_offset - 8] = key;
}

static struct dgus_keypress_callback internal_var_report_callback_array[] = {
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{8, dgus_page7_report_var},
	{9, dgus_page7_report_var},
	{10, dgus_page7_report_var},
	{11, dgus_page7_report_var},
	{12, dgus_page7_report_var},
	{0xffff, NULL},
};
#endif

static void dgus_page05_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;

	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x1000, 1, callback_buf_word, ret_len - 1);
		dgus_access_address(0x1050, 1, &callback_buf_word[ret_len - 1], 1);
	}

}

static void dgus_page08_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x100D, 1, callback_buf_word, ret_len);
	}
}

static void dgus_page09_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x100F, 1, callback_buf_word, ret_len);
	}
}

static void dgus_page12_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x1011, 1, callback_buf_word, ret_len);
	}
}

static void dgus_page16_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x1013, 1, callback_buf_word, ret_len);
	}
}

static void dgus_page17_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x1015, 1, callback_buf_word, ret_len);
	}
}

static void dgus_page18_display(unsigned short page_num, 
	unsigned short *data_buf, int buf_len, int *len){
	int ret_len;
	DGUS_PAGECHANGE_CALLBACK pagechange_callback;

	memset(callback_buf_word, 0, sizeof(callback_buf_word));
	pagechange_callback = p_main_callback->page_callback[page_num].callback;
	if(pagechange_callback != NULL){
		pagechange_callback(page_num, callback_buf_word, sizeof(callback_buf_word), &ret_len);
		dgus_access_address(0x1017, 1, callback_buf_word, ret_len);
	}
}
static struct dgus_page_callback internal_page_callback_array[] = {
	{0, NULL},
	{1, NULL},
	{2, NULL},
	{3, NULL},
	{4, NULL},
	{5, dgus_page05_display},
	{6, NULL},
	{7, NULL},
	{8, dgus_page09_display},
	{9, NULL},
	{10, NULL},
	{11, NULL},
	{12, dgus_page12_display},
	{13, NULL},
	{14, NULL},
	{15, NULL},
	{16, dgus_page16_display},
	{17, dgus_page17_display},
	{18, dgus_page18_display},
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

/*
 * internal callback table
 */
static struct dgus_callback internal_callback = {
	.keypress_callback = internal_keypress_callback_array,
	.page_callback = internal_page_callback_array,
};

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

static int dgus_access_reg(unsigned char reg, int is_write, unsigned char *data,
		unsigned char len){
	//unsigned char *buf;
	int buf_len, cmd_len;
	int nread;
	int cmd_code_len;
	int i;

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
	//buf = malloc(buf_len);
	//if(buf == NULL){
	//	return -5;
	//}
	memset(buf_byte, 0, sizeof(buf_byte));

	buf_byte[0] = HEADER_BYTE1;
	buf_byte[1] = HEADER_BYTE2;
	if(is_write == 0){
		buf_byte[2] = 3;
		buf_byte[3] = 0x81;
		buf_byte[5] = len;
		cmd_len = buf_len - len;
	}else{
		buf_byte[2] = 2 + len;
		buf_byte[3] = 0x80;
		memcpy(buf_byte + 5, data, len);
		cmd_len = buf_len;
	}
	buf_byte[4] = reg;

	write(dgus_fd, buf_byte, cmd_len);
	if(is_write == 0){
		nread = read(dgus_fd, buf_byte, cmd_len + len);
		if(nread != cmd_len + len){
			//free(buf);
			return -6;
		}

		memcpy(data, buf_byte + cmd_len, len);
	}
	//free(buf);
	return 0;
}

static unsigned char _dgus_read_version(){
	unsigned char ver = 0xff;
	dgus_access_reg(REG_ADDRES_VERSION, 0, &ver, 1);
	return ver;
}

/*
 * read from serial device to get the info
 */
static int _dgus_wait_to_read_report(unsigned short *var_addr){
	int ret = 0;
	int i = 0;
	if(dgus_fd > 0){
		memset(buf_byte, 0, sizeof(buf_byte));
		ret = read(dgus_fd, buf_byte, sizeof(buf_byte));
	}
	if(ret <= 0){
		return ret;
	}

	while(i < ret){
		printf("read %d = 0x%x\n", i, buf_byte[i]);
		i++;
	}
	ret = 0;
	if((buf_byte[0] == HEADER_BYTE1) &&
		(buf_byte[1] == HEADER_BYTE2) &&
		(buf_byte[3] == 0x83)){/* report the event */
		if(((buf_byte[2] - 4) / 2) == buf_byte[6]){
			int i = 0, data_len_byte = buf_byte[6] * 2;
			unsigned char *p = buf_byte;

			*var_addr = buf_byte[4] << 8 | buf_byte[5];
			memset(buf_word, 0, sizeof(buf_word));

			p = buf_byte + 7;
			while(i < data_len_byte){
				buf_word[i/2] = *(p + i) << 8 | *(p + i + 1);
				i += 2;
			}
			ret = buf_byte[6];
		}
	}
	return ret;
}

static int dgus_access_address(unsigned short addr, int is_write,
		unsigned short *data, unsigned short len){
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
	buf[1] = HEADER_BYTE2;
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

	i = 0;
	while(i < cmd_len){
		printf("write buf %d = 0x%x\n", i, buf[i]);
		i++;
	}
	write(dgus_fd, buf, cmd_len);
	memset(buf, 0, buf_len);
	if(is_write == 0){
		nread = read(dgus_fd, buf, buf_len);
		printf("nread = %d, buf_len = %d\n",nread,  buf_len);
		i = 0;
		while(i < buf_len){
			printf("read buf %d = %x\n", i, buf[i]);
			i++;
		}
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

static void dgus_waiting_thread_func(void *para){
	int ret;
	unsigned short var_addr;
	//struct dgus_callback *main_callback = (struct dgus_callback *)para;

	while(1){
		if((ret = _dgus_wait_to_read_report(&var_addr)) > 0){
			printf("get var_addr = %x\n", var_addr);

			if(ret == 1){
				int addr_offset = var_addr - VAR_ADDRES_REPORT_BASE;
				DGUS_REPORT_CALLBACK report_callback;

				printf("addr_offset = %d\n", addr_offset);
				printf("internal_callback size = %d\n", sizeof(internal_keypress_callback_array)/sizeof(struct dgus_keypress_callback));
				if(addr_offset >= 0 &&
					addr_offset <  sizeof(internal_keypress_callback_array)/sizeof(struct dgus_keypress_callback)){
					report_callback = internal_callback.keypress_callback[addr_offset].callback;
					if(report_callback != NULL){
						report_callback(addr_offset, buf_word[0], NULL, 0, NULL);
					}
				}
				//addr_offset = var_addr - 0x1000;
				//if(addr_offset >= 0 &&
				//	addr_offset <  sizeof(internal_var_report_callback_array)/sizeof(struct dgus_keypress_callback)){
				//	report_callback = internal_var_report_callback_array[addr_offset].callback;
				//	if(report_callback != NULL){
				//		report_callback(addr_offset, buf_word[0], NULL, 0, NULL);
				//	}
				//}

			}
		}
	}

}


/************************************************ 
设置操作系统时间 
参数:*dt数据格式为"2006-4-20 20:30:30" 
调用方法: 
    char *pt="2006-4-20 20:30:30"; 
    SetSystemTime(pt); 
**************************************************/  
static int dgus_set_system_time()  
{  
	struct tm _tm;  
	struct timeval tv;  
	time_t timep; 
	unsigned char date[7];
	int i = 0;

	dgus_access_reg(0x20, 0, date, 7);
	while(i < 7){
			printf("date[%d] = %d ", i, (date[i] / 16) * 10 + (date[i] % 16));
			i++;
	}
	printf("\n");

    _tm.tm_sec = (date[6] / 16) * 10 + (date[6] % 16);  
    _tm.tm_min = (date[5] / 16) * 10 + (date[5] % 16);  
    _tm.tm_hour = (date[4] / 16) * 10 + (date[4] % 16);  
    _tm.tm_mday = (date[2] / 16) * 10 + (date[2] % 16);  
    _tm.tm_mon = (date[1] / 16) * 10 + (date[1] % 16) - 1;  
    _tm.tm_year = (date[0] / 16) * 10 + (date[0] % 16) + 2000 - 1900; 
  
    timep = mktime(&_tm);  
    tv.tv_sec = timep;  
    tv.tv_usec = 0;  
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
	    printf("Set system datatime error!/n");  
	    return -1;  
    }  
    return 0;  
}  


int dgus_init(char *device_path, struct dgus_callback *main_callback){

	unsigned char ver;
	
	if(dgus_fd > 0){
		return 0;
	}
	dgus_fd = open(device_path, O_RDWR);
	if (dgus_fd < 0){
		printf("open device %s faild\n", TTY_DEVICE);
		return -1;
	}
	set_speed(dgus_fd,115200);
	set_Parity(dgus_fd,8,1,'N');
	/*
	 *	to be done
	 */
	ver = _dgus_read_version();
	printf("the dgus ver is %u\n", ver);

	
	dgus_set_system_time();

	p_main_callback = main_callback;
	pthread_create(&dgus_thread,NULL,(void*)dgus_waiting_thread_func,NULL);
	return 0;
}

void dgus_deinit(){
	if(dgus_fd > 0){
		close(dgus_fd);
	}
}

void dgus_get_now_date(unsigned char *data, int len){

	dgus_access_reg(0x20, 0, data, len);
}




