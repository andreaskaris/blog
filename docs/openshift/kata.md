# kata containers and the kata operator #

This post looks at kata containers on top of OpenShift as deployed with the `kata-operator` ([https://github.com/harche/kata-operator](https://github.com/harche/kata-operator)).

## kata-operator ##

### kata-operator will not install ###

At time of this writing, you will have to hack the worker node once the kata-operator and CR were installed and configured the worker: [https://github.com/harche/kata-operator/issues/38](https://github.com/harche/kata-operator/issues/38)

### VM configuration ###

~~~
[root@openshift-worker-2 ~]# ps aux | grep qemu-kvm
root       28450  0.1  0.2 2599904 351472 ?      Sl   Jul26   1:08 /usr/libexec/qemu-kvm -name sandbox-dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 -uuid 6f03e56e-efa1-4f0f-9b8f-492cf1824c83 -machine q35,accel=kvm,kernel_irqchip -cpu host -qmp unix:/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/qmp.sock,server,nowait -m 2048M,slots=10,maxmem=129853M -device pci-bridge,bus=pcie.0,id=pci-bridge-0,chassis_nr=1,shpc=on,addr=2,romfile= -device virtio-serial-pci,disable-modern=false,id=serial0,romfile= -device virtconsole,chardev=charconsole0,id=console0 -chardev socket,id=charconsole0,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/console.sock,server,nowait -device virtio-scsi-pci,id=scsi0,disable-modern=false,romfile= -object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0,romfile= -device vhost-vsock-pci,disable-modern=false,vhostfd=3,id=vsock-555233078,guest-cid=555233078,romfile= -chardev socket,id=char-1082a328d9d79959,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/vhost-fs.sock -device vhost-user-fs-pci,chardev=char-1082a328d9d79959,tag=kataShared,romfile= -netdev tap,id=network-0,vhost=on,vhostfds=4,fds=5 -device driver=virtio-net-pci,netdev=network-0,mac=d6:96:23:1b:02:05,disable-modern=false,mq=on,vectors=4,romfile= -global kvm-pit.lost_tick_policy=discard -vga none -no-user-config -nodefaults -nographic -daemonize -object memory-backend-file,id=dimm1,size=2048M,mem-path=/dev/shm,share=on -numa node,memdev=dimm1 -kernel /usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/vmlinuz -initrd /var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd -append tsc=reliable no_timer_check rcupdate.rcu_expedited=1 i8042.direct=1 i8042.dumbkbd=1 i8042.nopnp=1 i8042.noaux=1 noreplace-smp reboot=k console=hvc0 console=hvc1 iommu=off cryptomgr.notests net.ifnames=0 pci=lastbus=0 quiet panic=1 nr_cpus=40 agent.use_vsock=true scsi_mod.scan=none -pidfile /run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/pid -smp 1,cores=1,threads=1,sockets=40,maxcpus=40
~~~

### Initrd build process ###

kata-operator builds an initrd image with `/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh`. This script is executed by service kata-osbuilder-generate.service upon start and restart. For further details, also see: [https://github.com/kata-containers/osbuilder](https://github.com/kata-containers/osbuilder):
~~~
[root@openshift-worker-2 ~]# systemctl list-unit-files | grep kata
kata-osbuilder-generate.service                                        enabled  
[root@openshift-worker-2 ~]# systemctl status kata-osbuilder-generate.service
● kata-osbuilder-generate.service - Hacky service to enable kata-osbuilder-generate.service
   Loaded: loaded (/etc/systemd/system/kata-osbuilder-generate.service; enabled; vendor preset: disabled)
   Active: inactive (dead) since Sun 2020-07-26 15:54:05 UTC; 18h ago
  Process: 12437 ExecStart=/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh (code=exited, status=0/SUCCESS)
 Main PID: 12437 (code=exited, status=0/SUCCESS)
      CPU: 23.046s

Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Create /etc/resolv.conf file in rootfs if not exist
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Creating summary file
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Created summary file '/var/lib/osbuilder/osbuilder.yaml' inside rootfs
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: + Calling osbuilder initrd_builder.sh
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] init is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] Agent is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Creating /tmp/kata-dracut-images-jeAfIG/kata-containers-initrd.img based on rootfs at /tmp/kata-dracut-rootfs-qPP8lc
Jul 26 15:54:05 openshift-worker-2.example.com kata-osbuilder.sh[12437]: 133725 blocks
Jul 26 15:54:05 openshift-worker-2.example.com systemd[1]: Started Hacky service to enable kata-osbuilder-generate.service.
Jul 26 15:54:05 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Consumed 23.046s CPU time
[root@openshift-worker-2 ~]# cat /etc/systemd/system/kata-osbuilder-generate.service

[Unit]
Description=Hacky service to enable kata-osbuilder-generate.service
ConditionPathExists=/usr/lib/systemd/system/kata-osbuilder-generate.service
[Service]
Type=oneshot
ExecStart=/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
ExecRestart=/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
[Install]
WantedBy=multi-user.target
~~~

~~~
[root@openshift-worker-2 ~]# journalctl -u kata-osbuilder-generate.service | cat
-- Logs begin at Thu 2020-07-23 14:27:02 UTC, end at Mon 2020-07-27 10:12:56 UTC. --
Jul 26 15:46:48 openshift-worker-2.example.com systemd[1]: Starting Hacky service to enable kata-osbuilder-generate.service...
Jul 26 15:46:48 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Main process exited, code=exited, status=203/EXEC
Jul 26 15:46:48 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Failed with result 'exit-code'.
Jul 26 15:46:48 openshift-worker-2.example.com systemd[1]: Failed to start Hacky service to enable kata-osbuilder-generate.service.
Jul 26 15:46:48 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Consumed 944us CPU time
Jul 26 15:53:28 openshift-worker-2.example.com systemd[1]: Starting Hacky service to enable kata-osbuilder-generate.service...
Jul 26 15:53:28 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Main process exited, code=exited, status=203/EXEC
Jul 26 15:53:28 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Failed with result 'exit-code'.
Jul 26 15:53:28 openshift-worker-2.example.com systemd[1]: Failed to start Hacky service to enable kata-osbuilder-generate.service.
Jul 26 15:53:28 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Consumed 1ms CPU time
Jul 26 15:53:34 openshift-worker-2.example.com systemd[1]: /etc/systemd/system/kata-osbuilder-generate.service:8: Unknown lvalue 'ExecRestart' in section 'Service'
Jul 26 15:53:41 openshift-worker-2.example.com systemd[1]: Starting Hacky service to enable kata-osbuilder-generate.service...
Jul 26 15:53:41 openshift-worker-2.example.com kata-osbuilder.sh[12437]: + Building dracut initrd
Jul 26 15:53:41 openshift-worker-2.example.com dracut[12466]: Executing: /usr/bin/dracut --confdir ./dracut/dracut.conf.d --no-compress --conf /dev/null /tmp/kata-dracut-images-jeAfIG/tmp.dHnQ14COyx 4.18.0-193.13.2.el8_2.x86_64
Jul 26 15:53:42 openshift-worker-2.example.com dracut[12466]: *** Including module: bash ***
Jul 26 15:53:42 openshift-worker-2.example.com dracut[12466]: *** Including module: systemd ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including module: rescue ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including module: nss-softokn ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including module: kernel-modules ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including module: udev-rules ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: Skipping udev rule: 91-permissions.rules
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: Skipping udev rule: 80-drivers-modprobe.rules
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including module: syslog ***
Jul 26 15:53:43 openshift-worker-2.example.com kata-osbuilder.sh[12437]: dracut: Could not find any syslog binary although the syslogmodule is selected to be installed. Please check.
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: Could not find any syslog binary although the syslogmodule is selected to be installed. Please check.
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Including modules done ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Installing kernel module dependencies ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Installing kernel module dependencies done ***
Jul 26 15:53:43 openshift-worker-2.example.com dracut[12466]: *** Resolving executable dependencies ***
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: *** Resolving executable dependencies done***
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: Could not find 'strip'. Not stripping the initramfs.
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: *** Store current command line parameters ***
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: *** Creating image file '/tmp/kata-dracut-images-jeAfIG/tmp.dHnQ14COyx' ***
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: Image: /var/tmp/dracut.9gy4Wx/initramfs.img: 39M
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: ========================================================================
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: Version:
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: Arguments: --confdir './dracut/dracut.conf.d' --no-compress --conf '/dev/null'
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: dracut modules:
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: bash
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: systemd
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: rescue
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: nss-softokn
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: kernel-modules
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: udev-rules
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: syslog
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: ========================================================================
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: crw-r--r--   1 root     root       5,   1 Jan  1  1970 dev/console
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: crw-r--r--   1 root     root       1,  11 Jan  1  1970 dev/kmsg
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: crw-r--r--   1 root     root       1,   3 Jan  1  1970 dev/null
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: crw-r--r--   1 root     root       1,   8 Jan  1  1970 dev/random
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: crw-r--r--   1 root     root       1,   9 Jan  1  1970 dev/urandom
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x  11 root     root            0 Jan  1  1970 .
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            7 Jan  1  1970 bin -> usr/bin
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 dev
Jul 26 15:53:44 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   7 root     root            0 Jan  1  1970 etc
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 etc/cmdline.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 etc/conf.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          124 Jan  1  1970 etc/conf.d/systemd.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          116 Jan  1  1970 etc/group
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         4335 Jan  1  1970 etc/ld.so.cache
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           28 Jan  1  1970 etc/ld.so.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 etc/ld.so.conf.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           26 Jan  1  1970 etc/ld.so.conf.d/bind-export-x86_64.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -r--r--r--   1 root     root           67 Jan  1  1970 etc/ld.so.conf.d/kernel-4.18.0-193.13.2.el8_2.x86_64.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root            0 Jan  1  1970 etc/machine-id
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root            0 Jan  1  1970 etc/passwd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 etc/systemd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           64 Jan  1  1970 etc/systemd/journald.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 etc/udev
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 etc/udev/rules.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          297 Jan  1  1970 etc/udev/rules.d/59-persistent-storage.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1030 Jan  1  1970 etc/udev/rules.d/61-persistent-storage.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          215 Jan  1  1970 etc/udev/udev.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1204 Jan  1  1970 etc/virc
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           23 Jan  1  1970 init -> usr/lib/systemd/systemd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            7 Jan  1  1970 lib -> usr/lib
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            9 Jan  1  1970 lib64 -> usr/lib64
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 proc
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           12 Jan  1  1970 root -> var/roothome
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 run
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            8 Jan  1  1970 sbin -> usr/sbin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 sys
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 sysroot
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 tmp
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   6 root     root            0 Jan  1  1970 usr
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/bin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1596592 Jan  1  1970 usr/bin/bash
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        38504 Jan  1  1970 usr/bin/cat
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        38392 Jan  1  1970 usr/bin/echo
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        22496 Jan  1  1970 usr/bin/free
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       193768 Jan  1  1970 usr/bin/grep
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        78712 Jan  1  1970 usr/bin/journalctl
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       163744 Jan  1  1970 usr/bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            4 Jan  1  1970 usr/bin/loginctl -> true
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        46112 Jan  1  1970 usr/bin/more
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwsr-xr-x   1 root     root        50456 Jan  1  1970 usr/bin/mount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       180280 Jan  1  1970 usr/bin/netstat
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        80136 Jan  1  1970 usr/bin/ping
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       141240 Jan  1  1970 usr/bin/ps
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        72216 Jan  1  1970 usr/bin/rm
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        33552 Jan  1  1970 usr/bin/rpcinfo
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       171464 Jan  1  1970 usr/bin/scp
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            4 Jan  1  1970 usr/bin/sh -> bash
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1040320 Jan  1  1970 usr/bin/ssh
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1505288 Jan  1  1970 usr/bin/strace
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       215336 Jan  1  1970 usr/bin/systemctl
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16632 Jan  1  1970 usr/bin/systemd-cgls
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16608 Jan  1  1970 usr/bin/systemd-escape
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        49984 Jan  1  1970 usr/bin/systemd-run
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        74936 Jan  1  1970 usr/bin/systemd-tmpfiles
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        34288 Jan  1  1970 usr/bin/true
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       434592 Jan  1  1970 usr/bin/udevadm
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwsr-xr-x   1 root     root        33640 Jan  1  1970 usr/bin/umount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        38384 Jan  1  1970 usr/bin/uname
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1416744 Jan  1  1970 usr/bin/vi
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   9 root     root            0 Jan  1  1970 usr/lib
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/dracut
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           68 Jan  1  1970 usr/lib/dracut/build-parameter.txt
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x  15 root     root            0 Jan  1  1970 usr/lib/dracut/hooks
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/cleanup
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/cmdline
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/emergency
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   6 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/initqueue
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/initqueue/finished
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/initqueue/online
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/initqueue/settled
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/initqueue/timeout
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/mount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/netroot
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/pre-mount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/pre-pivot
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/pre-shutdown
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/pre-trigger
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/pre-udev
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/shutdown
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/dracut/hooks/shutdown-emergency
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           65 Jan  1  1970 usr/lib/dracut/modules.txt
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root            0 Jan  1  1970 usr/lib/dracut/need-initqueue
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modprobe.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          382 Jan  1  1970 usr/lib/modprobe.d/dist-alsa.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          884 Jan  1  1970 usr/lib/modprobe.d/dist-blacklist.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          559 Jan  1  1970 usr/lib/modprobe.d/libmlx4.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          765 Jan  1  1970 usr/lib/modprobe.d/systemd.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   7 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/arch
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/arch/x86
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/arch/x86/crypto
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/arch/x86/crypto/sha256-mb
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         9256 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/arch/x86/crypto/sha256-mb/sha256-mb.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/crypto
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         6756 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/crypto/mcryptd.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   6 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/block
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         8924 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/block/virtio_blk.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/char
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        14796 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/char/virtio_console.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/net
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         6776 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/net/net_failover.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        24512 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/net/virtio_net.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/scsi
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        20836 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/scsi/sg.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         8744 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/drivers/scsi/virtio_scsi.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/fs
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/fs/fuse
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        57012 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/fs/fuse/fuse.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        11132 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/fs/fuse/virtiofs.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   4 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/core
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         4100 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/core/failover.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/vmw_vsock
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         6884 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/vmw_vsock/vmw_vsock_virtio_transport.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        12144 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/vmw_vsock/vmw_vsock_virtio_transport_common.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root        13696 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/kernel/net/vmw_vsock/vsock.ko.xz
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          552 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.alias
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          777 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.alias.bin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         7534 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.builtin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         9748 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.builtin.bin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          827 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.dep
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1374 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.dep.bin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           70 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.devname
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root       100570 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.order
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root           55 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.softdep
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         5544 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.symbols
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         5720 Jan  1  1970 usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/modules.symbols.bin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/sysctl.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          122 Jan  1  1970 usr/lib/sysctl.d/10-coreos-ratelimit-kmsg.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1810 Jan  1  1970 usr/lib/sysctl.d/10-default-yama-scope.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          524 Jan  1  1970 usr/lib/sysctl.d/50-coredump.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1270 Jan  1  1970 usr/lib/sysctl.d/50-default.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          246 Jan  1  1970 usr/lib/sysctl.d/50-libkcapi-optmem_max.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          636 Jan  1  1970 usr/lib/sysctl.d/50-pid-max.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   5 root     root            0 Jan  1  1970 usr/lib/systemd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      2707624 Jan  1  1970 usr/lib/systemd/libsystemd-shared-239.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/network
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          412 Jan  1  1970 usr/lib/systemd/network/99-default.link
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   8 root     root            0 Jan  1  1970 usr/lib/systemd/system
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system-generators
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16648 Jan  1  1970 usr/lib/systemd/system-generators/systemd-debug-generator
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        46080 Jan  1  1970 usr/lib/systemd/system-generators/systemd-fstab-generator
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1024 Jan  1  1970 usr/lib/systemd/system/basic.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          412 Jan  1  1970 usr/lib/systemd/system/cryptsetup.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           13 Jan  1  1970 usr/lib/systemd/system/ctrl-alt-del.target -> reboot.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1084 Jan  1  1970 usr/lib/systemd/system/debug-shell.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib/systemd/system/default.target -> multi-user.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          471 Jan  1  1970 usr/lib/systemd/system/emergency.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/emergency.target.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           33 Jan  1  1970 usr/lib/systemd/system/emergency.target.wants/systemd-vconsole-setup.service -> ../systemd-vconsole-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          480 Jan  1  1970 usr/lib/systemd/system/final.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          527 Jan  1  1970 usr/lib/systemd/system/halt.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          541 Jan  1  1970 usr/lib/systemd/system/kexec.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          721 Jan  1  1970 usr/lib/systemd/system/kmod-static-nodes.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          435 Jan  1  1970 usr/lib/systemd/system/local-fs-pre.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          547 Jan  1  1970 usr/lib/systemd/system/local-fs.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          532 Jan  1  1970 usr/lib/systemd/system/multi-user.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          505 Jan  1  1970 usr/lib/systemd/system/network-online.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          502 Jan  1  1970 usr/lib/systemd/system/network-pre.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          521 Jan  1  1970 usr/lib/systemd/system/network.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          554 Jan  1  1970 usr/lib/systemd/system/nss-lookup.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          513 Jan  1  1970 usr/lib/systemd/system/nss-user-lookup.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          394 Jan  1  1970 usr/lib/systemd/system/paths.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          592 Jan  1  1970 usr/lib/systemd/system/poweroff.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          583 Jan  1  1970 usr/lib/systemd/system/reboot.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          436 Jan  1  1970 usr/lib/systemd/system/remote-fs-pre.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          522 Jan  1  1970 usr/lib/systemd/system/remote-fs.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          492 Jan  1  1970 usr/lib/systemd/system/rescue.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/rescue.target.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           33 Jan  1  1970 usr/lib/systemd/system/rescue.target.wants/systemd-vconsole-setup.service -> ../systemd-vconsole-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          540 Jan  1  1970 usr/lib/systemd/system/rpcbind.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          442 Jan  1  1970 usr/lib/systemd/system/shutdown.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          402 Jan  1  1970 usr/lib/systemd/system/sigpwr.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          449 Jan  1  1970 usr/lib/systemd/system/slices.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          396 Jan  1  1970 usr/lib/systemd/system/sockets.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/sockets.target.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           34 Jan  1  1970 usr/lib/systemd/system/sockets.target.wants/systemd-journald-dev-log.socket -> ../systemd-journald-dev-log.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           26 Jan  1  1970 usr/lib/systemd/system/sockets.target.wants/systemd-journald.socket -> ../systemd-journald.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           31 Jan  1  1970 usr/lib/systemd/system/sockets.target.wants/systemd-udevd-control.socket -> ../systemd-udevd-control.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           30 Jan  1  1970 usr/lib/systemd/system/sockets.target.wants/systemd-udevd-kernel.socket -> ../systemd-udevd-kernel.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          393 Jan  1  1970 usr/lib/systemd/system/swap.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          767 Jan  1  1970 usr/lib/systemd/system/sys-kernel-config.mount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          558 Jan  1  1970 usr/lib/systemd/system/sysinit.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           28 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/kmod-static-nodes.service -> ../kmod-static-nodes.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           36 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-ask-password-console.path -> ../systemd-ask-password-console.path
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           27 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-journald.service -> ../systemd-journald.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           31 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-modules-load.service -> ../systemd-modules-load.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           25 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-sysctl.service -> ../systemd-sysctl.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           37 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-tmpfiles-setup-dev.service -> ../systemd-tmpfiles-setup-dev.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           33 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-tmpfiles-setup.service -> ../systemd-tmpfiles-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           31 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-udev-trigger.service -> ../systemd-udev-trigger.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           24 Jan  1  1970 usr/lib/systemd/system/sysinit.target.wants/systemd-udevd.service -> ../systemd-udevd.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1407 Jan  1  1970 usr/lib/systemd/system/syslog.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          704 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-console.path
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          728 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-console.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-console.service.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           33 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-console.service.wants/systemd-vconsole-setup.service -> ../systemd-vconsole-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-plymouth.service.wants
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           33 Jan  1  1970 usr/lib/systemd/system/systemd-ask-password-plymouth.service.wants/systemd-vconsole-setup.service -> ../systemd-vconsole-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          671 Jan  1  1970 usr/lib/systemd/system/systemd-fsck@.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          588 Jan  1  1970 usr/lib/systemd/system/systemd-halt.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          647 Jan  1  1970 usr/lib/systemd/system/systemd-journald-audit.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1130 Jan  1  1970 usr/lib/systemd/system/systemd-journald-dev-log.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1537 Jan  1  1970 usr/lib/systemd/system/systemd-journald.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          882 Jan  1  1970 usr/lib/systemd/system/systemd-journald.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          601 Jan  1  1970 usr/lib/systemd/system/systemd-kexec.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1011 Jan  1  1970 usr/lib/systemd/system/systemd-modules-load.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          597 Jan  1  1970 usr/lib/systemd/system/systemd-poweroff.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          800 Jan  1  1970 usr/lib/systemd/system/systemd-random-seed.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          592 Jan  1  1970 usr/lib/systemd/system/systemd-reboot.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          697 Jan  1  1970 usr/lib/systemd/system/systemd-sysctl.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          771 Jan  1  1970 usr/lib/systemd/system/systemd-tmpfiles-setup-dev.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          751 Jan  1  1970 usr/lib/systemd/system/systemd-tmpfiles-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          867 Jan  1  1970 usr/lib/systemd/system/systemd-udev-settle.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          771 Jan  1  1970 usr/lib/systemd/system/systemd-udev-trigger.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          635 Jan  1  1970 usr/lib/systemd/system/systemd-udevd-control.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          610 Jan  1  1970 usr/lib/systemd/system/systemd-udevd-kernel.socket
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1070 Jan  1  1970 usr/lib/systemd/system/systemd-udevd.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          622 Jan  1  1970 usr/lib/systemd/system/systemd-vconsole-setup.service
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          445 Jan  1  1970 usr/lib/systemd/system/timers.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          457 Jan  1  1970 usr/lib/systemd/system/umount.target
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1559672 Jan  1  1970 usr/lib/systemd/systemd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        12504 Jan  1  1970 usr/lib/systemd/systemd-cgroups-agent
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        62488 Jan  1  1970 usr/lib/systemd/systemd-coredump
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        25192 Jan  1  1970 usr/lib/systemd/systemd-fsck
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       153912 Jan  1  1970 usr/lib/systemd/systemd-journald
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        21192 Jan  1  1970 usr/lib/systemd/systemd-modules-load
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        12512 Jan  1  1970 usr/lib/systemd/systemd-reply-password
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        62504 Jan  1  1970 usr/lib/systemd/systemd-shutdown
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16640 Jan  1  1970 usr/lib/systemd/systemd-sysctl
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       418112 Jan  1  1970 usr/lib/systemd/systemd-udevd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        21104 Jan  1  1970 usr/lib/systemd/systemd-vconsole-setup
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/tmpfiles.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1676 Jan  1  1970 usr/lib/tmpfiles.d/systemd.conf
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   3 root     root            0 Jan  1  1970 usr/lib/udev
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16592 Jan  1  1970 usr/lib/udev/ata_id
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        33312 Jan  1  1970 usr/lib/udev/cdrom_id
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib/udev/rules.d
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         1834 Jan  1  1970 usr/lib/udev/rules.d/40-redhat.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         3679 Jan  1  1970 usr/lib/udev/rules.d/50-udev-default.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          626 Jan  1  1970 usr/lib/udev/rules.d/60-block.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         6528 Jan  1  1970 usr/lib/udev/rules.d/60-persistent-storage.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         2671 Jan  1  1970 usr/lib/udev/rules.d/70-uaccess.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         2758 Jan  1  1970 usr/lib/udev/rules.d/71-seat.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          636 Jan  1  1970 usr/lib/udev/rules.d/73-seat-late.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          452 Jan  1  1970 usr/lib/udev/rules.d/75-net-description.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          615 Jan  1  1970 usr/lib/udev/rules.d/80-drivers.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          292 Jan  1  1970 usr/lib/udev/rules.d/80-net-setup-link.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          510 Jan  1  1970 usr/lib/udev/rules.d/90-vconsole.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root         4367 Jan  1  1970 usr/lib/udev/rules.d/99-systemd.rules
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        34072 Jan  1  1970 usr/lib/udev/scsi_id
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/lib64
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       252288 Jan  1  1970 usr/lib64/ld-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           10 Jan  1  1970 usr/lib64/ld-linux-x86-64.so.2 -> ld-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libacl.so.1 -> libacl.so.1.1.2253
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        54816 Jan  1  1970 usr/lib64/libacl.so.1.1.2253
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           19 Jan  1  1970 usr/lib64/libattr.so.1 -> libattr.so.1.1.2448
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        26192 Jan  1  1970 usr/lib64/libattr.so.1.1.2448
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libaudit.so.1 -> libaudit.so.1.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       123336 Jan  1  1970 usr/lib64/libaudit.so.1.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libblkid.so.1 -> libblkid.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       339272 Jan  1  1970 usr/lib64/libblkid.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           15 Jan  1  1970 usr/lib64/libbz2.so.1 -> libbz2.so.1.0.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        74744 Jan  1  1970 usr/lib64/libbz2.so.1.0.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      3149120 Jan  1  1970 usr/lib64/libc-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           12 Jan  1  1970 usr/lib64/libc.so.6 -> libc-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libcap-ng.so.0 -> libcap-ng.so.0.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        30256 Jan  1  1970 usr/lib64/libcap-ng.so.0.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/lib64/libcap.so.2 -> libcap.so.2.26
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        37952 Jan  1  1970 usr/lib64/libcap.so.2.26
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libcom_err.so.2 -> libcom_err.so.2.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        17336 Jan  1  1970 usr/lib64/libcom_err.so.2.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libcrypt.so.1 -> libcrypt.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       142712 Jan  1  1970 usr/lib64/libcrypt.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           19 Jan  1  1970 usr/lib64/libcrypto.so.1.1 -> libcrypto.so.1.1.1c
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      3058976 Jan  1  1970 usr/lib64/libcrypto.so.1.1.1c
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           23 Jan  1  1970 usr/lib64/libcryptsetup.so.12 -> libcryptsetup.so.12.5.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       485432 Jan  1  1970 usr/lib64/libcryptsetup.so.12.5.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -r-xr-xr-x   1 root     root       371736 Jan  1  1970 usr/lib64/libdevmapper.so.1.02
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        28968 Jan  1  1970 usr/lib64/libdl-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           13 Jan  1  1970 usr/lib64/libdl.so.2 -> libdl-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       658352 Jan  1  1970 usr/lib64/libdw-0.178.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/lib64/libdw.so.1 -> libdw-0.178.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           13 Jan  1  1970 usr/lib64/libe2p.so.2 -> libe2p.so.2.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        38896 Jan  1  1970 usr/lib64/libe2p.so.2.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       104552 Jan  1  1970 usr/lib64/libelf-0.178.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           15 Jan  1  1970 usr/lib64/libelf.so.1 -> libelf-0.178.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libext2fs.so.2 -> libext2fs.so.2.4
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       428784 Jan  1  1970 usr/lib64/libext2fs.so.2.4
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           21 Jan  1  1970 usr/lib64/libfipscheck.so.1 -> libfipscheck.so.1.2.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        12200 Jan  1  1970 usr/lib64/libfipscheck.so.1.2.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        12112 Jan  1  1970 usr/lib64/libfreebl3.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rw-r--r--   1 root     root          899 Jan  1  1970 usr/lib64/libfreeblpriv3.chk
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       552696 Jan  1  1970 usr/lib64/libfreeblpriv3.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        99688 Jan  1  1970 usr/lib64/libgcc_s-8-20191121.so.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           24 Jan  1  1970 usr/lib64/libgcc_s.so.1 -> libgcc_s-8-20191121.so.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           19 Jan  1  1970 usr/lib64/libgcrypt.so.20 -> libgcrypt.so.20.2.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1385560 Jan  1  1970 usr/lib64/libgcrypt.so.20.2.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           22 Jan  1  1970 usr/lib64/libgpg-error.so.0 -> libgpg-error.so.0.24.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       144392 Jan  1  1970 usr/lib64/libgpg-error.so.0.24.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           21 Jan  1  1970 usr/lib64/libgssapi_krb5.so.2 -> libgssapi_krb5.so.2.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       335040 Jan  1  1970 usr/lib64/libgssapi_krb5.so.2.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libidn2.so.0 -> libidn2.so.0.3.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       162224 Jan  1  1970 usr/lib64/libidn2.so.0.3.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libip4tc.so.2 -> libip4tc.so.2.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        33240 Jan  1  1970 usr/lib64/libip4tc.so.2.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libjson-c.so.4 -> libjson-c.so.4.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        70432 Jan  1  1970 usr/lib64/libjson-c.so.4.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libk5crypto.so.3 -> libk5crypto.so.3.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       117216 Jan  1  1970 usr/lib64/libk5crypto.so.3.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libkeyutils.so.1 -> libkeyutils.so.1.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        16240 Jan  1  1970 usr/lib64/libkeyutils.so.1.6
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libkmod.so.2 -> libkmod.so.2.3.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       100200 Jan  1  1970 usr/lib64/libkmod.so.2.3.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/lib64/libkrb5.so.3 -> libkrb5.so.3.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       997600 Jan  1  1970 usr/lib64/libkrb5.so.3.3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           21 Jan  1  1970 usr/lib64/libkrb5support.so.0 -> libkrb5support.so.0.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        71624 Jan  1  1970 usr/lib64/libkrb5support.so.0.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           15 Jan  1  1970 usr/lib64/liblz4.so.1 -> liblz4.so.1.8.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        95728 Jan  1  1970 usr/lib64/liblz4.so.1.8.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/liblzma.so.5 -> liblzma.so.5.2.4
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       192016 Jan  1  1970 usr/lib64/liblzma.so.5.2.4
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      2191792 Jan  1  1970 usr/lib64/libm-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           12 Jan  1  1970 usr/lib64/libm.so.6 -> libm-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libmount.so.1 -> libmount.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       371208 Jan  1  1970 usr/lib64/libmount.so.1.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        76872 Jan  1  1970 usr/lib64/libnss_files-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           20 Jan  1  1970 usr/lib64/libnss_files.so.2 -> libnss_files-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libpam.so.0 -> libpam.so.0.84.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        66272 Jan  1  1970 usr/lib64/libpam.so.0.84.2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libpcap.so.1 -> libpcap.so.1.9.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       356920 Jan  1  1970 usr/lib64/libpcap.so.1.9.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libpcre.so.1 -> libpcre.so.1.2.10
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       471224 Jan  1  1970 usr/lib64/libpcre.so.1.2.10
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           19 Jan  1  1970 usr/lib64/libpcre2-8.so.0 -> libpcre2-8.so.0.7.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       553480 Jan  1  1970 usr/lib64/libpcre2-8.so.0.7.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libprocps.so.7 -> libprocps.so.7.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        86912 Jan  1  1970 usr/lib64/libprocps.so.7.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       322816 Jan  1  1970 usr/lib64/libpthread-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           18 Jan  1  1970 usr/lib64/libpthread.so.0 -> libpthread-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       123320 Jan  1  1970 usr/lib64/libresolv-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libresolv.so.2 -> libresolv-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        74976 Jan  1  1970 usr/lib64/librt-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           13 Jan  1  1970 usr/lib64/librt.so.1 -> librt-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           19 Jan  1  1970 usr/lib64/libseccomp.so.2 -> libseccomp.so.2.4.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       357336 Jan  1  1970 usr/lib64/libseccomp.so.2.4.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       172640 Jan  1  1970 usr/lib64/libselinux.so.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       766120 Jan  1  1970 usr/lib64/libsepol.so.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libssl.so.1.1 -> libssl.so.1.1.1c
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       615504 Jan  1  1970 usr/lib64/libssl.so.1.1.1c
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           20 Jan  1  1970 usr/lib64/libsystemd.so.0 -> libsystemd.so.0.23.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1354792 Jan  1  1970 usr/lib64/libsystemd.so.0.23.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           15 Jan  1  1970 usr/lib64/libtinfo.so.6 -> libtinfo.so.6.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       208616 Jan  1  1970 usr/lib64/libtinfo.so.6.1
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libtirpc.so.3 -> libtirpc.so.3.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       371032 Jan  1  1970 usr/lib64/libtirpc.so.3.0.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           17 Jan  1  1970 usr/lib64/libudev.so.1 -> libudev.so.1.6.11
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       628696 Jan  1  1970 usr/lib64/libudev.so.1.6.11
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           21 Jan  1  1970 usr/lib64/libunistring.so.2 -> libunistring.so.2.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root      1760264 Jan  1  1970 usr/lib64/libunistring.so.2.1.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        23392 Jan  1  1970 usr/lib64/libutil-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           15 Jan  1  1970 usr/lib64/libutil.so.1 -> libutil-2.28.so
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/lib64/libuuid.so.1 -> libuuid.so.1.3.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        33224 Jan  1  1970 usr/lib64/libuuid.so.1.3.0
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/lib64/libz.so.1 -> libz.so.1.2.11
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        97136 Jan  1  1970 usr/lib64/libz.so.1.2.11
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 usr/sbin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        97008 Jan  1  1970 usr/sbin/blkid
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/depmod -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       336512 Jan  1  1970 usr/sbin/e2fsck
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        54872 Jan  1  1970 usr/sbin/fsck
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       336512 Jan  1  1970 usr/sbin/fsck.ext2
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       336512 Jan  1  1970 usr/sbin/fsck.ext3
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root       336512 Jan  1  1970 usr/sbin/fsck.ext4
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        66688 Jan  1  1970 usr/sbin/fsck.fat
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            8 Jan  1  1970 usr/sbin/fsck.vfat -> fsck.fat
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           22 Jan  1  1970 usr/sbin/init -> ../lib/systemd/systemd
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/insmod -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root          193 Jan  1  1970 usr/sbin/insmodpost.sh
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/lsmod -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/modinfo -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/modprobe -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        12320 Jan  1  1970 usr/sbin/nologin
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/ping -> ../bin/ping
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/ping6 -> ../bin/ping
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/sbin/poweroff -> ../bin/systemctl
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           16 Jan  1  1970 usr/sbin/reboot -> ../bin/systemctl
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 usr/sbin/rmmod -> ../bin/kmod
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/sbin/rpcinfo -> ../bin/rpcinfo
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        21672 Jan  1  1970 usr/sbin/showmount
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: -rwxr-xr-x   1 root     root        21360 Jan  1  1970 usr/sbin/swapoff
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           14 Jan  1  1970 usr/sbin/udevadm -> ../bin/udevadm
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   4 root     root            0 Jan  1  1970 var
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root           11 Jan  1  1970 var/lock -> ../run/lock
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwx------   2 root     root            0 Jan  1  1970 var/roothome
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: lrwxrwxrwx   1 root     root            6 Jan  1  1970 var/run -> ../run
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: drwxr-xr-x   2 root     root            0 Jan  1  1970 var/tmp
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: ========================================================================
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: *** Creating initramfs image file '/tmp/kata-dracut-images-jeAfIG/tmp.dHnQ14COyx' done ***
Jul 26 15:53:45 openshift-worker-2.example.com kata-osbuilder.sh[12437]: Warning no default label for /tmp/kata-dracut-images-jeAfIG/tmp.dHnQ14COyx
Jul 26 15:53:45 openshift-worker-2.example.com dracut[12466]: dracut: warning: could not fsfreeze /tmp/kata-dracut-images-jeAfIG
Jul 26 15:53:45 openshift-worker-2.example.com kata-osbuilder.sh[12437]: + Extracting dracut initrd rootfs
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: 78145 blocks
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: + Copying agent directory tree into place
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: Calling osbuilder rootfs.sh on extracted rootfs
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Create symlink to /tmp in /var to create private temporal directories with systemd
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Install tmp.mount in ./etc/systemd/system
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: cp: cannot stat './usr/share/systemd/tmp.mount': No such file or directory
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Create /tmp/kata-dracut-rootfs-qPP8lc/etc
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Configure chrony file /tmp/kata-dracut-rootfs-qPP8lc/etc/chrony.conf
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] cp /usr/libexec/kata-containers/agent/usr/bin/kata-agent /tmp/kata-dracut-rootfs-qPP8lc/usr/bin/kata-agent
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] Agent installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Check init is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] init is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Create /etc/resolv.conf file in rootfs if not exist
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Creating summary file
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Created summary file '/var/lib/osbuilder/osbuilder.yaml' inside rootfs
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: + Calling osbuilder initrd_builder.sh
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] init is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: [OK] Agent is installed
Jul 26 15:53:46 openshift-worker-2.example.com kata-osbuilder.sh[12437]: INFO: Creating /tmp/kata-dracut-images-jeAfIG/kata-containers-initrd.img based on rootfs at /tmp/kata-dracut-rootfs-qPP8lc
Jul 26 15:54:05 openshift-worker-2.example.com kata-osbuilder.sh[12437]: 133725 blocks
Jul 26 15:54:05 openshift-worker-2.example.com systemd[1]: Started Hacky service to enable kata-osbuilder-generate.service.
Jul 26 15:54:05 openshift-worker-2.example.com systemd[1]: kata-osbuilder-generate.service: Consumed 23.046s CPU time
[root@openshift-worker-2 ~]# 
~~~

