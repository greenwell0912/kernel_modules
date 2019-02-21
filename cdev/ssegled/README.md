

参考
====
https://www.raspberrypi.org/documentation/linux/kernel/building.md

環境
====
HOST:Ubuntu 16.04 LTS

```
$ cd ~/work/raspberrypi3
$ git clone --depth=1 --branch rpi-4.18.y https://github.com/raspberrypi/linux linux-rpi-4.18.y
$ git clone https://github.com/raspberrypi/tools ~/work/raspberrypi3/tools
$ export PATH="$PATH":~/work/raspberrypi3/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin

$ sudo apt-get install git bison flex libssl-dev

$ cd linux-rpi-4.18.y
$ KERNEL=kernel7
$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
```

```
$ mkdir mnt
$ mkdir mnt/fat32
$ mkdir mnt/ext4
$ sudo umount /dev/sdc1
$ sudo umount /dev/sdc2
$ sudo mount /dev/sdc1 mnt/fat32
$ sudo mount /dev/sdc2 mnt/ext4
```


```
$ cd linux-rpi-4.18.y
$ sudo make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- INSTALL_MOD_PATH=mnt/ext4 modules_install
```

```
sudo cp mnt/fat32/$KERNEL.img mnt/fat32/$KERNEL-backup.img
sudo cp arch/arm/boot/zImage mnt/fat32/$KERNEL.img
sudo cp arch/arm/boot/dts/*.dtb mnt/fat32/
sudo cp arch/arm/boot/dts/overlays/*.dtb* mnt/fat32/overlays/
sudo cp arch/arm/boot/dts/overlays/README mnt/fat32/overlays/
sudo umount mnt/fat32
sudo umount mnt/ext4
```

```
pi@raspberrypi:~$ cat /proc/version 
Linux version 4.18.20-v7+ (hiroki@ubuntu-mac) (gcc version 5.4.0 20160609 (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.9)) #1 SMP Wed Feb 20 21:25:45 JST 2019
```

```
$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C (your dir)/linux-rpi-4.18.y M=(your dir)/kernel_modules/cdev/ssegled
```

```
$ su
# sudo insmod ssegled.ko 
# echo 1 > /dev/ssegled0
```
