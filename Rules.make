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

export OUT_DIR=$(TI_SDK_PATH)/out

export CONFIG_DIR=$(TI_SDK_PATH)/configs

#root of the target file system for installing applications
export ROOTFS=$(OUT_DIR)/rootfs

#Points to the root of the Linux libraries and headers matching the
#demo file system.
#export LINUX_DEVKIT_PATH=$(TI_SDK_PATH)/linux-devkit

#Cross compiler prefix
export CROSS_COMPILE=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

#Default CC value to be used when cross compiling.  This is so that the
#GNU Make default of "cc" is not used to point to the host compiler
export CC=$(CROSS_COMPILE)gcc

#Location of environment-setup file
export ENV_SETUP=$(LINUX_DEVKIT_PATH)/environment-setup

#The directory that points to the SDK kernel source tree
export LINUX_KERNEL_SRC_DIR=$(TI_SDK_PATH)/board-support/linux-4.9.59

#The directory that points to the SDK uboot source tree
export UBOOT_SRC_DIR=$(TI_SDK_PATH)/board-support/u-boot-2017.01

#The directory that points to the busybox source tree
export BUSYBOX_DIR=$(TI_SDK_PATH)/busybox-1.28.3

CFLAGS= -march=armv7-a -marm -mfpu=neon  -mfloat-abi=hard

#Strip modules when installing to conserve disk space
INSTALL_MOD_STRIP=1

SDK_PATH_TARGET=$(TI_SDK_PATH)/linux-devkit/sysroots/armv7ahf-neon-linux-gnueabi/

# Set EXEC_DIR to install example binaries
EXEC_DIR=/home/zte/ti-processor-sdk-linux-am335x-evm-04.02.00.09/targetNFS/home/root/am335x-evm

export NCURSES_SRC=$(TI_SDK_PATH)/apps/ncurses-5.9
export MYSQL_SRC=$(TI_SDK_PATH)/apps/mysql-5.1.73

MAKE_JOBS=2

SHELL=/bin/bash
