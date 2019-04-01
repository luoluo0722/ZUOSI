-include Rules.make

MAKE_JOBS ?= 1

all: ubifsimg
ubifsimg: linux u-boot-spl linux-dtbs busybox vsftpd app_test mtd-utils sqlite
clean: linux_clean u-boot-spl_clean linux-dtbs_clean busybox_clean ncurses_clean mysql_clean sqlite_clean vsftpd_clean clean_out_dir
install: linux_install u-boot-spl_install linux-dtbs_install

make_init:
	install -d $(ROOTFS)
	install -d $(ROOTFS)/etc
	install -d $(ROOTFS)/etc/init.d
	install -d $(ROOTFS)/usr/lib
	install -d $(KERNEL_OBJ)
	install -d $(UBOOT_OBJ)
	install -d $(NCURSES_OBJ)
	install -d $(BUSYBOX_OBJ)
	install -d $(MYSQL_OBJ)
	install -d $(SQLITE_OBJ)
	install -d $(MTDUTILS_OBJ)
	install -d $(ZLIB_OBJ)
	install -d $(LZO_OBJ)
	install -d $(E2FSPROGS_OBJ)

# Kernel build targets
linux: linux-dtbs
	@echo =================================
	@echo     Building the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) zImage
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules
	install -d $(OUT_DIR)
	install $(KERNEL_OBJ)/arch/arm/boot/zImage $(OUT_DIR)
	install $(KERNEL_OBJ)/vmlinux $(OUT_DIR)
	install $(KERNEL_OBJ)/System.map $(OUT_DIR)
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_MOD_PATH=$(ROOTFS) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install

linux_config:
	@echo =================================
	@echo     Configure the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) menuconfig

