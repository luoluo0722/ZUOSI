-include Rules.make

MAKE_JOBS ?= 1

all: linux u-boot-spl linux-dtbs busybox ncurses mysql ubifsimg
clean: linux_clean u-boot-spl_clean linux-dtbs_clean busybox_clean ncurses_clean mysql_clean
install: linux_install u-boot-spl_install linux-dtbs_install

# Kernel build targets
linux: linux-dtbs
	@echo =================================
	@echo     Building the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) zImage
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules

linux_install: linux-dtbs_install
	@echo ===================================
	@echo     Installing the Linux Kernel
	@echo ===================================
	@if [ ! -d $(ROOTFS) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(ROOTFS)/boot
	install $(LINUX_KERNEL_SRC_DIR)/arch/arm/boot/zImage $(ROOTFS)/boot
	install $(LINUX_KERNEL_SRC_DIR)/vmlinux $(ROOTFS)/boot
	install $(LINUX_KERNEL_SRC_DIR)/System.map $(ROOTFS)/boot
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_MOD_PATH=$(ROOTFS) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install

linux_clean:
	@echo =================================
	@echo     Cleaning the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) mrproper

# u-boot build targets
u-boot-spl: u-boot
u-boot-spl_clean: u-boot_clean
u-boot-spl_install: u-boot_install

u-boot: linux-dtbs
	@echo ===================================
	@echo    Building U-boot
	@echo ===================================
	$(MAKE) -j $(MAKE_JOBS) -C $(UBOOT_SRC_DIR) CROSS_COMPILE=$(CROSS_COMPILE) $(UBOOT_MACHINE)
	$(MAKE) -j $(MAKE_JOBS) -C $(UBOOT_SRC_DIR) CROSS_COMPILE=$(CROSS_COMPILE) DTC=$(LINUX_KERNEL_SRC_DIR)/scripts/dtc/dtc
	@echo ===================================
	@echo    Installing U-boot
	@echo ===================================
	cp $(UBOOT_SRC_DIR)/MLO $(TI_SDK_PATH)/out/
	cp $(UBOOT_SRC_DIR)/u-boot.img $(TI_SDK_PATH)/out/
	cp $(UBOOT_SRC_DIR)/spl/u-boot-spl.bin $(TI_SDK_PATH)/out/

u-boot_clean:
	@echo ===================================
	@echo    Cleaining U-boot
	@echo ===================================
	$(MAKE) -C $(UBOOT_SRC_DIR) CROSS_COMPILE=$(CROSS_COMPILE) distclean

u-boot_install:
	@echo ===================================
	@echo    Installing U-boot
	@echo ===================================
	cp $(UBOOT_SRC_DIR)/MLO $(TI_SDK_PATH)/out/
	cp $(UBOOT_SRC_DIR)/u-boot.img $(TI_SDK_PATH)/out/
	cp $(UBOOT_SRC_DIR)/spl/u-boot-spl.bin $(TI_SDK_PATH)/out/

# Kernel DTB build targets
linux-dtbs:
	@echo =====================================
	@echo     Building the Linux Kernel DTBs
	@echo =====================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) am335x-evm.dtb am335x-evmsk.dtb am335x-bone.dtb am335x-boneblack.dtb am335x-bonegreen.dtb am335x-icev2.dtb am335x-icev2-pru-excl-uio.dtb am335x-boneblack-iot-cape.dtb am335x-icev2-pru-excl-uio.dtb

