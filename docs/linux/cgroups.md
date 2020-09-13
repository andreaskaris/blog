# Quick guide for cgroups #

## What are cgroups? ##

### Summary - what is a cgroup ###

cgroups ...

- stand for control groups
- handle management, accounting of system resources like CPU, memory, I/O
- associates a set of tasks with a set of parameters for one or more subsystems 
- on their own allow for simple job tracking 
- combined with other subsystems (so-called resource controllers) allow for resource accounting / monitoring / limiting of resources
- provided through the cgroupfs pseudo filesystem
- are organized in hierarchies - hierarchies are defined by creating subdirectories in the cgroup filessystem
- each process is in exactly one node in each hierarchy (cpu hierarchy, memory hierarchy, ...)
- limit *how much* you can use of a system resource (quantity)

### Definitions - what is a cgroup ###

The following is the definition of cgroups v1 from the kernel documentation: [https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt](https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt):
~~~
(...)
A *cgroup* associates a set of tasks with a set of parameters for one
or more subsystems.

A *subsystem* is a module that makes use of the task grouping
facilities provided by cgroups to treat groups of tasks in
particular ways. A subsystem is typically a "resource controller" that
schedules a resource or applies per-cgroup limits, but it may be
anything that wants to act on a group of processes, e.g. a
virtualization subsystem.

A *hierarchy* is a set of cgroups arranged in a tree, such that
every task in the system is in exactly one of the cgroups in the
hierarchy, and a set of subsystems; each subsystem has system-specific
state attached to each cgroup in the hierarchy.  Each hierarchy has
an instance of the cgroup virtual filesystem associated with it.
(...)
On their own, the only use for cgroups is for simple job
tracking. The intention is that other subsystems hook into the generic
cgroup support to provide new attributes for cgroups, such as
accounting/limiting the resources which processes in a cgroup can
access. For example, cpusets (see Documentation/cgroup-v1/cpusets.txt) allow
you to associate a set of CPUs and a set of memory nodes with the
tasks in each cgroup.
(...)
~~~


`man 7 cgroups`:
~~~
(...)
Control  cgroups,  usually  referred to as cgroups, are a Linux kernel feature which
allow processes to be organized into hierarchical  groups  whose  usage  of  various
types of resources can then be limited and monitored.  The kernel's cgroup interface
is provided through a pseudo-filesystem called cgroupfs.  Grouping is implemented in
the core cgroup kernel code, while resource tracking and limits are implemented in a
set of per-resource-type subsystems (memory, CPU, and so on).
(...)
Subsystems are sometimes also known as resource controllers (or 
simply, controllers).
(...)
The cgroups for a controller are arranged in a hierarchy.  This hierarchy is defined
by creating, removing, and renaming subdirectories within the cgroup filesystem.  At  
each level of the hierarchy, attributes (e.g., limits) can be defined.  The  limits,
control,  and  accounting  provided  by cgroups generally have effect throughout the 
subhierarchy underneath the cgroup where the  attributes  are  defined.   Thus,  for 
example,  the limits placed on a cgroup at a higher level in the hierarchy cannot be
exceeded by descendant cgroups.
(...)
~~~

