# initramfs

## Resources

* [https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt](https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt)

* [https://www.linux.com/training-tutorials/kernel-newbie-corner-initrd-and-initramfs-whats/](https://www.linux.com/training-tutorials/kernel-newbie-corner-initrd-and-initramfs-whats/)

## Setup on RHEL 8

~~~
yum install qemu-kvm-core
yum install '@Development Tools'
subscription-manager repos --enable=codeready-builder-for-rhel-8-x86_64-rpms
yum install glibc-static -y
~~~

Build custom init binary:
~~~
cat > hello.c << EOF
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  printf("Hello world!\n");
  sleep(999999999);
}
EOF
gcc -static hello.c -o init
~~~

Build custom initrd image:
~~~
echo init | cpio -o -H newc | gzip > test.cpio.gz
~~~

Run in qemu:
~~~
/usr/libexec/qemu-kvm -kernel /boot/vmlinuz-4.18.0-240.1.1.el8_3.x86_64 -initrd test.cpio.gz -nographic -append console=ttyS0
~~~

To get out of it:
~~~
CTRL+A X
~~~
