# Linux namespaces #

## What is a namespace? ##

- one important component of containers

- process within the namespace sees its own isolated instance of a resource

- the resources are visible only to other processes that are members of the namespace 

- limit what you can view on a system

- each process is a member of one namespace of each resource

- namespace resources are: Cgroup, IPC, Network, Mount, PID, User, UTS

`man 7 namespaces`:
~~~
(...)
A  namespace  wraps a global system resource in an abstraction that makes it appear to the processes within the namespace that they have their own isolated instance of the global resource.  Changes to the global resource are visible to other processes that are members of the namespace, but are invisible to other processes.  One use  of namespaces  is  to implement containers.
(...)
~~~

## unshare - the tool to create new namespaces ##

A child process starts within the same namespaces as its parent. In order to move a process to a new namespace, it needs to be spawned with `unshare`:
~~~
man unshare

NAME
       unshare - run program with some namespaces unshared from parent

SYNOPSIS
       unshare [options] [program [arguments]]

DESCRIPTION
       Unshares the indicated namespaces from the parent process and then executes the specified program. If program is not given, then ``${SHELL}'' is run (default: /bin/sh).

       The  namespaces can optionally be made persistent by bind mounting /proc/pid/ns/type files to a filesystem path and entered with nsenter(1) even after the program terminates (except PID namespaces
       where permanently running init process is required).  Once a persistent namespace is no longer needed, it can be unpersisted with umount(8).  See the EXAMPLES section for more details.

       The namespaces to be unshared are indicated via options.  Unshareable namespaces are:

       mount namespace
              Mounting and unmounting filesystems will not affect the rest of the system, except for filesystems which are explicitly marked as shared (with mount --make-shared; see  /proc/self/mountinfo
              or findmnt -o+PROPAGATION for the shared flags).  For further details, see mount_namespaces(7) and the discussion of the CLONE_NEWNS flag in clone(2).

              unshare  since  util-linux  version 2.27 automatically sets propagation to private in a new mount namespace to make sure that the new namespace is really unshared.  It's possible to disable
              this feature with option --propagation unchanged.  Note that private is the kernel default.

       UTS namespace
              Setting hostname or domainname will not affect the rest of the system.  For further details, see namespaces(7) and the discussion of the CLONE_NEWUTS flag in clone(2).

       IPC namespace
              The process will have an independent namespace for POSIX message queues as well as System V message queues, semaphore sets and shared memory  segments.   For  further  details,  see  names‐
              paces(7) and the discussion of the CLONE_NEWIPC flag in clone(2).

       network namespace
              The  process  will have independent IPv4 and IPv6 stacks, IP routing tables, firewall rules, the /proc/net and /sys/class/net directory trees, sockets, etc.  For further details, see names‐
              paces(7) and the discussion of the CLONE_NEWNET flag in clone(2).

       PID namespace
              Children will have a distinct set of PID-to-process mappings from their parent.  For further details, see pid_namespaces(7) and the discussion of the CLONE_NEWPID flag in clone(2).

       cgroup namespace
              The process will have a virtualized view of /proc/self/cgroup, and new cgroup mounts will be rooted at the namespace cgroup root.  For further details, see cgroup_namespaces(7) and the dis‐
              cussion of the CLONE_NEWCGROUP flag in clone(2).

       user namespace
              The process will have a distinct set of UIDs, GIDs and capabilities.  For further details, see user_namespaces(7) and the discussion of the CLONE_NEWUSER flag in clone(2).