[https://lwn.net/Articles/679786/](https://lwn.net/Articles/679786/):
~~~
The cgroup subsystem and associated controllers handle management and accounting of various system resources like CPU, memory, I/O, and more. Together with the Linux namespace subsystem, which is a bit older (having started around 2002) and is considered a bit more mature (apart, perhaps, from user namespaces, which still raise discussions), these subsystems form the basis of Linux containers. Currently, most projects involving Linux containers, like Docker, LXC, OpenVZ, Kubernetes, and others, are based on both of them. The development of the Linux cgroup subsystem started in 2006 at Google, led primarily by Rohit Seth and Paul Menage. Initially the project was called "Process Containers", but later on the name was changed to "Control Groups", to avoid confusion with Linux containers, and nowadays everybody calls them "cgroups" for short. There are currently 12 cgroup controllers in cgroups v1; all—except one—have existed for several years. The new addition is the PIDs controller, developed by Aditya Kali and merged in kernel 4.3. It allows restricting the number of processes created inside a control group, and it can be used as an anti-fork-bomb solution. The PID space in Linux consists of, at a maximum, about four million PIDs (PID_MAX_LIMIT). Given today's RAM capacities, this limit could easily and quite quickly be exhausted by a fork bomb from within a single container. The PIDs controller is supported by both cgroups v1 and cgroups v2.
Over the years, there was a lot of criticism about the implementation of cgroups, which seems to present a number of inconsistencies and a lot of chaos. For example, when creating subgroups (cgroups within cgroups), several cgroup controllers propagate parameters to their immediate subgroups, while other controllers do not. Or, for a different example, some controllers use interface files (such as the cpuset controller's clone_children) that appear in all controllers even though they only affect one.
As maintainer Tejun Heo himself has admitted [YouTube], "design followed implementation", "different decisions were taken for different controllers", and "sometimes too much flexibility causes a hindrance". In an LWN article from 2012, it was said that "control groups are one of those features that kernel developers love to hate." 
~~~

## The relationship of containers and cgroups ##

Containers are basically just a bunch of cgroups plus namespace isolation (plus some extra features):

[https://en.wikipedia.org/wiki/LXC](https://en.wikipedia.org/wiki/LXC):
~~~
The Linux kernel provides the cgroups functionality that allows limitation and prioritization of resources (CPU, memory, block I/O, network, etc.) without the need for starting any virtual machines, and also namespace isolation functionality that allows complete isolation of an applications' view of the operating environment, including process trees, networking, user IDs and mounted file systems.[3]

LXC combines the kernel's cgroups and support for isolated namespaces to provide an isolated environment for applications. Early versions of Docker used LXC as the container execution driver, though LXC was made optional in v0.9 and support was dropped in Docker v1.10. [4] 
~~~

[https://en.wikipedia.org/wiki/Docker_(software)](https://en.wikipedia.org/wiki/Docker_(software)):
~~~
Docker is developed primarily for Linux, where it uses the resource isolation features of the Linux kernel such as cgroups and kernel namespaces, and a union-capable file system such as OverlayFS and others[28] to allow independent "containers" to run within a single Linux instance, avoiding the overhead of starting and maintaining virtual machines (VMs).[29] The Linux kernel's support for namespaces mostly[30] isolates an application's view of the operating environment, including process trees, network, user IDs and mounted file systems, while the kernel's cgroups provide resource limiting for memory and CPU.[31] Since version 0.9, Docker includes the libcontainer library as its own way to directly use virtualization facilities provided by the Linux kernel, in addition to using abstracted virtualization interfaces via libvirt, LXC and systemd-nspawn.[13][32][27]
~~~

## cgroups versions ##

cgroup comes in 2 versions. cgroups v2 are to replace cgroups v1 eventually. However, for reasons of backwards compatibility, both will probably be around for a very long time.

cgroups v1 have several issues ...

- uncoordinated development of resource controllers
- inconsistencies between controllers
- complex hierarchy management

Solution: cgroups v2.

`man 7 cgroups`:
~~~
(...)
The initial release of the cgroups implementation was in Linux 2.6.24.   Over  time,
various  cgroup controllers have been added to allow the management of various types
of resources.  However, the development of these controllers was  largely  uncoordi‐
nated,  with the result that many inconsistencies arose between controllers and man‐
agement of the cgroup hierarchies became rather complex.  (A longer  description  of
these problems can be found in the kernel source file Documentation/cgroup-v2.txt.)
(...)
~~~

### Backwards compatibility ###

- cgroups v1 is unlikely to be removed
- cgroups v1 and v2 can coexist
- cgroups v2 only implemented a subset of v1's functionality
- users can use resource controllers supported in v2 and use v1 controllers for features which are unsupported in v2

`man 7 cgroups`:
~~~
(...)
Although cgroups v2 is intended as a replacement for cgroups v1,  the  older  system
continues  to exist (and for compatibility reasons is unlikely to be removed).  Cur‐
rently, cgroups v2 implements only a subset of the controllers available in  cgroups
v1.   The two systems are implemented so that both v1 controllers and v2 controllers
can be mounted on the same system.  Thus, for example, it is possible to  use  those
controllers  that  are  supported  under  version 2, while also using version 1 con‐
trollers where version 2 does not yet support those controllers.  The only  restric‐
tion here is that a controller can't be simultaneously employed in both a cgroups v1
hierarchy and in the cgroups v2 hierarchy.
(...)
~~~

### Which version of cgroups are you running? ###

cgroups are mounted as a virtual filesystem. Hence, verify with the mount command which version is currently in use.

#### Default in RHEL 7 ####

RHEL 7 uses cgroups v1:
~~~
[root@rhospbl-1 ~]# mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,xattr,release_agent=/usr/lib/systemd/systemd-cgroups-agent,name=systemd)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,net_prio,net_cls)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,blkio)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,devices)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,hugetlb)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,freezer)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,cpuacct,cpu)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,cpuset)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,perf_event)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,memory)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,pids)
[root@rhospbl-1 ~]# uname -a
Linux rhospbl-1.openstack.gsslab.rdu2.redhat.com 3.10.0-693.el7.x86_64 #1 SMP Thu Jul 6 19:56:57 EDT 2017 x86_64 x86_64 x86_64 GNU/Linux
~~~

#### Default in Fedora 28 ####
~~~
[akaris@wks-akaris blog]$ cat /etc/redhat-release 
Fedora release 28 (Twenty Eight)
[akaris@wks-akaris blog]$ mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup2 on /sys/fs/cgroup/unified type cgroup2 (rw,nosuid,nodev,noexec,relatime,seclabel,nsdelegate)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,xattr,name=systemd)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpu,cpuacct)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,hugetlb)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,perf_event)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,memory)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,pids)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,freezer)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,net_cls,net_prio)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,blkio)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,devices)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpuset)
~~~

