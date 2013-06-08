This fork of QEMU will allow you to emulate the Olimex OLinuXino board within another host OS; for example OS X.

To use:

* ./configure
* make

run with: 

```
rm-softmmu/qemu-system-arm -s -machine imx233o -kernel ~/zImage_dtb -serial stdio -sd ~/ArchLinuxARM-olinuxino-latest.img
```

Where ```ArchLinuxARM-olinuxino-latest.img``` is an ArchLinux image, and the zImage_dtb kernel is configured to boot from the image.

For example, the default ArchLinux distro image will work (from http://archlinuxarm.org/platforms/armv5/olinuxino) with the kernel configure to use to root filesystem on /dev/mmcblk0p2.