OPTIONS
       -i, --ipc[=file]
              Unshare the IPC namespace.  If file is specified, then a persistent namespace is created by a bind mount.

       -m, --mount[=file]
              Unshare  the  mount namespace.  If file is specified, then a persistent namespace is created by a bind mount.  Note that file has to be located on a filesystem with the propagation flag set
              to private.  Use the command findmnt -o+PROPAGATION when not sure about the current setting.  See also the examples below.

       -n, --net[=file]
              Unshare the network namespace.  If file is specified, then a persistent namespace is created by a bind mount.

       -p, --pid[=file]
              Unshare the PID namespace.  If file is specified then persistent namespace is created by a bind mount.  See also the --fork and --mount-proc options.

       -u, --uts[=file]
              Unshare the UTS namespace.  If file is specified, then a persistent namespace is created by a bind mount.

       -U, --user[=file]
              Unshare the user namespace.  If file is specified, then a persistent namespace is created by a bind mount.

       -C, --cgroup[=file]
              Unshare the cgroup namespace. If file is specified then persistent namespace is created by bind mount.

       -f, --fork
              Fork the specified program as a child process of unshare rather than running it directly.  This is useful when creating a new PID namespace.

(...)
~~~

`unshare` uses the `unshare` system call:
~~~
[akaris@wks-akaris ~]$ sudo strace -tt -f -s1024 unshare -u /bin/bash 2>&1 | grep -i uts
15:54:50.016682 unshare(CLONE_NEWUTS)   = 0
~~~

~~~
man unshare
UNSHARE(2)                         Linux Programmer's Manual                         UNSHARE(2)

NAME
       unshare - disassociate parts of the process execution context

SYNOPSIS
       #define _GNU_SOURCE
       #include <sched.h>

       int unshare(int flags);

DESCRIPTION
       unshare()  allows  a  process (or thread) to disassociate parts of its execution context
       that are currently being shared with other processes (or threads).  Part of  the  execu‐
       tion  context,  such  as the mount namespace, is shared implicitly when a new process is
       created using fork(2) or vfork(2), while other parts, such as  virtual  memory,  may  be
       shared by explicit request when creating a process or thread using clone(2).

       The  main use of unshare() is to allow a process to control its shared execution context
       without creating a new process.

(...)
~~~

