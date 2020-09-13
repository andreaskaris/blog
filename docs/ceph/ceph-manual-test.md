# Using Ceph with qemu-kvm manually #

## Install Ceph ##

Create a Ceph cluster according to: 
[https://access.redhat.com/documentation/en-us/red_hat_ceph_storage/3/html/container_guide/deploying-red-hat-ceph-storage-in-containers](https://access.redhat.com/documentation/en-us/red_hat_ceph_storage/3/html/container_guide/deploying-red-hat-ceph-storage-in-containers)

## Installing ceph credentials to /etc/ceph ##

Make sure that all required ceph credentials are in /etc/ceph. In this case, I copied them directly from /etc/ceph on one of my monitor nodes.

## Creating a pool ##

~~~
ceph osd pool create rbd-pool 128
rbd pool init rbd-pool
~~~

## Downloading and customizing RHEL qcow2 ##

Download the RHEL qcow2 from [https://access.redhat.com](https://access.redhat.com), e.g.: [https://access.redhat.com/downloads/content/69/ver=/rhel---7/7.7/x86_64/product-software](https://access.redhat.com/downloads/content/69/ver=/rhel---7/7.7/x86_64/product-software)

Install libguestfs-tools:
~~~
yum install libguestfs-tools -y
~~~

Change the root password of the image:
~~~
virt-customize -a rhel-server-7.8-beta-1-x86_64-kvm.qcow2 --password root:password
# or, if needed:
# export LIBGUESTFS_BACKEND=direct ; virt-customize -a rhel-server-7.8-beta-1-x86_64-kvm.qcow2 --password root:password
~~~

## Converting and uploading image into Ceph pool ##

~~~
qemu-img convert -f qcow2 -O raw rhel-server-7.8-beta-1-x86_64-kvm.qcow2 rbd:rbd-pool/rhel-server
~~~

#### Booting a VM from the raw Ceph image with QEMU-KVM ##

Start a VM that directly uses the uploaded image from the Ceph pool:
~~~
/usr/libexec/qemu-kvm -drive file=rbd:rbd-pool/rhel-server -nographic -m 1024
~~~

To get out of qemu-kvm, type `CTRL-a x`

You should be able to log into the image with credentials: `root` / `password`

> **Note:** With the RHEL cloud image, the screen will show grub, then go blank for a while. This is normal, just wait for a few seconds.

## Booting a VM from the raw Ceph image with libvirt ##

See:
[https://blog.modest-destiny.com/posts/kvm-libvirt-add-ceph-rbd-pool/](https://blog.modest-destiny.com/posts/kvm-libvirt-add-ceph-rbd-pool/)

~~~
[root@undercloud-0 ~]# DISK_SIZE=$(qemu-img  info rhel-server-7.8-beta-1-x86_64-kvm.raw | awk -F '[(|)]' '/virtual size/ {print $(NF-1)}' | awk '{print $1}')
[root@undercloud-0 ~]# virsh vol-create-as $CEPH_POOL rhel $DISK_SIZE --format raw
Vol rhel created

[root@undercloud-0 ~]# virsh vol-upload --pool $CEPH_POOL rhel rhel-server-7.8-beta-1-x86_64-kvm.raw
error: cannot upload to volume rhel
error: this function is not supported by the connection driver: storage pool doesn't support volume upload
~~~