linux-dtbs_install:
	@echo =======================================
	@echo     Installing the Linux Kernel DTBs
	@echo =======================================
	@if [ ! -d $(ROOTFS) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(ROOTFS)/boot
	@cp -f $(LINUX_KERNEL_SRC_DIR)/arch/arm/boot/dts/*.dtb $(ROOTFS)/boot/

linux-dtbs_clean:
	@echo =======================================
	@echo     Cleaning the Linux Kernel DTBs
	@echo =======================================
	@echo "Nothing to do"

busybox:
	@echo =====================================
	@echo     Building the busybox for rootfs
	@echo =====================================
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	pushd $(ROOTFS);mkdir -p dev etc lib usr var proc tmp home root mnt sys;popd
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) install
	cp -r $(BUSYBOX_DIR)/examples/bootfloppy/etc/* $(ROOTFS)/etc
	cp -a $(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/lib/*so* $(ROOTFS)/lib/
	cp -a $(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/lib/*so* $(ROOTFS)/lib/
	
busybox_clean:
	@echo =======================================
	@echo     Cleaning the busybox
	@echo =======================================
	$(MAKE) -C $(BUSYBOX_DIR) clean
	rm -rf $(ROOTFS)/*

ncurses:
	@echo =====================================
	@echo     Building the ncurses for mysql
	@echo =====================================
	pushd $(NCURSES_SRC);PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) ./configure --host=arm-linux-gnueabihf --prefix=/usr --enable-static;popd
	PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) $(MAKE) -C $(NCURSES_SRC)
	PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) $(MAKE) -C $(NCURSES_SRC) install DESTDIR=$(TI_SDK_PATH)/out/intermediate/ncurses-5.9
	
ncurses_clean:
	@echo =======================================
	@echo     Cleaning the ncurses
	@echo =======================================
	$(MAKE) -C $(NCURSES_SRC) clean
	rm -rf $(TI_SDK_PATH)/out/intermediate/ncurses-5.9/*

mysql:
	@echo =====================================
	@echo     Building the mysql
	@echo =====================================
	pushd $(MYSQL_SRC);PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) ./configure --host=arm-linux-gnueabihf --prefix=/usr --with-named-curses-libs=$(TI_SDK_PATH)/out/intermediate/ncurses-5.9/usr/lib/libncurses.a --without-debug --without-docs --without-man --without-bench --with-charset=gb2312 --with-extra-charsets=ascii,latin1,utf8 --enable-static;popd
	pushd $(MYSQL_SRC)/sql;cp gen_lex_hash_x86 gen_lex_hash;cp lex_hash_new.h lex_hash.h;popd
	PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) $(MAKE) -C $(MYSQL_SRC)	
	pushd $(MYSQL_SRC)/sql;cp gen_lex_hash_x86 gen_lex_hash;cp lex_hash_new.h lex_hash.h;popd
	PATH=$(TI_SDK_PATH)/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/bin:$(PATH) $(MAKE) -C $(MYSQL_SRC) install DESTDIR=$(TI_SDK_PATH)/out/intermediate/mysql-5.1.73
	rm -rf $(TI_SDK_PATH)/out/intermediate/mysql-5.1.73/usr/lib/mysql/*.a
	rm -rf $(TI_SDK_PATH)/out/intermediate/mysql-5.1.73/usr/lib/mysql/plugin/*.a
	rm -rf $(TI_SDK_PATH)/out/intermediate/mysql-5.1.73/usr/mysql-test
	cp -ar $(TI_SDK_PATH)/out/intermediate/mysql-5.1.73/usr $(ROOTFS)/
	cp $(MYSQL_SRC)/support-files/my-medium.cnf $(ROOTFS)/etc/my.cnf
	cp $(MYSQL_SRC)/support-files/mysql.server $(ROOTFS)/etc/init.d/mysqld
	chmod +x $(ROOTFS)/etc/init.d/mysqld
	mkdir -p $(ROOTFS)/var/run/mysqld
	touch $(ROOTFS)/var/run/mysqld/mysqld.pid
	mkdir -p $(ROOTFS)/usr/sbin
	ln -sf /usr/bin/mysql $(ROOTFS)/usr/sbin/
	ln -sf /usr/bin/mysqldump $(ROOTFS)/usr/sbin/
	ln -sf /usr/bin/mysqladmin $(ROOTFS)/usr/sbin/
	
mysql_clean:
	@echo =======================================
	@echo     Cleaning the mysql
	@echo =======================================
	$(MAKE) -C $(MYSQL_SRC) clean
	rm -rf $(TI_SDK_PATH)/out/intermediate/mysql-5.1.73/*

ubifsimg:
	pushd $(OUT_DIR);mkfs.ubifs -r rootfs -m 2048 -e 126976 -c 992 -o am335xubifs.img;ubinize -o am335xubi.img -m 2048 -p 128KiB -s 512 -O 2048 $(CONFIG_DIR)/ubinize.cfg;popd
