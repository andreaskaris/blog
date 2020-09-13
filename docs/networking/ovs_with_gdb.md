# Inspecting OVS with GDB #

## Introduction ##

Most of this article is based on [http://www.openvswitch.org/support/ovscon2018/6/1345-chaudron.pdf](http://www.openvswitch.org/support/ovscon2018/6/1345-chaudron.pdf)

This is mainly a walkthrough of how to get there on RHEL 7.

## Generate a core dump of OVS ##

Enable core dump collection with abrt:
~~~
yum install abrt abrt-addon-ccpp abrt-tui
abrt-install-ccpp-hook install
abrt-install-ccpp-hook is-installed; echo $?;
service abrtd start
service abrt-ccpp start
abrt-auto-reporting enabled
abrt-cli list
~~~

Kill OVS:
~~~
kill -11 $(pidof ovs-vswitchd)
~~~

List core dumps:
~~~
[root@overcloud-computedpdk-0 ~]# abrt-cli list
id 60b0650c5db6225ab1debfba0f80bbdee60cef43
reason:         ovs-vswitchd killed by SIGSEGV
time:           Mon 31 Dec 2018 05:09:12 PM UTC
cmdline:        ovs-vswitchd unix:/var/run/openvswitch/db.sock -vconsole:emer -vsyslog:err -vfile:info --mlockall --user openvswitch:hugetlbfs --no-chdir --log-file=/var/log/openvswitch/ovs-vswitchd.log --pidfile=/var/run/openvswitch/ovs-vswitchd.pid --detach
package:        openvswitch-2.9.0-56.el7fdp
uid:            993 (openvswitch)
count:          1
Directory:      /var/spool/abrt/ccpp-2018-12-31-17:09:12-671118
~~~

~~~
[root@overcloud-computedpdk-0 ~]# file !$
file /var/spool/abrt/ccpp-2018-12-31-17:09:12-671118/coredump
/var/spool/abrt/ccpp-2018-12-31-17:09:12-671118/coredump: ELF 64-bit LSB core file x86-64, version 1 (SYSV), SVR4-style, from 'ovs-vswitchd unix:/var/run/openvswitch/db.sock -vconsole:emer -vsyslog:err -vfi', real uid: 0, effective uid: 0, real gid: 0, effective gid: 0, execfn: '/sbin/ovs-vswitchd', platform: 'x86_64'
~~~

## Install debuginfo for OVS ##

It's important that the right debuginfo be installed. 
Otherwise, you will get a message like this from the script later down the road:
~~~
(gdb) ovs_dump_bridge ports
Can't find all_bridges global variable, are you sure your debugging OVS?
~~~

~~~
debuginfo-install $(rpm -qa | egrep '^openvswitch-[0-9]')
~~~

## Download the script ##

It should be better to get the script from a source RPM. However, my version of OVS doesn't have the script yet. 
Hence I'm directly downloading it:
~~~
curl -o ovs_gdb.py \
  https://raw.githubusercontent.com/openvswitch/ovs/cd5b89a5a99c3ead973b168326eaef47d4e4c077/utilities/gdb/ovs_gdb.py
~~~

## Open core dump with GDB ##

~~~
gdb $(which ovs-vswitchd) /var/spool/abrt/ccpp-2018-12-31-17:09:12-671118/coredump
~~~

Once at the prompt:
~~~
source ovs_gdb.py
~~~

## Using the debug script ##

~~~
(gdb) ovs_dump_bridge
(struct bridge *) 0x5563a14fd2b0: name = br-link0, type = netdev
(struct bridge *) 0x5563a14fd8c0: name = br-int, type = netdev
(gdb) ovs_dump_bridge ports
(struct bridge *) 0x5563a14fd2b0: name = br-link0, type = netdev
    (struct port *) 0x5563a1552800: name = br-link0, brige = (struct bridge *) 0x5563a14fd2b0
        (struct iface *) 0x5563a157b090: name = 0x5563a1502070 "br-link0", ofp_port = 65534, netdev = (struct netdev *) 0x5563a157a7d0
    (struct port *) 0x5563a157d480: name = dpdk0, brige = (struct bridge *) 0x5563a14fd2b0
        (struct iface *) 0x5563a157d720: name = 0x5563a157bc40 "dpdk0", ofp_port = 1, netdev = (struct netdev *) 0x7f223fc6b6c0
    (struct port *) 0x5563a15809b0: name = phy-br-link0, brige = (struct bridge *) 0x5563a14fd2b0
        (struct iface *) 0x5563a15806a0: name = 0x5563a1580720 "phy-br-link0", ofp_port = 2, netdev = (struct netdev *) 0x5563a1582d20
(struct bridge *) 0x5563a14fd8c0: name = br-int, type = netdev
    (struct port *) 0x5563a1583220: name = int-br-link0, brige = (struct bridge *) 0x5563a14fd8c0
        (struct iface *) 0x5563a15839c0: name = 0x5563a1580790 "int-br-link0", ofp_port = 1, netdev = (struct netdev *) 0x5563a1583620
    (struct port *) 0x5563a1584560: name = br-int, brige = (struct bridge *) 0x5563a14fd8c0
        (struct iface *) 0x5563a1584e70: name = 0x5563a1584ef0 "br-int", ofp_port = 65534, netdev = (struct netdev *) 0x5563a1584150
~~~