linux_install: linux-dtbs_install
	@echo ===================================
	@echo     Installing the Linux Kernel
	@echo ===================================
	@if [ ! -d $(OUT_DIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(OUT_DIR)
	install $(KERNEL_OBJ)/arch/arm/boot/zImage $(OUT_DIR)
	install $(KERNEL_OBJ)/vmlinux $(OUT_DIR)
	install $(KERNEL_OBJ)/System.map $(OUT_DIR)
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_MOD_PATH=$(ROOTFS) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) modules_install

linux_clean:
	@echo =================================
	@echo     Cleaning the Linux Kernel
	@echo =================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) mrproper
	rm -rf $(OUT_DIR)/zImage
	rm -rf $(OUT_DIR)/vmlinux
	rm -rf $(OUT_DIR)/System.map
	rm -rf $(KERNEL_OBJ)/*
	rm -rf $(ROOTFS)/lib

# u-boot build targets
u-boot-spl: u-boot
u-boot-spl_clean: u-boot_clean
u-boot-spl_install: u-boot_install

u-boot: linux-dtbs
	@echo ===================================
	@echo    Building U-boot
	@echo ===================================
	$(MAKE) -j $(MAKE_JOBS) -C $(UBOOT_SRC_DIR) O=$(UBOOT_OBJ) CROSS_COMPILE=$(CROSS_COMPILE) $(UBOOT_MACHINE)
	$(MAKE) -j $(MAKE_JOBS) -C $(UBOOT_SRC_DIR) O=$(UBOOT_OBJ) CROSS_COMPILE=$(CROSS_COMPILE) DTC=$(KERNEL_OBJ)/scripts/dtc/dtc
	@echo ===================================
	@echo    Installing U-boot
	@echo ===================================
	cp $(UBOOT_OBJ)/MLO $(OUT_DIR)
	cp $(UBOOT_OBJ)/u-boot.img $(OUT_DIR)
	cp $(UBOOT_OBJ)/spl/u-boot-spl.bin $(OUT_DIR)

u-boot_clean:
	@echo ===================================
	@echo    Cleaining U-boot
	@echo ===================================
	$(MAKE) -C $(UBOOT_SRC_DIR) O=$(UBOOT_OBJ) CROSS_COMPILE=$(CROSS_COMPILE) distclean
	rm -rf $(OUT_DIR)/MLO
	rm -rf $(OUT_DIR)/u-boot.img
	rm -rf $(OUT_DIR)/u-boot-spl.bin
	rm -rf $(UBOOT_OBJ)/*

u-boot_install:
	@echo ===================================
	@echo    Installing U-boot
	@echo ===================================
	cp $(UBOOT_OBJ)/MLO $(TI_SDK_PATH)/out/
	cp $(UBOOT_OBJ)/u-boot.img $(TI_SDK_PATH)/out/
	cp $(UBOOT_OBJ)/spl/u-boot-spl.bin $(TI_SDK_PATH)/out/

# Kernel DTB build targets
linux-dtbs:
	@echo =====================================
	@echo     Building the Linux Kernel DTBs
	@echo =====================================
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) $(DEFCONFIG)
	$(MAKE) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) headers_install
	$(MAKE) -j $(MAKE_JOBS) -C $(LINUX_KERNEL_SRC_DIR) O=$(KERNEL_OBJ) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) am335x-evm.dtb
	install -d $(OUT_DIR)
	cp -f $(KERNEL_OBJ)/arch/arm/boot/dts/am335x-evm.dtb $(OUT_DIR)

linux-dtbs_install:
	@echo =======================================
	@echo     Installing the Linux Kernel DTBs
	@echo =======================================
	@if [ ! -d $(OUT_DIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	install -d $(OUT_DIR)
	cp -f $(KERNEL_OBJ)/arch/arm/boot/dts/am335x-evm.dtb $(OUT_DIR)

linux-dtbs_clean:
	@echo =======================================
	@echo     Cleaning the Linux Kernel DTBs
	@echo =======================================
	@echo "Nothing to do"
	rm -rf $(OUT_DIR)/am335x-evm.dtb

busybox: make_init
	@echo =====================================
	@echo     Building the busybox for rootfs
	@echo =====================================

	pushd $(ROOTFS);mkdir -p dev etc lib usr var proc tmp home root mnt sys;popd
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUSYBOX_OBJ) CONFIG_PREFIX=$(ROOTFS) zuosi_defconfig
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUSYBOX_OBJ) CONFIG_PREFIX=$(ROOTFS)
	pushd $(ROOTFS);mkdir -p dev etc lib usr var proc tmp home root mnt sys;popd
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUSYBOX_OBJ) install CONFIG_PREFIX=$(ROOTFS)
	cp -r $(BUSYBOX_DIR)/examples/bootfloppy/etc/* $(ROOTFS)/etc
	chmod +x $(ROOTFS)/etc/init.d/rcS
	cp -a $(TI_SDK_PATH)/tools/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/lib/*so* $(ROOTFS)/lib/
	cp -a $(TI_SDK_PATH)/tools/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/lib/*so* $(ROOTFS)/lib/

busybox_config: make_init
	@echo =====================================
	@echo     Building the busybox for rootfs
	@echo =====================================
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUSYBOX_OBJ) CONFIG_PREFIX=$(ROOTFS) zuosi_defconfig
	$(MAKE) -C $(BUSYBOX_DIR) CROSS_COMPILE=$(CROSS_COMPILE) O=$(BUSYBOX_OBJ) CONFIG_PREFIX=$(ROOTFS) menuconfig
	
busybox_clean:
	@echo =======================================
	@echo     Cleaning the busybox
	@echo =======================================
	$(MAKE) -C $(BUSYBOX_DIR) O=$(BUSYBOX_OBJ) CONFIG_PREFIX=$(ROOTFS) mrproper
	rm -rf $(ROOTFS)/*

ncurses:make_init
	@echo =====================================
	@echo     Building the ncurses for mysql
	@echo =====================================
	pushd $(NCURSES_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) $(NCURSES_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr --enable-static;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(NCURSES_OBJ)
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(NCURSES_OBJ) install DESTDIR=$(NCURSES_DESTDIR)

mtd-utils:make_init
	@echo =====================================
	@echo     Building the mtd-utils
	@echo =====================================
	pushd $(MTDUTILS_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) CC=arm-linux-gnueabihf-gcc $(MTDUTILS_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr --enable-static --without-jffs --without-xattr --without-lzo --with-selinux --without-crypto --without-ubifs;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(MTDUTILS_OBJ)
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(MTDUTILS_OBJ) install DESTDIR=$(MTDUTILS_DESTDIR)
	rm -rf $(MTDUTILS_DESTDIR)/usr/share
	cp -ar $(MTDUTILS_DESTDIR)/usr $(ROOTFS)/
	
ncurses_clean:
	@echo =======================================
	@echo     Cleaning the ncurses
	@echo =======================================
	$(MAKE) -C $(NCURSES_OBJ) distclean
	rm -rf $(NCURSES_DESTDIR)

mysql: make_init
	@echo =====================================
	@echo     Building the mysql
	@echo =====================================
	pushd $(MYSQL_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) $(MYSQL_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr --with-named-curses-libs=$(NCURSES_DESTDIR)/usr/lib/libncurses.a --without-debug --without-docs --without-man --without-bench --with-charset=gb2312 --with-extra-charsets=ascii,latin1,utf8 --enable-static;popd
	#cp $(MYSQL_SRC)/scripts/*.sql $(MYSQL_OBJ)/scripts/
	pushd $(MYSQL_SRC)/sql;cp gen_lex_hash_x86 gen_lex_hash;cp lex_hash_new.h lex_hash.h;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(MYSQL_OBJ)
	pushd $(MYSQL_SRC)/sql;cp gen_lex_hash_x86 gen_lex_hash;cp lex_hash_new.h lex_hash.h;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(MYSQL_OBJ) install DESTDIR=$(MYSQL_DESTDIR)
	rm -rf $(MYSQL_DESTDIR)/usr/lib/mysql/*.a
	rm -rf $(MYSQL_DESTDIR)/usr/lib/mysql/plugin/*.a
	rm -rf $(MYSQL_DESTDIR)/usr/mysql-test
	cp -ar $(MYSQL_DESTDIR)/usr $(ROOTFS)/
	install $(MYSQL_SRC)/support-files/my-medium.cnf $(ROOTFS)/etc/my.cnf
	install $(MYSQL_SRC)/support-files/mysql.server $(ROOTFS)/etc/init.d/mysqld
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

sqlite: make_init
	@echo =====================================
	@echo     Building the sqlite
	@echo =====================================
	pushd $(SQLITE_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) $(SQLITE_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr ;popd
	#cp $(MYSQL_SRC)/scripts/*.sql $(MYSQL_OBJ)/scripts/
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(SQLITE_OBJ)
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(SQLITE_OBJ) install DESTDIR=$(SQLITE_DESTDIR)
	PATH=$(GCC_BIN_PATH):$(PATH) $(STRIP) $(SQLITE_DESTDIR)/usr/bin/*
	PATH=$(GCC_BIN_PATH):$(PATH) $(STRIP) $(SQLITE_DESTDIR)/usr/lib/*.so
	rm -rf $(SQLITE_DESTDIR)/usr/lib/*.a
	rm -rf $(SQLITE_DESTDIR)/usr/lib/*.la
	cp -ar $(SQLITE_DESTDIR)/usr $(ROOTFS)/
	mkdir -p $(ROOTFS)/usr/sbin
	ln -sf /usr/bin/sqlite3 $(ROOTFS)/usr/sbin/
	
sqlite_clean:
	@echo =======================================
	@echo     Cleaning the sqlite
	@echo =======================================
	$(MAKE) -C $(SQLITE_OBJ) distclean
	rm -rf $(TI_SDK_PATH)/out/intermediate/sqlite-autoconf-3240000/*

vsftpd: make_init
	@echo =====================================
	@echo     Building the vsftpd
	@echo =====================================
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(VSFTPD_SRC)
	mkdir -p $(ROOTFS)/usr/bin
	install -m 755 $(VSFTPD_SRC)/vsftpd $(ROOTFS)/usr/bin/vsftpd
	install -m 644 $(VSFTPD_SRC)/vsftpd.conf $(ROOTFS)/etc/vsftpd.conf
	
vsftpd_clean:
	@echo =======================================
	@echo     Cleaning the vsftpd
	@echo =======================================
	$(MAKE) -C $(VSFTPD_SRC) clean

app_test: make_init
	@echo =====================================
	@echo     Building the app_test
	@echo =====================================
	$(MAKE) -C $(APP_TEST_SRC) CROSS_COMPILE=$(CROSS_COMPILE) ROOTFS=$(ROOTFS)

zlib:make_init
	@echo =====================================
	@echo     Building the mtd-utils
	@echo =====================================
	pushd $(ZLIB_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) CC=arm-linux-gnueabihf-gcc $(ZLIB_SRC)/configure --prefix=/usr;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(ZLIB_OBJ)
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(ZLIB_OBJ) install DESTDIR=$(ZLIB_DESTDIR)
	cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so $(ROOTFS)/usr/lib
	cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so.1 $(ROOTFS)/usr/lib
	cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so.1.2.11 $(ROOTFS)/usr/lib

lzo:make_init
	@echo =====================================
	@echo     Building the mtd-utils
	@echo =====================================
	pushd $(LZO_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) CC=arm-linux-gnueabihf-gcc $(LZO_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(LZO_OBJ)
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(LZO_OBJ) install DESTDIR=$(LZO_DESTDIR)
	#rm -rf $(MTDUTILS_DESTDIR)/usr/share
	#cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so $(ROOTFS)/usr/lib
	#cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so.1 $(ROOTFS)/usr/lib
	#cp -ar $(ZLIB_DESTDIR)/usr/lib/libz.so.1.2.11 $(ROOTFS)/usr/lib

e2fsprogs:make_init
	@echo =====================================
	@echo     Building the mtd-utils
	@echo =====================================
	pushd $(E2FSPROGS_OBJ);PATH=$(GCC_BIN_PATH):$(PATH) CC=arm-linux-gnueabihf-gcc $(E2FSPROGS_SRC)/configure --host=arm-linux-gnueabihf --prefix=/usr;popd
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(E2FSPROGS_OBJ) all-libs-recursive
	PATH=$(GCC_BIN_PATH):$(PATH) $(MAKE) -C $(E2FSPROGS_OBJ) install-libs DESTDIR=$(E2FSPROGS_DESTDIR)

clean_out_dir:
	@echo =======================================
	@echo     Cleaning out_dir
	@echo =======================================
	rm -rf $(OUT_DIR)

ubifsimg:
	pushd $(OUT_DIR);mkfs.ubifs -r rootfs -o am335xubifs.img $(MKUBIFS_ARGS);ubinize -o am335xubi.img $(UBINIZE_ARGS) $(CONFIG_DIR)/ubinize.cfg;popd