#### Default in RHEL 8 ####
~~~
[root@rhel8 ~]# cat /etc/redhat-release 
Red Hat Enterprise Linux release 8.0 Beta (Ootpa)
[root@rhel8 ~]# mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,xattr,release_agent=/usr/lib/systemd/systemd-cgroups-agent,name=systemd)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,freezer)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpu,cpuacct)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,memory)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,net_cls,net_prio)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,pids)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,blkio)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,hugetlb)
cgroup on /sys/fs/cgroup/rdma type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,rdma)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,devices)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpuset)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,perf_event)
~~~

### Support for cgroupv2? ###

#### In RHEL 7 ####

No, cgroups v2 is not in the kernel in RHEL 7:
~~~
[root@rhospbl-1 ~]# mount -t cgroup2 none /mnt/test
mount: unknown filesystem type 'cgroup2'
~~~

#### In RHEL 8 ####

Yes:
~~~~
[root@rhel8 ~]# mount cgroup2 -t cgroup2 /mnt/cgroupv2
[root@rhel8 ~]# mount | grep cgroup2
cgroup2 on /mnt/cgroupv2 type cgroup2 (rw,relatime,seclabel)
[root@rhel8 ~]# 
~~~~

## Types of cgroups v1 resource controllers ##

### cpu resource controller ###

- cpu tracking based on cgroups

### cpuset resource controller ###