~~~
[root@openshift-worker-2 ~]# grep IMAGE_TOPDIR /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
readonly IMAGE_TOPDIR="/var/cache/kata-containers"
readonly KERNEL_SYMLINK="${IMAGE_TOPDIR}/vmlinuz.container"
stable symlink paths in ${IMAGE_TOPDIR}
    local image_osbuilder_dir="${IMAGE_TOPDIR}/osbuilder-images"
    local image_dest_link="${IMAGE_TOPDIR}/kata-containers.img"
    ln -sf ${initrd_dest_path} ${IMAGE_TOPDIR}/kata-containers-initrd.img
[root@openshift-worker-2 ~]# grep image_osbuilder_dir
^C
[root@openshift-worker-2 ~]# grep image_osbuilder_dir /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
    local image_osbuilder_dir="${IMAGE_TOPDIR}/osbuilder-images"
    local image_dir="${image_osbuilder_dir}/$KVERSION"
    rm -rf "${image_osbuilder_dir}"
[root@openshift-worker-2 ~]# grep image_dir /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
    local image_dir="${image_osbuilder_dir}/$KVERSION"
    local initrd_dest_path="${image_dir}/${DISTRO}-kata-${KVERSION}.initrd"
    local image_dest_path="${image_dir}/${DISTRO}-kata-${KVERSION}.img"
    mkdir -p "${image_dir}"
