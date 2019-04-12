#platform
PLATFORM=am335x-evm

#defconfig
DEFCONFIG=tisdk_am335x-evm_defconfig

#Architecture
ARCH=armv7-a

#u-boot machine
UBOOT_MACHINE=am335x_evm_config

#Points to the root of the TI SDK
export TI_SDK_PATH=$(shell pwd)

export CONFIG_DIR=$(TI_SDK_PATH)/configs

#Points to the root of the Linux libraries and headers matching the
#demo file system.
#export LINUX_DEVKIT_PATH=$(TI_SDK_PATH)/linux-devkit

#Cross compiler prefix
export TOOLS_DIR=$(TI_SDK_PATH)/tools
export GCC_DIR=$(TOOLS_DIR)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf
export GCC_BIN_PATH=$(GCC_DIR)/bin
export CROSS_COMPILE=$(GCC_BIN_PATH)/arm-linux-gnueabihf-

#Default CC value to be used when cross compiling.  This is so that the
#GNU Make default of "cc" is not used to point to the host compiler
export CC=$(CROSS_COMPILE)gcc
export STRIP=$(CROSS_COMPILE)strip
export LDFLAGS="-L$(GCC_DIR)/lib -L$(GCC_DIR)/arm-linux-gnueabihf/lib"

#Location of environment-setup file
export ENV_SETUP=$(LINUX_DEVKIT_PATH)/environment-setup

#The directory that points to the SDK kernel source tree
export LINUX_KERNEL_SRC_DIR=$(TI_SDK_PATH)/board-support/linux-4.9.59_ti

#The directory that points to the SDK uboot source tree
export UBOOT_SRC_DIR=$(TI_SDK_PATH)/board-support/u-boot-2017.01_ti

#The directory that points to the busybox source tree
export BUSYBOX_DIR=$(TI_SDK_PATH)/busybox-1.28.3

CFLAGS= -march=armv7-a -marm -mfpu=neon  -mfloat-abi=hard
LDFLAGS=-L$(GCC_DIR)/lib -L$(GCC_DIR)/arm-linux-gnueabihf/lib

#Strip modules when installing to conserve disk space
INSTALL_MOD_STRIP=1

#SDK_PATH_TARGET=$(TI_SDK_PATH)/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/

# Set EXEC_DIR to install example binaries
#EXEC_DIR=/home/zte/ti-processor-sdk-linux-am335x-evm-04.02.00.09/targetNFS/home/root/am335x-evm

export APP_SRC_DIR=$(TI_SDK_PATH)/apps
export NCURSES_SRC=$(APP_SRC_DIR)/ncurses-5.9
export MYSQL_SRC=$(APP_SRC_DIR)/mysql-5.1.73
export SQLITE_SRC=$(APP_SRC_DIR)/sqlite-autoconf-3240000
export VSFTPD_SRC=$(APP_SRC_DIR)/vsftpd-2.3.4
export APP_TEST_SRC=$(APP_SRC_DIR)/app_test
export MTDUTILS_SRC=$(APP_SRC_DIR)/mtd-utils
export ZLIB_SRC=$(APP_SRC_DIR)/zlib
export LZO_SRC=$(APP_SRC_DIR)/lzo
export E2FSPROGS_SRC=$(APP_SRC_DIR)/e2fsprogs
export HENHOUSE_SRC=$(APP_SRC_DIR)/henhouse

#root of the target file system for installing applications
export ROOTFS=$(OUT_DIR)/rootfs
export OUT_DIR=$(TI_SDK_PATH)/out
export INTERMEDIATES_DIR=$(OUT_DIR)/intermediates
export KERNEL_OBJ=$(INTERMEDIATES_DIR)/kernel_obj
export BUSYBOX_OBJ=$(INTERMEDIATES_DIR)/busybox_obj
export UBOOT_OBJ=$(INTERMEDIATES_DIR)/uboot_obj
export MYSQL_OBJ=$(INTERMEDIATES_DIR)/mysql_obj
export SQLITE_OBJ=$(INTERMEDIATES_DIR)/sqlite_obj
export NCURSES_OBJ=$(INTERMEDIATES_DIR)/ncurses_obj
export VSFTPD_OBJ=$(INTERMEDIATES_DIR)/vsftpd_obj
export MTDUTILS_OBJ=$(INTERMEDIATES_DIR)/mtdutils_obj
export ZLIB_OBJ=$(INTERMEDIATES_DIR)/zlib_obj
export LZO_OBJ=$(INTERMEDIATES_DIR)/lzo_obj
export E2FSPROGS_OBJ=$(INTERMEDIATES_DIR)/e2fsprogs_obj

export NCURSES_DESTDIR=$(OUT_DIR)/ncurses-5.9
export MYSQL_DESTDIR=$(OUT_DIR)/mysql-5.1.73
export SQLITE_DESTDIR=$(OUT_DIR)/sqlite-autoconf-3240000
export MTDUTILS_DESTDIR=$(OUT_DIR)/mtd-utils
export ZLIB_DESTDIR=$(OUT_DIR)/zlib
export LZO_DESTDIR=$(OUT_DIR)/lzo
export E2FSPROGS_DESTDIR=$(OUT_DIR)/e2fsprogs

export MKUBIFS_ARGS= -m 2048 -e 126976 -c 720
export UBINIZE_ARGS= -m 2048 -p 128KiB -s 512 -O 2048

MAKE_JOBS=`cat /proc/cpuinfo | grep processor | wc -l`

SHELL=/bin/bash