However, namespaces can also be created by feeding flags to the clone system call:
~~~
[akaris@wks-akaris ~]$ man clone | egrep '^[ ]+CLONE_NEW'
       CLONE_NEWCGROUP (since Linux 4.6)
       CLONE_NEWIPC (since Linux 2.6.19)
       CLONE_NEWNET (since Linux 2.6.24)
       CLONE_NEWNS (since Linux 2.4.19)
       CLONE_NEWPID (since Linux 2.6.24)
       CLONE_NEWUSER
       CLONE_NEWUTS (since Linux 2.6.19)
              CLONE_NEWPID  was  specified  in flags, but the limit on the nesting depth of PID
              CLONE_NEWUSER was specified in flags, and the call would cause the limit  on  the
              CLONE_NEWUTS  was  specified  by  an  unprivileged   process   (process   without
              CLONE_NEWUSER was specified in flags and the caller is in  a  chroot  environment
              CLONE_NEWUSER  was specified in flags, and the limit on the number of nested user
~~~

## How to list namespaces ##

Note that when a namespace is unshared, it will show up with a different namespace ID in `/proc/$PID/ns/`:
~~~
[root@wks-akaris akaris]# sudo ls /proc/1/ns/ -al
total 0
dr-x--x--x. 2 root root 0 Dec 20 15:47 .
dr-xr-xr-x. 9 root root 0 Dec 20 09:59 ..
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 ipc -> 'ipc:[4026531839]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 mnt -> 'mnt:[4026531840]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 net -> 'net:[4026532008]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 pid -> 'pid:[4026531836]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 pid_for_children -> 'pid:[4026531836]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 user -> 'user:[4026531837]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:47 uts -> 'uts:[4026531838]'
[root@wks-akaris akaris]# sudo ls /proc/$$/ns/ -al
total 0
dr-x--x--x. 2 root root 0 Dec 20 15:45 .
dr-xr-xr-x. 9 root root 0 Dec 20 15:43 ..
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 ipc -> 'ipc:[4026531839]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 mnt -> 'mnt:[4026531840]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 net -> 'net:[4026532008]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 pid -> 'pid:[4026531836]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 pid_for_children -> 'pid:[4026531836]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 user -> 'user:[4026531837]'
lrwxrwxrwx. 1 root root 0 Dec 20 15:45 uts -> 'uts:[4026532828]'
~~~

Or, a more compact example of the above:
~~~
[akaris@wks-akaris ~]$ sudo unshare -m uts /bin/bash
[sudo] password for akaris: 
unshare: failed to execute uts: No such file or directory
[akaris@wks-akaris ~]$ sudo unshare -u /bin/bash
[root@wks-akaris akaris]# readlink /proc/1/ns/uts
uts:[4026531838]
[root@wks-akaris akaris]# readlink /proc/$$/ns/uts
uts:[4026532828]
~~~

The easiest and most detailed way to show the namespace hierarchy, though, is `lsns`:
~~~
man lsns
LSNS(8)                                                                                 System Administration                                                                                 LSNS(8)

NAME
       lsns - list namespaces

SYNOPSIS
       lsns [options] [namespace]

DESCRIPTION
       lsns lists information about all the currently accessible namespaces or about the given namespace.  The namespace identifier is an inode number.

       The  default  output  is  subject  to  change.  So whenever possible, you should avoid using default outputs in your scripts.  Always explicitly define expected columns by using the --output
       option together with a columns list in environments where a stable output is required.

       NSFS column, printed when net is specified for --type option, is special; it uses multi-line cells.  Use the option --nowrap is for switching to "," separated single-line representation.

       Note that lsns reads information directly from the /proc filesystem and for non-root users it may return incomplete information.  The current /proc filesystem may be unshared and affected by
       a  PID  namespace  (see  unshare  --mount-proc  for  more  details).   lsns is not able to see persistent namespaces without processes where the namespace instance is held by a bind mount to
       /proc/pid/ns/type.
(...)
~~~

Example:
~~~
[akaris@wks-akaris ~]$ sudo lsns -t uts
        NS TYPE NPROCS PID USER COMMAND
4026531838 uts     288   1 root /usr/lib/systemd/systemd --switched-root --system --deserialize 32
[akaris@wks-akaris ~]$ sudo unshare -u /bin/bash
[root@wks-akaris akaris]# lsns -t uts
        NS TYPE NPROCS   PID USER COMMAND
4026531838 uts     289     1 root /usr/lib/systemd/systemd --switched-root --system --deserialize 32
4026532849 uts       2 13226 root /bin/bash
[root@wks-akaris akaris]# unshare -u /bin/bash
[root@wks-akaris akaris]# lsns -t uts
        NS TYPE NPROCS   PID USER COMMAND
4026531838 uts     289     1 root /usr/lib/systemd/systemd --switched-root --system --deserialize 32
4026532849 uts       1 13226 root /bin/bash
4026532850 uts       2 13257 root /bin/bash
[root@wks-akaris akaris]# readlink /proc/$$/ns/uts
uts:[4026532850]
[root@wks-akaris akaris]# exit
exit
[root@wks-akaris akaris]# readlink /proc/$$/ns/uts
uts:[4026532849]
[root@wks-akaris akaris]# exit
exit
[akaris@wks-akaris ~]$ readlink /proc/$$/ns/uts
uts:[4026531838]
~~~

## namespace types ##

### UTS namespace ###

- UTS stands for the datastructure that stores the identifiers returned by uname. 

- UTS namespaces allow every process to have its own hostname and domain name.

#### Example ####

Create an instance of `/bin/bash` in its own UTS namespace and assign it its own hostname:
~~~
[akaris@wks-akaris ~]$ sudo unshare -u /bin/bash
[root@wks-akaris akaris]# hostname utsnamespace
[root@wks-akaris akaris]# bash
[root@utsnamespace akaris]#
[root@wks-akaris akaris]# hostname
utsnamespace
[root@wks-akaris akaris]# uname -a
Linux utsnamespace 4.18.12-200.fc28.x86_64 #1 SMP Thu Oct 4 15:46:35 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux
~~~

Open another CLI on the same system and verify the hostname:
~~~
[akaris@wks-akaris ~]$ hostname
wks-akaris
[akaris@wks-akaris ~]$ uname -a
Linux wks-akaris 4.18.12-200.fc28.x86_64 #1 SMP Thu Oct 4 15:46:35 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux
~~~

### network namespaces ###

- Allow to add a private network stack to processes within a network namespace.

- Let you move interfaces from one network namespace to another.

- veth interfaces can connect namespaces together.

~~~
man ip-netns
(...)
       A network namespace is logically another copy of the network stack, with its own routes, firewall rules, and network devices.

       By default a process inherits its network namespace from its parent. Initially all the processes share the same default network namespace from the init process.

       By convention a named network namespace is an object at /var/run/netns/NAME that can be opened. The file descriptor resulting from opening /var/run/netns/NAME refers to the spec‐
       ified network namespace. Holding that file descriptor open keeps the network namespace alive. The file descriptor can be used with the setns(2) system call to change the network
       namespace associated with a task.
(...)
~~~

Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems:
~~~
Network namespaces provide isolation of network controllers, system resources associated
with networking, firewall and routing tables. This allows container to use separate virtual
network stack, loopback device and process space. You can add virtual or real devices to the
container, assign them their own IP Addresses and even full iptables rules. You can view the
different network settings by executing the ip addr command on the host and inside the
container.
~~~

#### Example ####

There are 2 ways to generate persistent network namespaces. Either use `ip netns add`, or use the more inconvenient `unshare` command:
~~~
[akaris@wks-akaris ~]$ sudo ip netns add netns1
[akaris@wks-akaris ~]$ sudo ip netns 
netns1
[akaris@wks-akaris ~]$ sudo mount | grep netns
tmpfs on /run/netns type tmpfs (rw,nosuid,nodev,seclabel,mode=755)
nsfs on /run/netns/netns1 type nsfs (rw,seclabel)
nsfs on /run/netns/netns1 type nsfs (rw,seclabel)
[akaris@wks-akaris ~]$ sudo touch /run/netns/netns2
[akaris@wks-akaris ~]$ sudo unshare --net=!$ /bin/bash
sudo unshare --net=/run/netns/netns2 /bin/bash
[root@wks-akaris akaris]# exit
exit
[akaris@wks-akaris ~]$ sudo ip netns
netns2
netns1
[akaris@wks-akaris ~]$ sudo mount | grep netns
tmpfs on /run/netns type tmpfs (rw,nosuid,nodev,seclabel,mode=755)
nsfs on /run/netns/netns1 type nsfs (rw,seclabel)
nsfs on /run/netns/netns1 type nsfs (rw,seclabel)
nsfs on /run/netns/netns2 type nsfs (rw,seclabel)
nsfs on /run/netns/netns2 type nsfs (rw,seclabel)
~~~

In both cases, you will be able to enter the namespace and create interfaces therein:
~~~
[akaris@wks-akaris ~]$ sudo ip netns exec netns2 /bin/bash
[root@wks-akaris akaris]# ip link ls
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
[root@wks-akaris akaris]# sudo ip link add veth0 type veth  peer name veth1
[root@wks-akaris akaris]# ip link ls
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: veth1@veth0: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 1e:a0:b3:b7:e6:db brd ff:ff:ff:ff:ff:ff
3: veth0@veth1: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether ee:1a:b2:f0:07:87 brd ff:ff:ff:ff:ff:ff
[root@wks-akaris akaris]# exit
exit
[akaris@wks-akaris ~]$ sudo ip netns exec netns1 /bin/bash
[root@wks-akaris akaris]# ip link ls
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
[root@wks-akaris akaris]# exit
exit
~~~

### mount namespaces ###

- mount namespaces allow processes to have their own mount points, such as :root fs, /tmp, /proc/, etc.

- within a mount namespace, it's possible to mount and unmount mount points without affecting any other namespace

~~~
man unshare
(...)
       mount namespace
              Mounting and unmounting filesystems will not affect the rest of the system, except for filesystems which are explicitly  marked
              as  shared  (with  mount  --make-shared; see /proc/self/mountinfo or findmnt -o+PROPAGATION for the shared flags).  For further
              details, see mount_namespaces(7) and the discussion of the CLONE_NEWNS flag in clone(2).

              unshare since util-linux version 2.27 automatically sets propagation to private in a new mount namespace to make sure that  the
              new  namespace  is really unshared.  It's possible to disable this feature with option --propagation unchanged.  Note that pri‐
              vate is the kernel default.
(...)
~~~

~~~
man mount_namespaces
(...)
       Mount  namespaces  provide  isolation  of the list of mount points seen by the processes in each namespace instance.  Thus, the processes in each of the mount namespace instances
       will see distinct single-directory hierarchies.

       The views provided by the /proc/[pid]/mounts, /proc/[pid]/mountinfo, and /proc/[pid]/mountstats files (all described in proc(5)) correspond to the mount namespace  in  which  the
       process with the PID [pid] resides.  (All of the processes that reside in the same mount namespace will see the same view in these files.)

       When  a  process  creates a new mount namespace using clone(2) or unshare(2) with the CLONE_NEWNS flag, the mount point list for the new namespace is a copy of the caller's mount
       point list.  Subsequent modifications to the mount point list (mount(2) and umount(2)) in either mount namespace will not (by default) affect the mount point  list  seen  in  the
       other namespace (but see the following discussion of shared subtrees).
(...)
~~~

Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems:
~~~
Mount namespaces isolate the set of file system mount points seen by a group of processes so
that processes in different mount namespaces can have different views of the file system
hierarchy. With mount namespaces, the mount() and umount() system calls cease to operate
on a global set of mount points (visible to all processes) and instead perform operations that
affect just the mount namespace associated with the container process. For example, each
container can have its own /tmp or /var directory or even have an entirely different
userspace.
~~~

#### Example ####
~~~
[akaris@wks-akaris ~]$ sudo unshare -m /bin/bash
[root@wks-akaris akaris]# lsns -t mnt
        NS TYPE NPROCS   PID USER   COMMAND
4026531840 mnt     297     1 root   /usr/lib/systemd/systemd --switched-root --system --deserialize 32
4026531860 mnt       1    33 root   kdevtmpfs
4026532186 mnt       1   983 root   /usr/lib/systemd/systemd-udevd
4026532508 mnt       1  1261 rtkit  /usr/libexec/rtkit-daemon
4026532509 mnt       1  1276 chrony /usr/sbin/chronyd
4026532510 mnt       4  1460 root   /usr/sbin/NetworkManager --no-daemon
4026532511 mnt       1  2481 root   /usr/libexec/bluetooth/bluetoothd
4026532514 mnt       1  3045 root   /usr/libexec/fwupd/fwupd
4026532665 mnt       1  1541 colord /usr/libexec/colord
4026532749 mnt       1  2030 root   /usr/libexec/boltd
4026532828 mnt       2 17149 root   /bin/bash
[root@wks-akaris akaris]# umount -a -l
[root@wks-akaris akaris]# ls /dev/
[root@wks-akaris akaris]# ls /proc
[root@wks-akaris akaris]# exit
exit
[akaris@wks-akaris ~]$ ls /dev | head -2
autofs
block
~~~

### PID namespaces ###

- isolate the process ID number space

- different processes in different PID namespaces can have the same PID

- a process can only see processes in its own PID namespace. 

- a process has a PID per namespace

- The global namespace has a different PID for the same process than the 'custom' namespace

~~~
man pid_namespaces
(...)
       PID  namespaces  isolate  the process ID number space, meaning that processes in different PID namespaces can have the same PID.  PID namespaces allow containers to provide func‐
       tionality such as suspending/resuming the set of processes in the container and migrating the container to a new host while the processes inside the container maintain  the  same
       PIDs.

       PIDs in a new PID namespace start at 1, somewhat like a standalone system, and calls to fork(2), vfork(2), or clone(2) will produce processes with PIDs that are unique within the
       namespace.
(...)
       The first process created in a new namespace (i.e., the process created using clone(2) with the CLONE_NEWPID flag, or the first child  created  by  a  process  after  a  call  to
       unshare(2)  using  the CLONE_NEWPID flag) has the PID 1, and is the "init" process for the namespace (see init(1)).  A child process that is orphaned within the namespace will be
       reparented to this process rather than init(1) (unless one of the ancestors of the child in the same PID namespace employed the prctl(2) PR_SET_CHILD_SUBREAPER  command  to  mark
       itself as the reaper of orphaned descendant processes).

       If  the  "init" process of a PID namespace terminates, the kernel terminates all of the processes in the namespace via a SIGKILL signal.  This behavior reflects the fact that the
       "init" process is essential for the correct operation of a PID namespace.  In this case, a subsequent fork(2) into this PID namespace fail with the error ENOMEM; it is not possi‐
       ble  to  create  a new processes in a PID namespace whose "init" process has terminated. 
(...)
~~~

Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems:
~~~
PID namespaces allow processes in different containers to have the same PID, so each
container can have its own init (PID1) process that manages various system initialization tasks
as well as containers life cycle. Also, each container has its unique /proc directory. Note that
from within the container you can monitor only processes running inside this container. In
other words, the container is only aware of its native processes and can not "see" the
processes running in different parts of the system. On the other hand, the host operating
system is aware of processes running inside of the container, but assigns them different PID
numbers. For example, run the ps -eZ | grep systemd$ command on host to see all
instances of systemd including those running inside of containers.
~~~

#### Example ####

Note that PID namespaces need to be unshared with `--fork`. Also note that this will reset the PID space, however this will not show in `ps` as the information comes from the /proc file system:
~~~
[akaris@wks-akaris ~]$ sudo unshare -p --fork /bin/bash
[root@wks-akaris akaris]# echo $$
1
[root@wks-akaris akaris]# ps
  PID TTY          TIME CMD
17365 pts/5    00:00:00 sudo
17367 pts/5    00:00:00 unshare
17368 pts/5    00:00:00 bash
17399 pts/5    00:00:00 ps
~~~

We need a more complex series of commands to display the actual process IDs in `ps` - we need to create both a new PID and a new mount namespace and then unmount and remount proc:
~~~
[akaris@wks-akaris ~]$ sudo unshare -p --fork -m /bin/bash
[root@wks-akaris akaris]# umount -l /proc
[root@wks-akaris akaris]# mount proc /proc -t proc
[root@wks-akaris akaris]# ps aux
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.1  0.0 123344  5916 pts/5    S    19:01   0:00 /bin/bash
root        31  0.0  0.0 153224  3700 pts/5    R+   19:01   0:00 ps aux
[root@wks-akaris akaris]# 
~~~

### user namespaces ###

- Allows UID/GID mapping. 

- Within the namespace, a user can have UID 0 (root) but this will be squashed to root outside of the container.

~~~
man user_namespaces
(...)
       User  namespaces  isolate security-related identifiers and attributes, in particular, user IDs and group IDs (see credentials(7)), the root directory, keys (see keyrings(7)), and
       capabilities (see capabilities(7)).  A process's user and group IDs can be different inside and outside a user namespace.  In particular, a process can have a normal unprivileged
       user ID outside a user namespace while at the same time having a user ID of 0 inside the namespace; in other words, the process has full privileges for operations inside the user
       namespace, but is unprivileged for operations outside the namespace.
(...)
~~~

Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems:
~~~
User namespaces are similar to PID
namespaces, they allow you to specify a range of host UIDs dedicated to the container. Consequently, a
process can have full root privileges for operations inside the container, and at the same time be
unprivileged for operations outside the container. For compatibility reasons, user namespaces are
turned off in the current version of Red Hat Enterprise Linux 7, but will be enabled in the near future.
~~~

#### Example ####

~~~
[akaris@wks-akaris ~]$ sudo unshare -U /bin/bash
[nfsnobody@wks-akaris akaris]$ id
uid=65534(nfsnobody) gid=65534(nfsnobody) groups=65534(nfsnobody) context=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023
[nfsnobody@wks-akaris akaris]$ echo $$
10086
~~~

Open another CLI and run:
~~~
[akaris@wks-akaris ~]$ sudo newuidmap 10086 0 0 1
[akaris@wks-akaris ~]$ 
~~~

Now, within the namespace, verify what happened:
~~~
[nfsnobody@wks-akaris akaris]$ id
uid=0(root) gid=65534(nfsnobody) groups=65534(nfsnobody) context=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023
[nfsnobody@wks-akaris akaris]$ 
~~~

### IPC namespaces ###

- Isolates interprocess communication

- A process / container can have its own semaphores, message queues, shared memory, etc

Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems:
~~~
IPC namespaces isolate certain interprocess communication (IPC) resources, such as System
V IPC objects and POSIX message queues. This means that two containers can create shared
memory segments and semaphores with the same name, but are not able to interact with other
containers memory segments or shared memory.
~~~

### cgroup namespaces and their purpose ###

~~~
man cgroup_namespaces
(...)
       Among the purposes served by the virtualization provided by cgroup namespaces are the following:

       * It prevents information leaks whereby cgroup directory paths outside of a container would otherwise be visible to processes in the container.  Such leakages could, for example,
         reveal information about the container framework to containerized applications.

       * It eases tasks such as container migration.  The virtualization provided by cgroup namespaces allows containers to be isolated from  knowledge  of  the  pathnames  of  ancestor
         cgroups.  Without such isolation, the full cgroup pathnames (displayed in /proc/self/cgroups) would need to be replicated on the target system when migrating a container; those
         pathnames would also need to be unique, so that they don't conflict with other pathnames on the target system.

       * It allows better confinement of containerized processes, because it is possible to mount the container's cgroup filesystems such that the container processes can't gain  access
         to ancestor cgroup directories.  Consider, for example, the following scenario:

           · We have a cgroup directory, /cg/1, that is owned by user ID 9000.

           · We  have a process, X, also owned by user ID 9000, that is namespaced under the cgroup /cg/1/2 (i.e., X was placed in a new cgroup namespace via clone(2) or unshare(2) with
             the CLONE_NEWCGROUP flag).

         In the absence of cgroup namespacing, because the cgroup directory /cg/1 is owned (and writable) by UID 9000 and process X is also owned by user ID 9000, then process  X  would
         be able to modify the contents of cgroups files (i.e., change cgroup settings) not only in /cg/1/2 but also in the ancestor cgroup directory /cg/1.  Namespacing process X under
         the cgroup directory /cg/1/2, in combination with suitable mount operations for the cgroup filesystem (as shown above), prevents it modifying files in /cg/1,  since  it  cannot
         even  see the contents of that directory (or of further removed cgroup ancestor directories).  Combined with correct enforcement of hierarchical limits, this prevents process X
         from escaping the limits imposed by ancestor cgroups.
(...)
~~~

## Building containers - the namespace part of it ##

A nice walkthrough can be found in this youtube video:
[https://youtu.be/sK5i-N34im8?t=2476](https://youtu.be/sK5i-N34im8?t=2476)

## Resources ##

- manpages:
~~~
[akaris@wks-akaris blog]$ apropos namespace | grep _namespaces
cgroup_namespaces (7) - overview of Linux cgroup namespaces
mount_namespaces (7) - overview of Linux mount namespaces
network_namespaces (7) - overview of Linux network namespaces
pid_namespaces (7)   - overview of Linux PID namespaces
user_namespaces (7)  - overview of Linux user namespaces
[akaris@wks-akaris blog]$ man 7 namespaces
~~~

- [https://www.youtube.com/watch?v=sK5i-N34im8](https://www.youtube.com/watch?v=sK5i-N34im8)