[https://www.kernel.org/doc/Documentation/cgroup-v1/hugetlb.txt](https://www.kernel.org/doc/Documentation/cgroup-v1/hugetlb.txt):
~~~
[root@overcloud-controller-0 cgroup]# cd /sys/fs/cgroup/cpuset/
[root@overcloud-controller-0 cpuset]# mkdir test
[root@overcloud-controller-0 test]# echo 2-3 > cpuset.cpus
[root@overcloud-controller-0 test]# cat cpuset.cpus
2-3
[root@overcloud-controller-0 test]# dd if=/dev/zero of=/dev/null &
[2] 931866
[root@overcloud-controller-0 test]# taskset -p -c 931866 
pid 931866's current affinity list: 0-3
[root@overcloud-controller-0 test]# echo 931866 > tasks 
[root@overcloud-controller-0 test]# taskset -p -c 931866 
pid 931866's current affinity list: 2,3
~~~

#### hugetlb resource controller ####

- controls amount of hugepages usable by a process
- by default, a process can request as many hugepages as it wants

Looking at meminfo, we see that 4 hugepages are used:
~~~
[root@overcloud-computesriov-0 system.slice]# cat /sys/devices/system/node/node?/meminfo | grep -i huge
Node 0 AnonHugePages:      4096 kB
Node 0 HugePages_Total:    16
Node 0 HugePages_Free:     12
Node 0 HugePages_Surp:      0
Node 1 AnonHugePages:      2048 kB
Node 1 HugePages_Total:    16
Node 1 HugePages_Free:     16
Node 1 HugePages_Surp:      0
~~~

One way to find out which processes are using hugepages, is to check the hugetlb cgroups:
~~~
[root@overcloud-computesriov-0 ~]# cd /sys/fs/cgroup/hugetlb
[root@overcloud-computesriov-0 hugetlb]# ll
total 0
-rw-r--r--.  1 root root 0 Nov 27 06:17 cgroup.clone_children
--w--w--w-.  1 root root 0 Nov 27 06:17 cgroup.event_control
-rw-r--r--.  1 root root 0 Nov 27 06:17 cgroup.procs
-r--r--r--.  1 root root 0 Nov 27 06:17 cgroup.sane_behavior
-rw-r--r--.  1 root root 0 Nov 27 06:17 hugetlb.1GB.failcnt
-rw-r--r--.  1 root root 0 Nov 27 06:17 hugetlb.1GB.limit_in_bytes
-rw-r--r--.  1 root root 0 Nov 27 06:17 hugetlb.1GB.max_usage_in_bytes
-r--r--r--.  1 root root 0 Nov 27 06:17 hugetlb.1GB.usage_in_bytes
-rw-r--r--.  1 root root 0 Nov 27 06:17 notify_on_release
-rw-r--r--.  1 root root 0 Nov 27 06:17 release_agent
drwxr-xr-x. 11 root root 0 Nov 27 06:46 system.slice
-rw-r--r--.  1 root root 0 Nov 27 06:17 tasks
[root@overcloud-computesriov-0 hugetlb]# cat hugetlb.1GB.usage_in_bytes
4294967296
[root@overcloud-computesriov-0 hugetlb]# cd system.slice
[root@overcloud-computesriov-0 system.slice]# ll
total 0
-rw-r--r--. 1 root root 0 Nov 27 06:25 cgroup.clone_children
--w--w--w-. 1 root root 0 Nov 27 06:25 cgroup.event_control
-rw-r--r--. 1 root root 0 Nov 27 06:25 cgroup.procs
drwxr-xr-x. 2 root root 0 Nov 27 06:46 docker-111c9c039324d640875e550763d6507450cbbd07f6674c3883388839807cd614.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-1286b301010bb53e3d919616054e645c00a2288b9cdc8235bdb68aa404a0c34b.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:46 docker-46ed5e7b2045df285552bde12209717f1601b27e3d6e137ed9122d3d9c519a3d.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-90030441400dd9536aa33d13d3d5792a4e1f025fb383141cb0f18cfaed260979.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:46 docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-b9ff1a61cc4f144cc2aa16332d8e07248ad71a7263056b0d9cddb7339368457a.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-c538d8c4e222b977b218746ee9ebef34335d768a364fbe1bfb3e72284d65520a.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-d2e87b4ec13cc51c9dab3e593ee44051eb243ce684cc300b7e6103e8f35e1320.scope
drwxr-xr-x. 2 root root 0 Nov 27 06:51 docker-dce63a96f513527b894bbec6c7f39f40dd2912bdbf4dec0a51b2e59704c03e7b.scope
-rw-r--r--. 1 root root 0 Nov 27 06:25 hugetlb.1GB.failcnt
-rw-r--r--. 1 root root 0 Nov 27 06:25 hugetlb.1GB.limit_in_bytes
-rw-r--r--. 1 root root 0 Nov 27 06:25 hugetlb.1GB.max_usage_in_bytes
-r--r--r--. 1 root root 0 Nov 27 06:25 hugetlb.1GB.usage_in_bytes
-rw-r--r--. 1 root root 0 Nov 27 06:25 notify_on_release
-rw-r--r--. 1 root root 0 Nov 27 06:25 tasks
[root@overcloud-computesriov-0 system.slice]# cat hugetlb.1GB.usage_in_bytes
4294967296
[root@overcloud-computesriov-0 system.slice]# find . -name '*usage_in_bytes'
./docker-b9ff1a61cc4f144cc2aa16332d8e07248ad71a7263056b0d9cddb7339368457a.scope/hugetlb.1GB.max_usage_in_bytes
./docker-b9ff1a61cc4f144cc2aa16332d8e07248ad71a7263056b0d9cddb7339368457a.scope/hugetlb.1GB.usage_in_bytes
./docker-dce63a96f513527b894bbec6c7f39f40dd2912bdbf4dec0a51b2e59704c03e7b.scope/hugetlb.1GB.max_usage_in_bytes
./docker-dce63a96f513527b894bbec6c7f39f40dd2912bdbf4dec0a51b2e59704c03e7b.scope/hugetlb.1GB.usage_in_bytes
./docker-c538d8c4e222b977b218746ee9ebef34335d768a364fbe1bfb3e72284d65520a.scope/hugetlb.1GB.max_usage_in_bytes
./docker-c538d8c4e222b977b218746ee9ebef34335d768a364fbe1bfb3e72284d65520a.scope/hugetlb.1GB.usage_in_bytes
./docker-d2e87b4ec13cc51c9dab3e593ee44051eb243ce684cc300b7e6103e8f35e1320.scope/hugetlb.1GB.max_usage_in_bytes
./docker-d2e87b4ec13cc51c9dab3e593ee44051eb243ce684cc300b7e6103e8f35e1320.scope/hugetlb.1GB.usage_in_bytes
./docker-90030441400dd9536aa33d13d3d5792a4e1f025fb383141cb0f18cfaed260979.scope/hugetlb.1GB.max_usage_in_bytes
./docker-90030441400dd9536aa33d13d3d5792a4e1f025fb383141cb0f18cfaed260979.scope/hugetlb.1GB.usage_in_bytes
./docker-1286b301010bb53e3d919616054e645c00a2288b9cdc8235bdb68aa404a0c34b.scope/hugetlb.1GB.max_usage_in_bytes
./docker-1286b301010bb53e3d919616054e645c00a2288b9cdc8235bdb68aa404a0c34b.scope/hugetlb.1GB.usage_in_bytes
./docker-111c9c039324d640875e550763d6507450cbbd07f6674c3883388839807cd614.scope/hugetlb.1GB.max_usage_in_bytes
./docker-111c9c039324d640875e550763d6507450cbbd07f6674c3883388839807cd614.scope/hugetlb.1GB.usage_in_bytes
./docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope/hugetlb.1GB.max_usage_in_bytes
./docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope/hugetlb.1GB.usage_in_bytes
./docker-46ed5e7b2045df285552bde12209717f1601b27e3d6e137ed9122d3d9c519a3d.scope/hugetlb.1GB.max_usage_in_bytes
./docker-46ed5e7b2045df285552bde12209717f1601b27e3d6e137ed9122d3d9c519a3d.scope/hugetlb.1GB.usage_in_bytes
./hugetlb.1GB.max_usage_in_bytes
./hugetlb.1GB.usage_in_bytes
[root@overcloud-computesriov-0 system.slice]# find . -name '*usage_in_bytes' | while read line; do echo $line ; cat $line ; done
./docker-b9ff1a61cc4f144cc2aa16332d8e07248ad71a7263056b0d9cddb7339368457a.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-b9ff1a61cc4f144cc2aa16332d8e07248ad71a7263056b0d9cddb7339368457a.scope/hugetlb.1GB.usage_in_bytes
0
./docker-dce63a96f513527b894bbec6c7f39f40dd2912bdbf4dec0a51b2e59704c03e7b.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-dce63a96f513527b894bbec6c7f39f40dd2912bdbf4dec0a51b2e59704c03e7b.scope/hugetlb.1GB.usage_in_bytes
0
./docker-c538d8c4e222b977b218746ee9ebef34335d768a364fbe1bfb3e72284d65520a.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-c538d8c4e222b977b218746ee9ebef34335d768a364fbe1bfb3e72284d65520a.scope/hugetlb.1GB.usage_in_bytes
0
./docker-d2e87b4ec13cc51c9dab3e593ee44051eb243ce684cc300b7e6103e8f35e1320.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-d2e87b4ec13cc51c9dab3e593ee44051eb243ce684cc300b7e6103e8f35e1320.scope/hugetlb.1GB.usage_in_bytes
0
./docker-90030441400dd9536aa33d13d3d5792a4e1f025fb383141cb0f18cfaed260979.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-90030441400dd9536aa33d13d3d5792a4e1f025fb383141cb0f18cfaed260979.scope/hugetlb.1GB.usage_in_bytes
0
./docker-1286b301010bb53e3d919616054e645c00a2288b9cdc8235bdb68aa404a0c34b.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-1286b301010bb53e3d919616054e645c00a2288b9cdc8235bdb68aa404a0c34b.scope/hugetlb.1GB.usage_in_bytes
0
./docker-111c9c039324d640875e550763d6507450cbbd07f6674c3883388839807cd614.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-111c9c039324d640875e550763d6507450cbbd07f6674c3883388839807cd614.scope/hugetlb.1GB.usage_in_bytes
0
./docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope/hugetlb.1GB.max_usage_in_bytes
4294967296
./docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope/hugetlb.1GB.usage_in_bytes
4294967296
./docker-46ed5e7b2045df285552bde12209717f1601b27e3d6e137ed9122d3d9c519a3d.scope/hugetlb.1GB.max_usage_in_bytes
0
./docker-46ed5e7b2045df285552bde12209717f1601b27e3d6e137ed9122d3d9c519a3d.scope/hugetlb.1GB.usage_in_bytes
0
./hugetlb.1GB.max_usage_in_bytes
4294967296
./hugetlb.1GB.usage_in_bytes
4294967296
[root@overcloud-computesriov-0 system.slice]# docker ps | grep a94d9089
a94d9089fac5        registry.access.redhat.com/rhosp13/openstack-nova-libvirt:13.0-72                "kolla_start"       3 days ago          Up 3 days                                 nova_libvirt
[root@overcloud-computesriov-0 system.slice]# systemctl status docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope
● docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope - libcontainer container a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d
   Loaded: loaded (/run/systemd/system/docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope; static; vendor preset: disabled)
  Drop-In: /run/systemd/system/docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope.d
           └─50-BlockIOAccounting.conf, 50-CPUAccounting.conf, 50-DefaultDependencies.conf, 50-Delegate.conf, 50-Description.conf, 50-MemoryAccounting.conf, 50-Slice.conf
   Active: active (running) since Tue 2018-11-27 06:46:11 UTC; 3 days ago
    Tasks: 18
   Memory: 14.8M
   CGroup: /system.slice/docker-a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.scope
           └─33304 /usr/sbin/libvirtd

Nov 27 06:46:11 overcloud-computesriov-0 systemd[1]: Started libcontainer container a94d9089fac5ed4b1dbe48b0f5460536462c49e0ff14fd4059fbae7a7dbd1b4d.
Nov 27 06:46:11 overcloud-computesriov-0 sudo[33325]:     root : TTY=unknown ; PWD=/ ; USER=root ; COMMAND=/usr/local/bin/kolla_set_configs
[root@overcloud-computesriov-0 system.slice]# 
~~~

### memory cgroup ###

- keep track of pages used by each group
- allow the OOM (out of memory) killer to trigger on a specific memory cgroup only
- kernel can "freeze" the cgroup

### blockio cgroup ###

- keep track of I/Os for each group
- throttle each group
- writes go through page cache unless O_DIRECT is set

### net_cls cgroup ###

- automatically set traffic classs for egress traffic (use tc/iptables)

### net_prio cgroup ###

- automatically set traffic classs for egress traffic (use queuing disciplines)

### devices cgroup ###

- which group can read/write from which device in /dev

### freezer cgroup ###

- used to freeze / stop all processes in a group (SIGSTOP / SIGCONT)

### mounting, unmounting and comounting cgroup v1 resource controllers ###
`man 7 cgroups`
~~~
(...)
It is possible to comount multiple controllers  against  the  same  hierarchy.   For
example, here the cpu and cpuacct controllers are comounted against a single hierar‐
chy:
    mount -t cgroup -o cpu,cpuacct none /sys/fs/cgroup/cpu,cpuacct
Comounting controllers has the effect that a process is in the same cgroup  for  all
of  the  comounted controllers.  Separately mounting controllers allows a process to
be in cgroup /foo1 for one controller while being in /foo2/foo3 for another.
It is possible to comount all v1 controllers against the same hierarchy:
    mount -t cgroup -o all cgroup /sys/fs/cgroup
(One can achieve the same result by omitting -o all, since it is the default  if  no
controllers are explicitly specified.)
(...)
~~~

~~~
[akaris@wks-akaris blog]$ mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup2 on /sys/fs/cgroup/unified type cgroup2 (rw,nosuid,nodev,noexec,relatime,seclabel,nsdelegate)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,xattr,name=systemd)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpu,cpuacct)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,hugetlb)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,perf_event)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,memory)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,pids)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,freezer)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,net_cls,net_prio)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,blkio)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,devices)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpuset)
[akaris@wks-akaris blog]$ ls /sys/fs/cgroup
blkio  cpu  cpuacct  cpu,cpuacct  cpuset  devices  freezer  hugetlb  memory  net_cls  net_cls,net_prio  net_prio  perf_event  pids  systemd  unified
[akaris@wks-akaris blog]$ 
~~~