[root@openshift-worker-2 ~]# grep initrd_dest_path /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
    local initrd_dest_path="${image_dir}/${DISTRO}-kata-${KVERSION}.initrd"
    mv -Z ${GENERATED_INITRD} ${initrd_dest_path}
    ln -sf ${initrd_dest_path} ${IMAGE_TOPDIR}/kata-containers-initrd.img
[root@openshift-worker-2 ~]# grep GENERATED_INITRD /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
readonly GENERATED_INITRD="${DRACUT_IMAGES}/kata-containers-initrd.img"
    mv -Z ${GENERATED_INITRD} ${initrd_dest_path}
    ./initrd-builder/initrd_builder.sh -o ${GENERATED_INITRD} ${DRACUT_ROOTFS}
[root@openshift-worker-2 ~]# grep image_dest_path /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:    local image_dest_path="${image_dir}/${DISTRO}-kata-${KVERSION}.img"
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:        mv -Z ${GENERATED_IMAGE} ${image_dest_path}
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:        ln -sf ${image_dest_path} ${image_dest_link}
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:    local image_dest_path="${image_dir}/${DISTRO}-kata-${KVERSION}.img"
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:        mv -Z ${GENERATED_IMAGE} ${image_dest_path}
/usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh:        ln -sf ${image_dest_path} ${image_dest_link}
[root@openshift-worker-2 ~]# grep GENERATED_IMAGE /usr/libexec/kata-containers/osbuilder/kata-osbuilder.sh
readonly GENERATED_IMAGE="${DRACUT_IMAGES}/kata-containers.img"
        mv -Z ${GENERATED_IMAGE} ${image_dest_path}
            -o ${GENERATED_IMAGE} ${DRACUT_ROOTFS}
