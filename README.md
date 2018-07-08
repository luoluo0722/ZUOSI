# ZUOSI
The project for ZUOSI

1.Build environmenta
a.Ubuntu 16.04 LTS
b.sudo apt-get install git build-essential python diffstat texinfo gawk chrpath dos2unix wget unzip socat doxygen libc6:i386 libncurses5:i386 libstdc++6:i386 libz1 mtd-utils automake perl

2.Build
Enter the source root, and make

3.OUTPUT
All the output file is in the out directory, and it include: zImage, dtb, MLO, uboot image and ubi image

4.create-sdcard.sh
tools/script/create-sdcard.sh
It is used to create booting sdcard. Must be used with sudo.