`man 7 cgroup`
~~~
(...)
It is not possible to mount the same controller against multiple cgroup hierarchies.
For example, it is not possible to  mount  both  the  cpu  and  cpuacct  controllers
against one hierarchy, and to mount the cpu controller alone against another hierar‐
chy.  It is possible to create multiple mount points with exactly the  same  set  of
comounted  controllers.   However,  in  this case all that results is multiple mount
points providing a view of the same hierarchy.
(...)
~~~

~~~
[root@rhel8 ~]# mount | grep cgroup
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,xattr,release_agent=/usr/lib/systemd/systemd-cgroups-agent,name=systemd)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,freezer)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpu,cpuacct)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,memory)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,net_cls,net_prio)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,pids)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,blkio)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,hugetlb)
cgroup on /sys/fs/cgroup/rdma type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,rdma)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,devices)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpuset)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,perf_event)
cgroup2 on /mnt/cgroupv2 type cgroup2 (rw,relatime,seclabel)
[root@rhel8 ~]# mount group -t net_cls /sys/fs/cgroup/net_cls
mount: /sys/fs/cgroup/net_cls,net_prio: unknown filesystem type 'net_cls'.
[root@rhel8 ~]# mount group -t cgroup -o net_cls /sys/fs/cgroup/net_cls
mount: /sys/fs/cgroup/net_cls,net_prio: group already mounted or mount point busy.
[root@rhel8 ~]# mount group -t cgroup -o net_cls /mnt
mount: /mnt: group already mounted or mount point busy.
[root@rhel8 ~]# mount group -t cgroup -o net_cls,net_prio /mnt
[root@rhel8 ~]# umount /mnt
~~~