~~~

~~~
[root@openshift-worker-2 ~]# find /var/cache/kata-containers
/var/cache/kata-containers
/var/cache/kata-containers/osbuilder-images
/var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64
/var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd
/var/cache/kata-containers/vmlinuz.container
/var/cache/kata-containers/kata-containers-initrd.img
~~~

### kata configuration - which kernel and initrd does the VM use? ###

Looking at the VM, it uses:
~~~
-kernel /usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/vmlinuz -initrd /var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd
~~~

And following the configuration for kata (and the symlinks):
~~~
[root@openshift-worker-2 ~]# egrep '^kernel' /usr/share/kata-containers/defaults/configuration.toml
kernel = "/var/cache/kata-containers/vmlinuz.container"
kernel_params = ""
kernel_modules=[]
[root@openshift-worker-2 ~]# egrep '^initrd' /usr/share/kata-containers/defaults/configuration.toml
initrd = "/var/cache/kata-containers/kata-containers-initrd.img"
[root@openshift-worker-2 ~]# ls -al /var/cache/kata-containers/vmlinuz.container
lrwxrwxrwx. 1 root root 50 Jul 26 15:54 /var/cache/kata-containers/vmlinuz.container -> /lib/modules/4.18.0-193.13.2.el8_2.x86_64//vmlinuz
[root@openshift-worker-2 ~]# ls -al /var/cache/kata-containers/kata-containers-initrd.img
lrwxrwxrwx. 1 root root 121 Jul 26 15:54 /var/cache/kata-containers/kata-containers-initrd.img -> '/var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd'
[root@openshift-worker-2 ~]# 
~~~

### kata-runtime ###

