# Resetting libvirt qcow attributes

I have run my instances from an external drive and occasionally it may so happen that I involuntarily disconnect the drive
as I move my laptop from one room to another.

So even after stopping the instance and restarting libvirtd, the result may be the following error message when I try
to start the VM:

```
Setting different SELinux label on /opt/external/libvirt/fedora1.qcow2 which is already in use
```

The reason is that libvirt couldn't remove the following file attributes:

```
$ sudo bash -c "getfattr  -d -m '.*' -- /opt/external/libvirt/fedora1.qcow2"
(...)
trusted.libvirt.security.dac="+1000:+107"
trusted.libvirt.security.ref_dac="1"
trusted.libvirt.security.ref_selinux="1"
trusted.libvirt.security.selinux="system_u:object_r:svirt_image_t:s0:c18,c447"
trusted.libvirt.security.timestamp_dac="1737363344"
trusted.libvirt.security.timestamp_selinux="1737363344"
```

Given that I absolutely know that the Qemu images are not in use, I can clear the libvirt security file attributes to
fix this without having to restart my computer:

```
file=/opt/external/libvirt/fedora2.qcow2; \
    sudo getfattr  -d -m '.*' -- $file | \
    awk -F '=' '/trusted.libvirt.security/ {print $1}' | \
    while read attr; do sudo setfattr -x $attr $file; done
```