`man 7 cgroups`
~~~
> (...)
Unmounting v1 controllers
A  mounted cgroup filesystem can be unmounted using the umount(8) command, as in the
following example:
    umount /sys/fs/cgroup/pids
But note well: a cgroup filesystem is unmounted only if it is not busy, that is,  it
has  no  child  cgroups.   If  this  is  not  the  case, then the only effect of the
umount(8) is to make the mount invisible.  Thus, to ensure that the mount  point  is
really  removed,  one must first remove all child cgroups, which in turn can be done
only after all member processes have been moved  from  those  cgroups  to  the  root
cgroup.
(...)
~~~

## Processes in cgroups ##

Processes start in the same cgroups as their parent

A process can be moved by:
~~~
echo $id > /sys/fs/cgroup/.../tasks
~~~

### Listing the cgroups that a process is in ###

[https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt](https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt):
~~~
Each task under /proc has an added file named 'cgroup' displaying,
for each active hierarchy, the subsystem names and the cgroup name
as the path relative to the root of the cgroup file system.
~~~

Example:
~~~
[root@rhospbl-1 ~]# cat /proc/$(pidof libvirtd)/cgroup
11:pids:/
10:memory:/system.slice
9:perf_event:/
8:cpuset:/
7:cpuacct,cpu:/system.slice
6:freezer:/
5:hugetlb:/
4:devices:/system.slice/libvirtd.service
3:blkio:/system.slice
2:net_prio,net_cls:/
1:name=systemd:/system.slice/libvirtd.service
~~~