For further details, see: [https://github.com/kata-containers/runtime](https://github.com/kata-containers/runtime)

Opened files:
~~~
[root@openshift-worker-2 ~]# strace -e trace=file kata-runtime list 2>&1 | grep open
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib64/libpthread.so.0", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib64/libdl.so.2", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib64/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/sys/kernel/mm/transparent_hugepage/hpage_pmd_size", O_RDONLY) = 3
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib64/libcrypto.so.1.1", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib64/libz.so.1", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/proc/self/uid_map", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/sys/kernel/mm/hugepages", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/dev/null", O_WRONLY|O_CREAT|O_APPEND|O_SYNC|O_CLOEXEC, 0640) = 3
openat(AT_FDCWD, "/usr/share/kata-containers/defaults/configuration.toml", O_RDONLY|O_CLOEXEC) = 5
openat(AT_FDCWD, "/etc//localtime", O_RDONLY) = 6
openat(AT_FDCWD, "/proc/self/uid_map", O_RDONLY|O_CLOEXEC) = 6
openat(AT_FDCWD, "/run/vc/sbs", O_RDONLY|O_CLOEXEC) = 6
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103", O_RDONLY|O_CLOEXEC) = 7
openat(AT_FDCWD, "/var/lib/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/config.json", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/persist.json", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 10
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/persist.json", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 10
openat(AT_FDCWD, "/proc/cpuinfo", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/proc/meminfo", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/containers/storage/overlay-containers/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/userdata/config.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/persist.json", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 10
openat(AT_FDCWD, "/run/containers/storage/overlay-containers/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/userdata/config.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 8
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/persist.json", O_RDONLY|O_CLOEXEC) = 9
openat(AT_FDCWD, "/run/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/persist.json", O_RDONLY|O_CLOEXEC) = 10
~~~

Listing VMs / containers and verifying runtime state:
~~~
[root@openshift-worker-2 ~]# kata-runtime kata-check
System is capable of running Kata Containers
System can currently create Kata Containers
[root@openshift-worker-2 ~]# kata-runtime list
ID                                                                 PID         STATUS      BUNDLE                                                                                                                 CREATED                          OWNER
dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103   -1          running     /run/containers/storage/overlay-containers/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/userdata   2020-07-26T16:04:25.183088432Z   #0
6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a   -1          running     /run/containers/storage/overlay-containers/6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a/userdata   2020-07-26T16:04:25.467048412Z   #0
[root@openshift-worker-2 ~]# ps aux | grep dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103
root       28433  0.0  0.0 878648 30720 ?        Sl   Jul26   0:59 /usr/bin/containerd-shim-kata-v2 -namespace default -address  -publish-binary /usr/bin/crio -id dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103
root       28445  0.0  0.0  68424  5664 ?        S    Jul26   0:00 /usr/libexec/virtiofsd --fd=3 -o source=/run/kata-containers/shared/sandboxes/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shared -o cache=always --syslog -o no_posix_lock -f
root       28450  0.1  0.2 2599904 351472 ?      Sl   Jul26   1:10 /usr/libexec/qemu-kvm -name sandbox-dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 -uuid 6f03e56e-efa1-4f0f-9b8f-492cf1824c83 -machine q35,accel=kvm,kernel_irqchip -cpu host -qmp unix:/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/qmp.sock,server,nowait -m 2048M,slots=10,maxmem=129853M -device pci-bridge,bus=pcie.0,id=pci-bridge-0,chassis_nr=1,shpc=on,addr=2,romfile= -device virtio-serial-pci,disable-modern=false,id=serial0,romfile= -device virtconsole,chardev=charconsole0,id=console0 -chardev socket,id=charconsole0,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/console.sock,server,nowait -device virtio-scsi-pci,id=scsi0,disable-modern=false,romfile= -object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0,romfile= -device vhost-vsock-pci,disable-modern=false,vhostfd=3,id=vsock-555233078,guest-cid=555233078,romfile= -chardev socket,id=char-1082a328d9d79959,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/vhost-fs.sock -device vhost-user-fs-pci,chardev=char-1082a328d9d79959,tag=kataShared,romfile= -netdev tap,id=network-0,vhost=on,vhostfds=4,fds=5 -device driver=virtio-net-pci,netdev=network-0,mac=d6:96:23:1b:02:05,disable-modern=false,mq=on,vectors=4,romfile= -global kvm-pit.lost_tick_policy=discard -vga none -no-user-config -nodefaults -nographic -daemonize -object memory-backend-file,id=dimm1,size=2048M,mem-path=/dev/shm,share=on -numa node,memdev=dimm1 -kernel /usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/vmlinuz -initrd /var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd -append tsc=reliable no_timer_check rcupdate.rcu_expedited=1 i8042.direct=1 i8042.dumbkbd=1 i8042.nopnp=1 i8042.noaux=1 noreplace-smp reboot=k console=hvc0 console=hvc1 iommu=off cryptomgr.notests net.ifnames=0 pci=lastbus=0 quiet panic=1 nr_cpus=40 agent.use_vsock=true scsi_mod.scan=none -pidfile /run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/pid -smp 1,cores=1,threads=1,sockets=40,maxcpus=40
root       28454  0.0  0.0 4215264 17304 ?       Sl   Jul26   0:00 /usr/libexec/virtiofsd --fd=3 -o source=/run/kata-containers/shared/sandboxes/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shared -o cache=always --syslog -o no_posix_lock -f
root     1366797  0.0  0.0  12920  2360 pts/0    S+   10:41   0:00 grep --color=auto dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103
[root@openshift-worker-2 ~]# 
~~~

And to verify the kata-runtime configuration:
~~~
[root@openshift-worker-2 ~]# kata-runtime --kata-show-default-config-paths
/etc/kata-containers/configuration.toml
/usr/share/kata-containers/defaults/configuration.toml
[root@openshift-worker-2 ~]# ls /etc/kata-containers
ls: cannot access '/etc/kata-containers': No such file or directory
[root@openshift-worker-2 ~]#  kata-runtime kata-env
[Meta]
  Version = "1.0.24"

[Runtime]
  Debug = false
  Trace = false
  DisableGuestSeccomp = true
  DisableNewNetNs = false
  SandboxCgroupOnly = true
  Path = "/usr/bin/kata-runtime"
  [Runtime.Version]
    OCI = "1.0.1-dev"
    [Runtime.Version.Version]
      Semver = "1.11.1"
      Major = 1
      Minor = 11
      Patch = 1
      Commit = ""
  [Runtime.Config]
    Path = "/usr/share/kata-containers/defaults/configuration.toml"

[Hypervisor]
  MachineType = "q35"
  Version = "QEMU emulator version 4.2.0 (qemu-kvm-4.2.0-19.el8)\nCopyright (c) 2003-2019 Fabrice Bellard and the QEMU Project developers"
  Path = "/usr/libexec/qemu-kvm"
  BlockDeviceDriver = "virtio-scsi"
  EntropySource = "/dev/urandom"
  SharedFS = "virtio-fs"
  VirtioFSDaemon = "/usr/libexec/virtiofsd"
  Msize9p = 8192
  MemorySlots = 10
  PCIeRootPort = 0
  HotplugVFIOOnRootBus = false
  Debug = false
  UseVSock = true

[Image]
  Path = ""

[Kernel]
  Path = "/usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/vmlinuz"
  Parameters = "scsi_mod.scan=none"

[Initrd]
  Path = "/var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/\"rhcos\"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd"

[Proxy]
  Type = "noProxy"
  Path = ""
  Debug = false
  [Proxy.Version]
    Semver = ""
    Major = 0
    Minor = 0
    Patch = 0
    Commit = ""

[Shim]
  Type = "kataShim"
  Path = "/usr/libexec/kata-containers/kata-shim"
  Debug = false
  [Shim.Version]
    Semver = "1.11.1"
    Major = 1
    Minor = 11
    Patch = 1
    Commit = "<<unknown>>"

[Agent]
  Type = "kata"
  Debug = false
  Trace = false
  TraceMode = ""
  TraceType = ""

[Host]
  Kernel = "4.18.0-193.13.2.el8_2.x86_64"
  Architecture = "amd64"
  VMContainerCapable = true
  SupportVSocks = true
  [Host.Distro]
    Name = "Red Hat Enterprise Linux CoreOS"
    Version = "4.5"
  [Host.CPU]
    Vendor = "GenuineIntel"
    Model = "Intel(R) Xeon(R) CPU E5-2630 v4 @ 2.20GHz"

[Netmon]
  Path = "/usr/libexec/kata-containers/kata-netmon"
  Debug = false
  Enable = false
  [Netmon.Version]
    Semver = "1.11.1"
    Major = 1
    Minor = 11
    Patch = 1
    Commit = "<<unknown>>"
~~~

And for logging:
~~~
[root@openshift-worker-2 ~]# journalctl -t kata-runtime
-- Logs begin at Thu 2020-07-23 14:27:02 UTC, end at Mon 2020-07-27 10:47:03 UTC. --
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.151041313Z" level=info msg="loaded configuration" arch=amd64 command=list file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=1356869>
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.151232547Z" level=info msg="vsock supported" arch=amd64 command=list name=kata-runtime pid=1356869 source=katautils
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.151287033Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=list name=kata-runtime pid=1356869 source=katautils
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.151325928Z" level=info arch=amd64 arguments="\"list\"" command=list commit= name=kata-runtime pid=1356869 source=runtime version=1.11.1
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.151435298Z" level=info msg="fetch sandbox" arch=amd64 command=list name=kata-runtime pid=1356869 source=virtcontainers
Jul 27 10:33:25 openshift-worker-2.example.com kata-runtime[1356869]: time="2020-07-27T10:33:25.206438572Z" level=warning msg="failed to get sandbox config from old store: open /var/lib/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/config.json: >
Jul 27 10:34:13 openshift-worker-2.example.com kata-runtime[1357888]: time="2020-07-27T10:34:13.491790968Z" level=info msg="loaded configuration" arch=amd64 command=state file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=135788>
Jul 27 10:34:13 openshift-worker-2.example.com kata-runtime[1357888]: time="2020-07-27T10:34:13.491979419Z" level=info msg="vsock supported" arch=amd64 command=state name=kata-runtime pid=1357888 source=katautils
Jul 27 10:34:13 openshift-worker-2.example.com kata-runtime[1357888]: time="2020-07-27T10:34:13.49202632Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=state name=kata-runtime pid=1357888 source=katautils
Jul 27 10:34:13 openshift-worker-2.example.com kata-runtime[1357888]: time="2020-07-27T10:34:13.492063527Z" level=info arch=amd64 arguments="\"state dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103\"" command=state commit= name=kata-runtime pid=1357888 so>
Jul 27 10:34:13 openshift-worker-2.example.com kata-runtime[1357888]: time="2020-07-27T10:34:13.492166408Z" level=error msg="Container ID (dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103) does not exist" arch=amd64 command=state container=dc7a95dc3219fa5>
Jul 27 10:34:29 openshift-worker-2.example.com kata-runtime[1358196]: time="2020-07-27T10:34:29.677420912Z" level=info msg="loaded configuration" arch=amd64 command=state file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=135819>
Jul 27 10:34:29 openshift-worker-2.example.com kata-runtime[1358196]: time="2020-07-27T10:34:29.677612114Z" level=info msg="vsock supported" arch=amd64 command=state name=kata-runtime pid=1358196 source=katautils
Jul 27 10:34:29 openshift-worker-2.example.com kata-runtime[1358196]: time="2020-07-27T10:34:29.677660668Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=state name=kata-runtime pid=1358196 source=katautils
Jul 27 10:34:29 openshift-worker-2.example.com kata-runtime[1358196]: time="2020-07-27T10:34:29.677699242Z" level=info arch=amd64 arguments="\"state 6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a\"" command=state commit= name=kata-runtime pid=1358196 so>
Jul 27 10:34:29 openshift-worker-2.example.com kata-runtime[1358196]: time="2020-07-27T10:34:29.677801888Z" level=error msg="Container ID (6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a) does not exist" arch=amd64 command=state container=6b41cdf410d12e7>
Jul 27 10:34:32 openshift-worker-2.example.com kata-runtime[1358313]: time="2020-07-27T10:34:32.680058234Z" level=info msg="loaded configuration" arch=amd64 command=ps file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=1358313 s>
Jul 27 10:34:32 openshift-worker-2.example.com kata-runtime[1358313]: time="2020-07-27T10:34:32.680258087Z" level=info msg="vsock supported" arch=amd64 command=ps name=kata-runtime pid=1358313 source=katautils
Jul 27 10:34:32 openshift-worker-2.example.com kata-runtime[1358313]: time="2020-07-27T10:34:32.680309596Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=ps name=kata-runtime pid=1358313 source=katautils
Jul 27 10:34:32 openshift-worker-2.example.com kata-runtime[1358313]: time="2020-07-27T10:34:32.680345567Z" level=info arch=amd64 arguments="\"ps\"" command=ps commit= name=kata-runtime pid=1358313 source=runtime version=1.11.1
Jul 27 10:34:32 openshift-worker-2.example.com kata-runtime[1358313]: time="2020-07-27T10:34:32.680388502Z" level=error msg="Missing container ID, should at least provide one" arch=amd64 command=ps name=kata-runtime pid=1358313 source=runtime
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.49846456Z" level=info msg="loaded configuration" arch=amd64 command=list file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=1358359 >
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.498647076Z" level=info msg="vsock supported" arch=amd64 command=list name=kata-runtime pid=1358359 source=katautils
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.498689418Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=list name=kata-runtime pid=1358359 source=katautils
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.498725525Z" level=info arch=amd64 arguments="\"list\"" command=list commit= name=kata-runtime pid=1358359 source=runtime version=1.11.1
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.498825313Z" level=info msg="fetch sandbox" arch=amd64 command=list name=kata-runtime pid=1358359 source=virtcontainers
Jul 27 10:34:35 openshift-worker-2.example.com kata-runtime[1358359]: time="2020-07-27T10:34:35.498906984Z" level=warning msg="failed to get sandbox config from old store: open /var/lib/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/config.json: >
Jul 27 10:34:51 openshift-worker-2.example.com kata-runtime[1358694]: time="2020-07-27T10:34:51.733126256Z" level=info msg="loaded configuration" arch=amd64 command=events file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=13586>
Jul 27 10:34:51 openshift-worker-2.example.com kata-runtime[1358694]: time="2020-07-27T10:34:51.73331869Z" level=info msg="vsock supported" arch=amd64 command=events name=kata-runtime pid=1358694 source=katautils
Jul 27 10:34:51 openshift-worker-2.example.com kata-runtime[1358694]: time="2020-07-27T10:34:51.733376204Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=events name=kata-runtime pid=1358694 source=katautils
Jul 27 10:34:51 openshift-worker-2.example.com kata-runtime[1358694]: time="2020-07-27T10:34:51.733416598Z" level=info arch=amd64 arguments="\"events\"" command=events commit= name=kata-runtime pid=1358694 source=runtime version=1.11.1
Jul 27 10:34:51 openshift-worker-2.example.com kata-runtime[1358694]: time="2020-07-27T10:34:51.733458859Z" level=error msg="container id cannot be empty" arch=amd64 command=events name=kata-runtime pid=1358694 source=runtime
Jul 27 10:34:56 openshift-worker-2.example.com kata-runtime[1358808]: time="2020-07-27T10:34:56.767961592Z" level=info msg="loaded configuration" arch=amd64 command=events file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=13588>
Jul 27 10:34:56 openshift-worker-2.example.com kata-runtime[1358808]: time="2020-07-27T10:34:56.768145404Z" level=info msg="vsock supported" arch=amd64 command=events name=kata-runtime pid=1358808 source=katautils
Jul 27 10:34:56 openshift-worker-2.example.com kata-runtime[1358808]: time="2020-07-27T10:34:56.768196771Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=events name=kata-runtime pid=1358808 source=katautils
Jul 27 10:34:56 openshift-worker-2.example.com kata-runtime[1358808]: time="2020-07-27T10:34:56.768235257Z" level=info arch=amd64 arguments="\"events dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103\"" command=events commit= name=kata-runtime pid=1358808 >
Jul 27 10:34:56 openshift-worker-2.example.com kata-runtime[1358808]: time="2020-07-27T10:34:56.768401639Z" level=error msg="Container ID (dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103) does not exist" arch=amd64 command=events container=dc7a95dc3219fa>
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.137798572Z" level=info msg="loaded configuration" arch=amd64 command=list file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=1359217>
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.137979805Z" level=info msg="vsock supported" arch=amd64 command=list name=kata-runtime pid=1359217 source=katautils
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.138022584Z" level=info msg="VSOCK supported, configure to not use proxy" arch=amd64 command=list name=kata-runtime pid=1359217 source=katautils
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.138057713Z" level=info arch=amd64 arguments="\"list\"" command=list commit= name=kata-runtime pid=1359217 source=runtime version=1.11.1
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.138166603Z" level=info msg="fetch sandbox" arch=amd64 command=list name=kata-runtime pid=1359217 source=virtcontainers
Jul 27 10:35:16 openshift-worker-2.example.com kata-runtime[1359217]: time="2020-07-27T10:35:16.138244704Z" level=warning msg="failed to get sandbox config from old store: open /var/lib/vc/sbs/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/config.json: >
Jul 27 10:35:20 openshift-worker-2.example.com kata-runtime[1359329]: time="2020-07-27T10:35:20.836297534Z" level=info msg="loaded configuration" arch=amd64 command=state file=/usr/share/kata-containers/defaults/configuration.toml format=TOML name=kata-runtime pid=135932>
Jul 27 10:35:20 openshift-worker-2.example.com kata-runtime[1359329]: time="2020-07-27T10:35:20.836493897Z" level=info msg="vsock supported" arch=amd64 command=state name=kata-runtime pid=1359329 source=katautils
[root@openshift-worker-2 ~]# journalctl -t kata
-- Logs begin at Thu 2020-07-23 14:27:02 UTC, end at Mon 2020-07-27 10:47:59 UTC. --
Jul 26 16:03:42 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:42.108410531Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:42 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:42.172077874Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:46 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:46.380966347Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:46 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:46.444520024Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.458482832Z" level=warning msg="Could not remove container share dir" ID=f3bfda981bbafd53ed8f8e1d29a52e9297db0532ba3c564449b43fe79ec31506 error="no such file or directory" sandbox=f3bfda>
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.493291963Z" level=warning msg="failed to cleanup rootfs mount" error="no such file or directory"
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.525282501Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.597489614Z" level=warning msg="Could not remove container share dir" ID=f3bfda981bbafd53ed8f8e1d29a52e9297db0532ba3c564449b43fe79ec31506 error="no such file or directory" sandbox=f3bfda>
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.599932291Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.631820928Z" level=error msg="Could not read qemu pid file" ID=f3bfda981bbafd53ed8f8e1d29a52e9297db0532ba3c564449b43fe79ec31506 error="open /run/vc/vm/f3bfda981bbafd53ed8f8e1d29a52e9297d>
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.632319378Z" level=error msg="Could not read qemu pid file" ID=f3bfda981bbafd53ed8f8e1d29a52e9297db0532ba3c564449b43fe79ec31506 error="open /run/vc/vm/f3bfda981bbafd53ed8f8e1d29a52e9297d>
Jul 26 16:03:53 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:03:53.735993003Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:04:25 openshift-worker-2.example.com kata[28433]: time="2020-07-26T16:04:25.25620997Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:04:25 openshift-worker-2.example.com kata[28433]: time="2020-07-26T16:04:25.324699487Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:04:25 openshift-worker-2.example.com kata[28433]: time="2020-07-26T16:04:25.532722656Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:04:25 openshift-worker-2.example.com kata[28433]: time="2020-07-26T16:04:25.59569864Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 26 16:04:37 openshift-worker-2.example.com kata[27183]: time="2020-07-26T16:04:37.109804951Z" level=warning msg="failed to cleanup rootfs mount" error="no such file or directory"
Jul 27 09:22:48 openshift-worker-2.example.com kata[28433]: time="2020-07-27T09:22:48.313239015Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 27 09:22:48 openshift-worker-2.example.com kata[28433]: time="2020-07-27T09:22:48.461956579Z" level=error msg="post event" error="failed to publish event: exit status 1"
Jul 27 09:23:05 openshift-worker-2.example.com kata[28433]: time="2020-07-27T09:23:05.940817814Z" level=error msg="post event" error="failed to publish event: exit status 1"
[root@openshift-worker-2 ~]# 
~~~

### How does kata containers plug the VM networking ###

The namespace looks like this:
~~~
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
3: eth0@if18: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1400 qdisc noqueue state UP group default qlen 1000
    link/ether d6:96:23:1b:02:05 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.27.2.4/23 brd 172.27.3.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::bce2:c3ff:feca:293b/64 scope link 
       valid_lft forever preferred_lft forever
4: tap0_kata: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1400 qdisc mq state UNKNOWN group default qlen 1000
    link/ether 9a:b6:22:f7:1c:26 brd ff:ff:ff:ff:ff:ff
    inet6 fe80::98b6:22ff:fef7:1c26/64 scope link 
       valid_lft forever preferred_lft forever
~~~

And the VM's tap configuration looks like this, using ifindex 4 within the namespace:
~~~
-netdev tap,id=network-0,vhost=on,vhostfds=4,fds=5
~~~

All traffic that arrives on tap0_kata will be mirrored to eth0 and vise versa. In the below case, the VM sends TCP SYNs. These SYNs enter on tap0_kata and are mirrored to eth0:
~~~
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tcpdump -nne -i tap0_kata 
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on tap0_kata, link-type EN10MB (Ethernet), capture size 262144 bytes
16:49:30.018482 d6:96:23:1b:02:05 > 0a:58:ac:1b:02:01, ethertype IPv4 (0x0800), length 74: 172.27.2.4.44268 > 172.30.0.10.80: Flags [S], seq 2995123630, win 27200, options [mss 1360,sackOK,TS val 3516836732 ecr 0,nop,wscale 7], length 0
16:49:31.038346 d6:96:23:1b:02:05 > 0a:58:ac:1b:02:01, ethertype IPv4 (0x0800), length 74: 172.27.2.4.44270 > 172.30.0.10.80: Flags [S], seq 1381681461, win 27200, options [mss 1360,sackOK,TS val 3516837752 ecr 0,nop,wscale 7], length 0
^C
2 packets captured
2 packets received by filter
0 packets dropped by kernel
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tcpdump -nne -i eth0     
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
16:49:40.197817 d6:96:23:1b:02:05 > 0a:58:ac:1b:02:01, ethertype IPv4 (0x0800), length 74: 172.27.2.4.44288 > 172.30.0.10.80: Flags [S], seq 712892146, win 27200, options [mss 1360,sackOK,TS val 3516846912 ecr 0,nop,wscale 7], length 0
16:49:41.215135 d6:96:23:1b:02:05 > 0a:58:ac:1b:02:01, ethertype IPv4 (0x0800), length 74: 172.27.2.4.44290 > 172.30.0.10.80: Flags [S], seq 3944941969, win 27200, options [mss 1360,sackOK,TS val 3516847929 ecr 0,nop,wscale 7], length 0
^C
2 packets captured
2 packets received by filter
0 packets dropped by kernel
sh-4.4# 
~~~

The kata documentation explains how this works: [https://github.com/kata-containers/documentation/blob/master/design/architecture.md#networking](https://github.com/kata-containers/documentation/blob/master/design/architecture.md#networking)
~~~
Containers will typically live in their own, possibly shared, networking namespace. At some point in a container lifecycle, container engines will set up that namespace to add the container to a network which is isolated from the host network, but which is shared between containers

In order to do so, container engines will usually add one end of a virtual ethernet (veth) pair into the container networking namespace. The other end of the veth pair is added to the host networking namespace.

This is a very namespace-centric approach as many hypervisors (in particular QEMU) cannot handle veth interfaces. Typically, TAP interfaces are created for VM connectivity.

To overcome incompatibility between typical container engines expectations and virtual machines, kata-runtime networking transparently connects veth interfaces with TAP ones using MACVTAP:
~~~

See the picture in the documentation for further details. eth0 and tap_kata0 are transparently connected via tc rules:
~~~
# ingress qdisc configured for both interfaces

sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tc qdisc ls dev eth0
qdisc noqueue 0: root refcnt 2 
qdisc ingress ffff: parent ffff:fff1 ---------------- 
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tc qdisc ls dev tap0_kata
qdisc mq 0: root 
qdisc fq_codel 0: parent :1 limit 10240p flows 1024 quantum 1414 target 5.0ms interval 100.0ms memory_limit 32Mb ecn 
qdisc ingress ffff: parent ffff:fff1 ---------------- 

# filter configured for both interfaces
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tc filter ls dev eth0 ingress
filter protocol all pref 49152 u32 chain 0 
filter protocol all pref 49152 u32 chain 0 fh 800: ht divisor 1 
filter protocol all pref 49152 u32 chain 0 fh 800::800 order 2048 key ht 800 bkt 0 terminal flowid ??? not_in_hw 
  match 00000000/00000000 at 0
	action order 1: mirred (Egress Redirect to device tap0_kata) stolen
 	index 1 ref 1 bind 1
 
sh-4.4# ip netns exec 317575d4-1143-4aab-b896-5767221326ec tc filter ls dev tap0_kata ingress
filter protocol all pref 49152 u32 chain 0 
filter protocol all pref 49152 u32 chain 0 fh 800: ht divisor 1 
filter protocol all pref 49152 u32 chain 0 fh 800::800 order 2048 key ht 800 bkt 0 terminal flowid ??? not_in_hw 
  match 00000000/00000000 at 0
	action order 1: mirred (Egress Redirect to device eth0) stolen
 	index 2 ref 1 bind 1
~~~

Further details about this in: 
* [https://gist.github.com/mcastelino/7d85f4164ffdaf48242f9281bb1d0f9b](https://gist.github.com/mcastelino/7d85f4164ffdaf48242f9281bb1d0f9b)
* [https://man7.org/linux/man-pages/man8/tc-mirred.8.html](https://man7.org/linux/man-pages/man8/tc-mirred.8.html)

### Selecting the appropriate runtime class per container ###

Kubernetes allows users to choose the appropriate runtimeClass for their pod: [https://kubernetes.io/docs/concepts/containers/runtime-class/](https://kubernetes.io/docs/concepts/containers/runtime-class/)

~~~
[root@openshift-jumpserver-0 ~]# oc explain runtimeclass
KIND:     RuntimeClass
VERSION:  node.k8s.io/v1beta1

DESCRIPTION:
     RuntimeClass defines a class of container runtime supported in the cluster.
     The RuntimeClass is used to determine which container runtime is used to
     run all containers in a pod. RuntimeClasses are (currently) manually
     defined by a user or cluster provisioner, and referenced in the PodSpec.
     The Kubelet is responsible for resolving the RuntimeClassName reference
     before running the pod. For more details, see
     https://git.k8s.io/enhancements/keps/sig-node/runtime-class.md

FIELDS:
   apiVersion	<string>
     APIVersion defines the versioned schema of this representation of an
     object. Servers should convert recognized schemas to the latest internal
     value, and may reject unrecognized values. More info:
     https://git.k8s.io/community/contributors/devel/sig-architecture/api-conventions.md#resources

   handler	<string> -required-
     Handler specifies the underlying runtime and configuration that the CRI
     implementation will use to handle pods of this class. The possible values
     are specific to the node & CRI configuration. It is assumed that all
     handlers are available on every node, and handlers of the same name are
     equivalent on every node. For example, a handler called "runc" might
     specify that the runc OCI runtime (using native Linux containers) will be
     used to run the containers in a pod. The Handler must conform to the DNS
     Label (RFC 1123) requirements, and is immutable.

   kind	<string>
     Kind is a string value representing the REST resource this object
     represents. Servers may infer this from the endpoint the client submits
     requests to. Cannot be updated. In CamelCase. More info:
     https://git.k8s.io/community/contributors/devel/sig-architecture/api-conventions.md#types-kinds

   metadata	<Object>
     More info:
     https://git.k8s.io/community/contributors/devel/sig-architecture/api-conventions.md#metadata

   overhead	<Object>
     Overhead represents the resource overhead associated with running a pod for
     a given RuntimeClass. For more details, see
     https://git.k8s.io/enhancements/keps/sig-node/20190226-pod-overhead.md This
     field is alpha-level as of Kubernetes v1.15, and is only honored by servers
     that enable the PodOverhead feature.

   scheduling	<Object>
     Scheduling holds the scheduling constraints to ensure that pods running
     with this RuntimeClass are scheduled to nodes that support it. If
     scheduling is nil, this RuntimeClass is assumed to be supported by all
     nodes.
~~~

The runtime class definition looks like this:
~~~
[root@openshift-jumpserver-0 ~]# oc get runtimeclass kata-oc -o yaml
apiVersion: node.k8s.io/v1beta1
handler: kata-oc
kind: RuntimeClass
metadata:
  creationTimestamp: "2020-07-26T15:48:38Z"
  managedFields:
  - apiVersion: node.k8s.io/v1beta1
    fieldsType: FieldsV1
    fieldsV1:
      f:handler: {}
      f:metadata:
        f:ownerReferences:
          .: {}
          k:{"uid":"e8e76d58-2615-4787-9d4c-7b89dd275131"}:
            .: {}
            f:apiVersion: {}
            f:blockOwnerDeletion: {}
            f:controller: {}
            f:kind: {}
            f:name: {}
            f:uid: {}
      f:scheduling:
        .: {}
        f:nodeSelector:
          .: {}
          f:kata-containers: {}
    manager: kata-operator
    operation: Update
    time: "2020-07-26T15:48:38Z"
  name: kata-oc
  ownerReferences:
  - apiVersion: kataconfiguration.openshift.io/v1alpha1
    blockOwnerDeletion: true
    controller: true
    kind: KataConfig
    name: example-kataconfig
    uid: e8e76d58-2615-4787-9d4c-7b89dd275131
  resourceVersion: "3111152"
  selfLink: /apis/node.k8s.io/v1beta1/runtimeclasses/kata-oc
  uid: bd0fea1a-2d41-43aa-90ba-c9804aabaaa7
scheduling:
  nodeSelector:
    kata-containers: ""
~~~

And the scheduling nodeSelector makes sure that kata pods will only be spawned on correctly configured workers:
~~~
[root@openshift-jumpserver-0 ~]# oc get nodes -l kata-containers=
NAME                             STATUS   ROLES    AGE     VERSION
openshift-worker-2.example.com   Ready    worker   4d19h   v1.18.3+b74c5ed
~~~

On these worker nodes, the kata operator configures the `kata-oc` handler in crio with:
~~~
[root@openshift-worker-2 ~]# cat /etc/crio/crio.conf.d/kata-50.conf 

[crio.runtime]
  manage_ns_lifecycle = true

[crio.runtime.runtimes.kata-oc]
  runtime_path = "/usr/bin/containerd-shim-kata-v2"
  runtime_type = "vm"
  runtime_root = "/run/vc"
  
[crio.runtime.runtimes.runc]
  runtime_path = ""
  runtime_type = "oci"
  runtime_root = "/run/runc"
~~~

And default runtime:
~~~
[root@openshift-worker-2 ~]# grep default_runtime /etc/crio/ -R
/etc/crio/crio.conf:# default_runtime is the _name_ of the OCI runtime to be used as the default.
/etc/crio/crio.conf:default_runtime = "runc"
[root@openshift-worker-2 ~]# 
~~~

Check the crio implementation for further details, e.g.:
[https://github.com/cri-o/cri-o/blob/d23a830d170ae177c922fe63611f8d1d3772ef8a/server/sandbox_run_linux.go#L494](https://github.com/cri-o/cri-o/blob/d23a830d170ae177c922fe63611f8d1d3772ef8a/server/sandbox_run_linux.go#L494)
~~~
	// If using kata runtime, the process label should be set to container_kvm_t
	// Keep in mind that kata does *not* apply any process label to containers within the VM
	// Note: the requirement here is that the name used for the runtime class has "kata" in it
	// or the runtime_type is set to "vm"
	if runtimeType == libconfig.RuntimeTypeVM || strings.Contains(strings.ToLower(runtimeHandler), "kata") {
		processLabel, err = selinux.SELinuxKVMLabel(processLabel)
		if err != nil {
			return nil, err
		}
		g.SetProcessSelinuxLabel(processLabel)
	}
~~~

For an explanation of the fields, see: [https://github.com/cri-o/cri-o/blob/master/docs/crio.conf.5.md#crioruntime-table](https://github.com/cri-o/cri-o/blob/master/docs/crio.conf.5.md#crioruntime-table)
~~~
CRIO.RUNTIME.RUNTIMES TABLE

The "crio.runtime.runtimes" table defines a list of OCI compatible runtimes. The runtime to use is picked based on the runtime_handler provided by the CRI. If no runtime_handler is provided, the runtime will be picked based on the level of trust of the workload.

runtime_path="" Path to the OCI compatible runtime used for this runtime handler.

runtime_root="" Root directory used to store runtime data

runtime_type="oci" Type of the runtime used for this runtime handler. "oci", "vm"
~~~

The runtime class in a pod is then selected with `runtimeClassName`:
~~~
[root@openshift-jumpserver-0 ~]# cat katapod.yaml 
apiVersion: v1
kind: Pod
metadata:
  name: katapod
spec:
  containers:
  - name: sample-container
    image: fedora:latest
    imagePullPolicy: IfNotPresent
    command: ["sleep", "infinity"]
  runtimeClassName: kata-oc
~~~

### crio - debugging kata container launch ###

Change log level to debug:
~~~
[root@openshift-worker-2 ~]# grep log_level /etc/crio/ -R
/etc/crio/crio.conf:log_level = "info"
/etc/crio/crio.conf.d/00-default:log_level = "debug"   <----------------------- here
[root@openshift-worker-2 ~]# systemctl restart crio
~~~

Then, spawn a new pod, named katapod. The logs will look like this:
~~~
[root@openshift-worker-2 ~]# journalctl -u crio --since "7 minutes ago" | grep kata | cut -b 1-250
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: time="2020-07-28T10:09:11.335500711Z" level=warning msg="Could not remove container share dir" ID=dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 error="no such file or direc
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: time="2020-07-28 10:09:11.360578238Z" level=info msg="Stopped container 6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a: katatest/katapod/sample-container" id=2a25e783-dc3f-4
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: time="2020-07-28 10:09:11.361758542Z" level=info msg="Got pod network &{Name:katapod Namespace:katatest ID:dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 NetNS:/var/run/netn
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: 2020-07-28T10:09:11Z [verbose] Del: katatest:katapod:a7633f9b-5ce7-4a97-9137-206ce44b58ac:ovn-kubernetes:eth0 {"cniVersion":"0.4.0","dns":{},"ipam":{},"logFile":"/var/log/ovn-kubernetes/ovn-k
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: time="2020-07-28T10:09:11.477479323Z" level=warning msg="Could not remove container share dir" ID=dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 error="no such file or direc
Jul 28 10:09:11 openshift-worker-2.example.com crio[2156]: time="2020-07-28 10:09:11.808648343Z" level=info msg="Removed container 6b41cdf410d12e75f99c37c7cadc4c968b1061d388db379aecacf381f46af56a: katatest/katapod/sample-container" id=573efd66-576b-4
Jul 28 10:10:03 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:03.191724317Z" level=debug msg="Found valid runtime \"kata-oc\" for runtime_path \"/usr/bin/containerd-shim-kata-v2\"" file="config/config.go:940"
Jul 28 10:10:03 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:03.225896842Z" level=warning msg="Unable to delete container k8s_katapod_katatest_a7633f9b-5ce7-4a97-9137-206ce44b58ac_0: identifier is not a container" file="server
Jul 28 10:10:04 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:04.343526624Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:14 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:14.380904505Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:16 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:16.654826038Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:20 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:20.903822858Z" level=debug msg="Request: &RunPodSandboxRequest{Config:&PodSandboxConfig{Metadata:&PodSandboxMetadata{Name:katapod,Uid:fdc30295-d339-448c-9019-5a7cfc9
Jul 28 10:10:20 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:20.903981027Z" level=info msg="Running pod sandbox: katatest/katapod/POD" file="server/sandbox_run_linux.go:55" id=06c123b2-8741-42d2-9e49-4a89bd681d1a name=/runtime
Jul 28 10:10:20 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:20.917167736Z" level=info msg="Got pod network &{Name:katapod Namespace:katatest ID:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 NetNS:/var/run/n
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: 2020-07-28T10:10:22Z [verbose] Add: katatest:katapod:fdc30295-d339-448c-9019-5a7cfc9585c2:(ovn-kubernetes):eth0 {"cniVersion":"0.4.0","interfaces":[{"name":"9d8a2e790b7836e","mac":"ea:0a:b
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: I0728 10:10:22.448390 3068254 event.go:221] Event(v1.ObjectReference{Kind:"Pod", Namespace:"katatest", Name:"katapod", UID:"fdc30295-d339-448c-9019-5a7cfc9585c2", APIVersion:"v1", Resource
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:22.462209769Z" level=info msg="Got pod network &{Name:katapod Namespace:katatest ID:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 NetNS:/var/run/n
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.530538309Z" level=info msg="loaded configuration" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 file=/usr/share/kata-containers/defaults/con
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.530688992Z" level=info msg="vsock supported" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=katautils
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.530538309Z" level=info msg="loaded configuration" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 file=/usr/share/kata-containers/defaults/con
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.530688992Z" level=info msg="vsock supported" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=katautils
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.530725036Z" level=info msg="VSOCK supported, configure to not use proxy" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=katautils
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.530725036Z" level=info msg="VSOCK supported, configure to not use proxy" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=katautils
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.530785191Z" level=info msg="shm-size detected: 67108864" source=virtcontainers subsystem=oci
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.533182605Z" level=info msg="adding volume" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=virtcontainers subsystem=qemu volume-type=vi
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.534052168Z" level=info msg="Endpoints found after scan" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 endpoints="[0xc000592780]" source=virt
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.534183103Z" level=info msg="Attaching endpoint" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 endpoint-type=virtual hotplug=false source=vir
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.576732075Z" level=info msg="Starting VM" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 sandbox=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.577702797Z" level=info msg="Adding extra file [0xc0000109f8 0xc000010ad0 0xc000010ab8]" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.577783973Z" level=info msg="launching /usr/libexec/qemu-kvm with: [-name sandbox-9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 -uuid 59a1fe1e-8
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.577783973Z" level=info msg="launching /usr/libexec/qemu-kvm with: [-name sandbox-9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 -uuid 59a1fe1e-8
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.662797076Z" level=info msg="{\"QMP\": {\"version\": {\"qemu\": {\"micro\": 0, \"minor\": 2, \"major\": 4}, \"package\": \"qemu-kvm-4.2.0-19.el8\"}, \"capabilities
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.663082377Z" level=info msg="QMP details" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 qmp-capabilities=oob qmp-major-version=4 qmp-micro-ve
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.663188824Z" level=info msg="{\"execute\":\"qmp_capabilities\"}" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=virtcontainers subsyste
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.669410267Z" level=info msg="{\"return\": {}}" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 source=virtcontainers subsystem=qmp
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.669553509Z" level=info msg="sanner return error: read unix @->/run/vc/vm/9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10/qmp.sock: use of closed 
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.669631223Z" level=info msg="VM started" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 sandbox=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.669728645Z" level=info msg="proxy started" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 proxy-pid=3068393 proxy-url="vsock://3022826186:102
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.669728645Z" level=info msg="proxy started" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 proxy-pid=3068393 proxy-url="vsock://3022826186:102
Jul 28 10:10:22 openshift-worker-2.example.com kata[3068374]: time="2020-07-28T10:10:22.669812928Z" level=info msg="New client" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 proxy=3068393 source=virtcontainers subsystem=kata_age
Jul 28 10:10:22 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:22.669812928Z" level=info msg="New client" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 proxy=3068393 source=virtcontainers subsystem=kata_age
Jul 28 10:10:24 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:24.400551122Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:25.048437517Z" level=info msg="Using sandbox shm" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 shm-size=67108864 source=virtcontainers subsyst
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:25.048700740Z" level=info msg="SELinux label from config will be applied to the hypervisor process, not the VM workload" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb0125
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.343910261Z" level=info msg="Ran pod sandbox 9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 with infra container: katatest/katapod/POD" file="ser
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.344647979Z" level=debug msg="Response: &PodSandboxStatusResponse{Status:&PodSandboxStatus{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10,Meta
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.349933952Z" level=debug msg="Request: &CreateContainerRequest{PodSandboxId:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10,Config:&ContainerConfi
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.350113427Z" level=info msg="Creating container: katatest/katapod/sample-container" file="server/container_create.go:524" id=ea52ea9c-e9ca-4d9e-8002-da3d3166fe7c n
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.362037581Z" level=debug msg="Setting container's log_path = /var/log/pods/katatest_katapod_fdc30295-d339-448c-9019-5a7cfc9585c2, sbox.logdir = sample-container/0.
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:25.402397904Z" level=info msg="Using sandbox shm" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10 shm-size=67108864 source=virtcontainers subsyst
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28T10:10:25.402627956Z" level=info msg="SELinux label from config will be applied to the hypervisor process, not the VM workload" ID=9d8a2e790b7836e0e2cebc446996acd9ef0cb0125
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.501604493Z" level=info msg="Created container 8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6cc4: katatest/katapod/sample-container" file="server/co
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.507785167Z" level=info msg="Started container 8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6cc4: katatest/katapod/sample-container" file="server/co
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.883031245Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.884143520Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.885476950Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.885897712Z" level=debug msg="Response: &PodSandboxStatusResponse{Status:&PodSandboxStatus{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10,Meta
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.886321570Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:25 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:25.886665870Z" level=debug msg="Response: &ContainerStatusResponse{Status:&ContainerStatus{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6cc4,Metada
Jul 28 10:10:26 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:26.887642507Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:26 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:26.889329001Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.198982924Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.200308230Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.201527681Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.202571813Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.891804708Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:27 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:27.892945930Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:28 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:28.894628148Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:28 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:28.895748056Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:29 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:29.197921214Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:29 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:29.198882077Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:29 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:29.897301640Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:29 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:29.898454054Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:30 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:30.901632289Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:30 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:30.902863561Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.198043649Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.199211238Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.200295569Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.201321049Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.904501224Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:31 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:31.905741478Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:32 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:32.907397641Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:32 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:32.908636589Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:33 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:33.197869270Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:33 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:33.198924362Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:33 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:33.910225026Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:33 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:33.911466751Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:34 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:34.437957994Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:34 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:34.913063858Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:34 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:34.914176789Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.197818930Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.198847621Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.199968036Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.200945896Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.491404744Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.492513493Z" level=debug msg="Response: &ContainerStatusResponse{Status:&ContainerStatus{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6cc4,Metada
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.915801132Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:35 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:35.918835240Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:36 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:36.920490279Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:36 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:36.921919929Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.197865644Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.198828183Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.199978674Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.201046190Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.923587550Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:37 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:37.924768896Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:38 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:38.926437831Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:38 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:38.927535330Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:39 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:39.197561502Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:39 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:39.198589011Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:39 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:39.929242132Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:39 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:39.930624954Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:40 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:40.932460677Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:40 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:40.933685014Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.198117596Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.199189319Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.200312331Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.201397260Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.935267335Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:41 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:41.936533454Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:42 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:42.938964939Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:42 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:42.940206741Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:43 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:43.198051418Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:43 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:43.199127960Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:43 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:43.941875134Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:43 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:43.943014454Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:44 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:44.477098929Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:44 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:44.944623020Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:44 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:44.945858418Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.198112719Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.199403987Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.201342432Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.202303876Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.497788583Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.498908989Z" level=debug msg="Response: &ContainerStatusResponse{Status:&ContainerStatus{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6cc4,Metada
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.543711612Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.545123859Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.546270304Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.947309093Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:45 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:45.948492558Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:46 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:46.659075373Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:46 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:46.950714682Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:46 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:46.951824792Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:47 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:47.198041957Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:47 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:47.198993669Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:47 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:47.953643279Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:47 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:47.954838431Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:48 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:48.956694271Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:48 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:48.957802676Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.200294004Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.201241261Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.202321480Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.203692827Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.959650132Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:49 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:49.960738848Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:50 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:50.962629963Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:50 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:50.964061786Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:51 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:51.198210094Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:51 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:51.199165518Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:51 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:51.965826824Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:51 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:51.967090960Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:52 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:52.968922540Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:52 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:52.970185474Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.197881649Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.199482168Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.201311069Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.202378219Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.971897478Z" level=debug msg="Response: &ListPodSandboxResponse{Items:[]*PodSandbox{&PodSandbox{Id:9d8a2e790b7836e0e2cebc446996acd9ef0cb01256b92d348e6fff710ce15f10
Jul 28 10:10:53 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:53.973242764Z" level=debug msg="Response: &ListContainersResponse{Containers:[]*Container{&Container{Id:8c884690004b734b63c1c7a04139f3570cb57846e5c948f4fe28756e017b6
Jul 28 10:10:54 openshift-worker-2.example.com crio[3067623]: time="2020-07-28 10:10:54.491767193Z" level=debug msg="Response: &ListImagesResponse{Images:[]*Image{&Image{Id:aa9557bde2f3e1699a119eea4fe53bfef7232628a8c03597816b58a77cd47297,RepoTags:[qu
Jul 28 10:10:55 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:55.236031546Z" level=warning msg="Unable to delete container k8s_POD_katapod_katatest_fdc30295-d339-448c-9019-5a7cfc9585c2_0: 1 error occurred:\n\t* unlinkat /var/ru
Jul 28 10:10:55 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:55.236122983Z" level=warning msg="Unable to delete container k8s_katapod_katatest_fdc30295-d339-448c-9019-5a7cfc9585c2_0: identifier is not a container"
Jul 28 10:10:55 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:55.350843107Z" level=warning msg="Unable to delete container k8s_sample-container_katapod_katatest_fdc30295-d339-448c-9019-5a7cfc9585c2_0: identifier is not a contai
Jul 28 10:10:55 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:55.977080875Z" level=info msg="Running pod sandbox: katatest/katapod/POD" id=00ec3ec8-8a02-4626-a4be-3eee173a95ed name=/runtime.v1alpha2.RuntimeService/RunPodSandbox
Jul 28 10:10:55 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:55.989280275Z" level=info msg="Got pod network &{Name:katapod Namespace:katatest ID:a9765d6aa6a1ceb776f4726aab1ce901cbd4d10c2c41c008fe2d4579e36b4c3b NetNS:/var/run/n
Jul 28 10:10:58 openshift-worker-2.example.com crio[3069288]: 2020-07-28T10:10:58Z [verbose] Add: katatest:katapod:fdc30295-d339-448c-9019-5a7cfc9585c2:(ovn-kubernetes):eth0 {"cniVersion":"0.4.0","interfaces":[{"name":"a9765d6aa6a1ceb","mac":"72:e1:9
Jul 28 10:10:58 openshift-worker-2.example.com crio[3069288]: I0728 10:10:58.038289 3069588 event.go:221] Event(v1.ObjectReference{Kind:"Pod", Namespace:"katatest", Name:"katapod", UID:"fdc30295-d339-448c-9019-5a7cfc9585c2", APIVersion:"v1", Resource
Jul 28 10:10:58 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:10:58.051337687Z" level=info msg="Got pod network &{Name:katapod Namespace:katatest ID:a9765d6aa6a1ceb776f4726aab1ce901cbd4d10c2c41c008fe2d4579e36b4c3b NetNS:/var/run/n
Jul 28 10:11:00 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:11:00.933784352Z" level=info msg="Ran pod sandbox a9765d6aa6a1ceb776f4726aab1ce901cbd4d10c2c41c008fe2d4579e36b4c3b with infra container: katatest/katapod/POD" id=00ec3e
Jul 28 10:11:00 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:11:00.938623997Z" level=info msg="Creating container: katatest/katapod/sample-container" id=25867b9e-a543-42e7-ba98-fb38ddd938db name=/runtime.v1alpha2.RuntimeService/C
Jul 28 10:11:01 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:11:01.091184087Z" level=info msg="Created container ed470742207b8df91b5b194d07c0f0db2746470accdeeba28d90d37cfc3a8447: katatest/katapod/sample-container" id=25867b9e-a54
Jul 28 10:11:01 openshift-worker-2.example.com crio[3069288]: time="2020-07-28 10:11:01.096407977Z" level=info msg="Started container ed470742207b8df91b5b194d07c0f0db2746470accdeeba28d90d37cfc3a8447: katatest/katapod/sample-container" id=5ec1b483-d18
~~~

Disable debugging after the test as crio logs quite a lot in debug mode.

### Finding the kata-containers socket ###

WIP

~~~
[root@openshift-worker-2 /]# ps aux | grep crio | grep -v conmon
root        2156  0.5  0.1 4854356 145820 ?      Ssl  Jul26  13:56 /usr/bin/crio --enable-metrics=true --metrics-port=9537
root        2210  5.5  0.1 5937816 168356 ?      Ssl  Jul26 139:27 kubelet --config=/etc/kubernetes/kubelet.conf --bootstrap-kubeconfig=/etc/kubernetes/kubeconfig --kubeconfig=/var/lib/kubelet/kubeconfig --container-runtime=remote --container-runtime-endpoint=/var/run/crio/crio.sock --runtime-cgroups=/system.slice/crio.service --node-labels=node-role.kubernetes.io/worker,node.openshift.io/os_id=rhcos --minimum-container-ttl-duration=6m0s --volume-plugin-dir=/etc/kubernetes/kubelet-plugins/volume/exec --cloud-provider= --pod-infra-container-image=quay.io/openshift-release-dev/ocp-v4.0-art-dev@sha256:636ba95d1f2717a876ab2cb1b09ac9b40739db00d26b5cdb9c3a7d6114b75494 --v=4
root       28433  0.0  0.0 878648 31028 ?        Sl   Jul26   2:14 /usr/bin/containerd-shim-kata-v2 -namespace default -address  -publish-binary /usr/bin/crio -id dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103
root     3047886  0.0  0.0  16368   972 pts/0    S+   09:53   0:00 grep --color=auto crio
[root@openshift-worker-2 /]# lsof -nn -p 2156 | grep sock
crio    2156 root   10u     unix 0xffff92a9d4622400      0t0      52558 /var/run/crio/crio.sock type=STREAM
crio    2156 root   22u     unix 0xffff92a9d3f10480      0t0      35195 /var/run/crio/crio.sock type=STREAM
crio    2156 root   23u     unix 0xffff92a9d3f5e300      0t0      35197 /var/run/crio/crio.sock type=STREAM
crio    2156 root   36u     sock                0,9      0t0   11508128 protocol: TCPv6
crio    2156 root   40u     sock                0,9      0t0   13352222 protocol: TCPv6
crio    2156 root   46u     unix 0xffff9299d3cb4c80      0t0   19430168 /var/run/crio/crio.sock type=STREAM
crio    2156 root   64u     unix 0xffff92a9c2873180      0t0   19407526 /var/run/crio/crio.sock type=STREAM
[root@openshift-worker-2 /]# ps aux | grep kata
root       28433  0.0  0.0 878648 31028 ?        Sl   Jul26   2:14 /usr/bin/containerd-shim-kata-v2 -namespace default -address  -publish-binary /usr/bin/crio -id dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103
root       28445  0.0  0.0  68424  5664 ?        S    Jul26   0:00 /usr/libexec/virtiofsd --fd=3 -o source=/run/kata-containers/shared/sandboxes/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shared -o cache=always --syslog -o no_posix_lock -f
root       28450  0.1  0.7 2599904 975108 ?      Sl   Jul26   4:35 /usr/libexec/qemu-kvm -name sandbox-dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103 -uuid 6f03e56e-efa1-4f0f-9b8f-492cf1824c83 -machine q35,accel=kvm,kernel_irqchip -cpu host -qmp unix:/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/qmp.sock,server,nowait -m 2048M,slots=10,maxmem=129853M -device pci-bridge,bus=pcie.0,id=pci-bridge-0,chassis_nr=1,shpc=on,addr=2,romfile= -device virtio-serial-pci,disable-modern=false,id=serial0,romfile= -device virtconsole,chardev=charconsole0,id=console0 -chardev socket,id=charconsole0,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/console.sock,server,nowait -device virtio-scsi-pci,id=scsi0,disable-modern=false,romfile= -object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0,romfile= -device vhost-vsock-pci,disable-modern=false,vhostfd=3,id=vsock-555233078,guest-cid=555233078,romfile= -chardev socket,id=char-1082a328d9d79959,path=/run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/vhost-fs.sock -device vhost-user-fs-pci,chardev=char-1082a328d9d79959,tag=kataShared,romfile= -netdev tap,id=network-0,vhost=on,vhostfds=4,fds=5 -device driver=virtio-net-pci,netdev=network-0,mac=d6:96:23:1b:02:05,disable-modern=false,mq=on,vectors=4,romfile= -global kvm-pit.lost_tick_policy=discard -vga none -no-user-config -nodefaults -nographic -daemonize -object memory-backend-file,id=dimm1,size=2048M,mem-path=/dev/shm,share=on -numa node,memdev=dimm1 -kernel /usr/lib/modules/4.18.0-193.13.2.el8_2.x86_64/vmlinuz -initrd /var/cache/kata-containers/osbuilder-images/4.18.0-193.13.2.el8_2.x86_64/"rhcos"-kata-4.18.0-193.13.2.el8_2.x86_64.initrd -append tsc=reliable no_timer_check rcupdate.rcu_expedited=1 i8042.direct=1 i8042.dumbkbd=1 i8042.nopnp=1 i8042.noaux=1 noreplace-smp reboot=k console=hvc0 console=hvc1 iommu=off cryptomgr.notests net.ifnames=0 pci=lastbus=0 quiet panic=1 nr_cpus=40 agent.use_vsock=true scsi_mod.scan=none -pidfile /run/vc/vm/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/pid -smp 1,cores=1,threads=1,sockets=40,maxcpus=40
root       28454  0.0  0.2 4216312 349024 ?      Sl   Jul26   0:23 /usr/libexec/virtiofsd --fd=3 -o source=/run/kata-containers/shared/sandboxes/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shared -o cache=always --syslog -o no_posix_lock -f
root     3048279  0.0  0.0  16368   972 pts/0    S+   09:54   0:00 grep --color=auto kata
[root@openshift-worker-2 /]# lsof -nn -p 28433 | grep sock
container 28433 root    3u     sock                0,9      0t0   3589421 protocol: AF_VSOCK
container 28433 root    7u     unix 0xffff92a9cbf2de80      0t0   3038410 @/containerd-shim/default/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shim.sock@ type=STREAM
container 28433 root    8u     unix 0xffff92a9cbf08480      0t0   3131447 @/containerd-shim/default/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shim.sock@ type=STREAM
container 28433 root   17u     unix 0xffff9299e58c5a00      0t0   3535834 @/containerd-shim/default/dc7a95dc3219fa58f6b87ac10cb2f5b0a562d8434c2cdc8d4d940d8e763e5103/shim.sock@ type=STREAM
[root@openshift-worker-2 /]# 
~~~

### Connecting a debug console to the kata VM ###

* [https://github.com/kata-containers/documentation/blob/master/Developer-Guide.md#set-up-a-debug-console](https://github.com/kata-containers/documentation/blob/master/Developer-Guide.md#set-up-a-debug-console)


## Resources ##

### Kata operator ###

* [https://github.com/harche/kata-operator](https://github.com/harche/kata-operator)
* [https://www.youtube.com/watch?v=k7rLIU3L94w](https://www.youtube.com/watch?v=k7rLIU3L94w)

### Kata containers ###

* [https://katacontainers.io/](https://katacontainers.io/)
* [https://github.com/kata-containers/kata-containers/tree/2.0-dev/tools/packaging/kata-deploy](https://github.com/kata-containers/kata-containers/tree/2.0-dev/tools/packaging/kata-deploy)
* [https://github.com/kata-containers/kata-containers/blob/2.0-dev/tools/packaging/kata-deploy/scripts/kata-deploy.sh](https://github.com/kata-containers/kata-containers/blob/2.0-dev/tools/packaging/kata-deploy/scripts/kata-deploy.sh)
* [https://github.com/kata-containers/osbuilder](https://github.com/kata-containers/osbuilder)
* [https://github.com/kata-containers/runtime](https://github.com/kata-containers/runtime)
* [https://github.com/kata-containers/documentation/blob/master/design/architecture.md](https://github.com/kata-containers/documentation/blob/master/design/architecture.md)
* [https://github.com/kata-containers/documentation/blob/master/how-to/run-kata-with-k8s.md](https://github.com/kata-containers/documentation/blob/master/how-to/run-kata-with-k8s.md)
* [https://github.com/kata-containers/documentation/blob/master/design/virtualization.md](https://github.com/kata-containers/documentation/blob/master/design/virtualization.md)
* [https://katacontainers.io/learn/](https://katacontainers.io/learn/)

### kubernetes ###

* [https://kubernetes.io/docs/concepts/containers/runtime-class/](https://kubernetes.io/docs/concepts/containers/runtime-class/)

### Firecracker ###

* [https://medium.com/@gokulchandrapr/kata-containers-on-kubernetes-and-kata-firecracker-vmm-support-28abb3a196e7](https://medium.com/@gokulchandrapr/kata-containers-on-kubernetes-and-kata-firecracker-vmm-support-28abb3a196e7)
* [https://github.com/kata-containers/documentation/wiki/Initial-release-of-Kata-Containers-with-Firecracker-support](https://github.com/kata-containers/documentation/wiki/Initial-release-of-Kata-Containers-with-Firecracker-support)