## notify_on_release ##

~~~
If the notify_on_release flag is enabled (1) in a cgroup, then
whenever the last task in the cgroup leaves (exits or attaches to
some other cgroup) and the last child cgroup of that cgroup
is removed, then the kernel runs the command specified by the contents
of the "release_agent" file in that hierarchy's root directory,
supplying the pathname (relative to the mount point of the cgroup
file system) of the abandoned cgroup.  This enables automatic
removal of abandoned cgroups.  The default value of
notify_on_release in the root cgroup at system boot is disabled
(0).  The default value of other cgroups at creation is the current
value of their parents' notify_on_release settings. The default value of
a cgroup hierarchy's release_agent path is empty.
~~~

Example:
~~~
[root@overcloud-controller-0 ~]# systemctl status session-c2.scope
● session-c2.scope - Session c2 of user rabbitmq
   Loaded: loaded (/run/systemd/system/session-c2.scope; static; vendor preset: disabled)
  Drop-In: /run/systemd/system/session-c2.scope.d
           └─50-After-systemd-logind\x2eservice.conf, 50-After-systemd-user-sessions\x2eservice.conf, 50-Description.conf, 50-SendSIGHUP.conf, 50-Slice.conf, 50-TasksMax.conf
   Active: active (abandoned) since Wed 2018-11-07 22:40:43 UTC; 2 weeks 5 days ago
   CGroup: /user.slice/user-975.slice/session-c2.scope
           └─22384 /usr/lib64/erlang/erts-7.3.1.4/bin/epmd -daemon

Warning: Journal has been rotated since unit was started. Log output is incomplete or unavailable.
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/
blkio/            cpu,cpuacct/      freezer/          net_cls/          perf_event/       
cpu/              cpuset/           hugetlb/          net_cls,net_prio/ pids/             
cpuacct/          devices/          memory/           net_prio/         systemd/          
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/
blkio/            cpu,cpuacct/      freezer/          net_cls/          perf_event/       
cpu/              cpuset/           hugetlb/          net_cls,net_prio/ pids/             
cpuacct/          devices/          memory/           net_prio/         systemd/          
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/
blkio/            cpu,cpuacct/      freezer/          net_cls/          perf_event/       
cpu/              cpuset/           hugetlb/          net_cls,net_prio/ pids/             
cpuacct/          devices/          memory/           net_prio/         systemd/          
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/systemd/
cgroup.clone_children  cgroup.procs           machine.slice/         release_agent          tasks
cgroup.event_control   cgroup.sane_behavior   notify_on_release      system.slice/          user.slice/
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/systemd/
cgroup.clone_children  cgroup.procs           machine.slice/         release_agent          tasks
cgroup.event_control   cgroup.sane_behavior   notify_on_release      system.slice/          user.slice/
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/systemd/user.slice/user-975.slice/session-c2.scope/
cgroup.clone_children  cgroup.event_control   cgroup.procs           notify_on_release      tasks
[root@overcloud-controller-0 ~]# cat /sys/fs/cgroup/systemd/user.slice/user-975.slice/session-c2.scope/notify_on_release 
1
~~~

### Resources ###
[https://en.wikipedia.org/wiki/Cgroups](https://en.wikipedia.org/wiki/Cgroups)

[https://www.kernel.org/doc/Documentation/cgroup-v1/](https://www.kernel.org/doc/Documentation/cgroup-v1/)

[https://lwn.net/Articles/679786/](https://lwn.net/Articles/679786/)

[https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/resource_management_guide/index](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/resource_management_guide/index)

`man 7 cgroups`

[https://www.youtube.com/watch?v=sK5i-N34im8](https://www.youtube.com/watch?v=sK5i-N34im8)
