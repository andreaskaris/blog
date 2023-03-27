# Using cgroups for CFS bandwidth control (CPU quotas)

CFS bandwidth control is a feature that allows you to limit the amount of CPU time that a control group (cgroup) can use.
You can specify the amount of CPU time that a group is allowed to use by setting a quota and a period.
During each period (which is measured in microseconds), a task group can use up to the quota amount of CPU time.
When all the quota has been used up, the threads in the cgroup will be throttled until the next period. 

For more details, have a look at [https://www.kernel.org/doc/html/latest/scheduler/sched-bwc.html](https://www.kernel.org/doc/html/latest/scheduler/sched-bwc.html).

In this article, we will set up a `test` cgroup manually both on RHEL 8 (cgroupsv1) and on RHEL 9 (cgroupsv2). We will 
limit the control group to a subset of CPUs and we will use bandwidth control to limit the total CPU time that our cgroup
is allowed to run per period. We will then attach a process to the `test` cgroup which will generate load, both
single-threaded and multi-threaded.

## Limiting CPU sets and setting CPU quota with cgroups

Let's start by configuring CPU sets and CPU quota for a manually created control group. We will not rely on systemd
or any other tools. Instead, we will directly manipulate cgroups within the mounted file system.
We will look at cgroupsv1 (the default in RHEL 8) and cgroupsv2 (the default in RHEL 9) in isolation for each task.

### Determine the cgroup version

You can use the commenct `mount | grep cgroup` to determine the cgroup version in use.

#### RHEL 8 - cgroupsv1

In RHEL 8, the output of the `mount` command used to display the currently mounted cgroups will reveal multiple mount points:

~~~
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,seclabel,mode=755)
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,xattr,release_agent=/usr/lib/systemd/systemd-cgroups-agent,name=systemd)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,perf_event)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,net_cls,net_prio)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,hugetlb)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpuset)
cgroup on /sys/fs/cgroup/rdma type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,rdma)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,pids)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,blkio)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,cpu,cpuacct)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,freezer)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,devices)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,seclabel,memory)
~~~

#### RHEL 9 - cgroupsv2

In RHEL 9, the output of mounting cgroups will show a single mount point for cgroup2.

~~~
[root@rhel9 ~]# mount | grep cgroup
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,seclabel,nsdelegate,memory_recursiveprot)
~~~

### Enable / Control cgroup controllers

Enable the cpuset and cpu cgroup controller. This is only needed in RHEL 9. In RHEL 8, the respective cgroups hierarchies
are already enabled via the `cpu,cpuacct` and `cpuset` mounts.

#### RHEL 9 - cgroupsv2

Enable the cpuset cgroup controller for the immediate childgroups of `/sys/fs/cgroup`:

~~~
[root@rhel9 ~]# cd /sys/fs/cgroup
[root@rhel9 cgroup]# cat cgroup.subtree_control
cpu io memory pids
[root@rhel9 cgroup]# echo "+cpuset" > cgroup.subtree_control
[root@rhel9 cgroup]# cat cgroup.subtree_control
cpuset cpu io memory pids
~~~

### Create a child cgroup and assign a process to it

Create a child group called `test` and attach the current bash session to it. List processes under the current cgroup both
with cat and with `systemd-cgls`.

#### RHEL 8 - cgroupsv1

Both the cpuset cgroup subsystem, as well as the cpu,cpuacct subsystem are already mounted by default in RHEL 8. We
will use those mountpoints to configure a cgroup for CPU as well as one for CPU set.

~~~
[root@rhel87 ~]# cd /sys/fs/cgroup
[root@rhel87 cgroup]# readlink -f cpu
/sys/fs/cgroup/cpu,cpuacct
[root@rhel87 cgroup]# mkdir cpu/test
[root@rhel87 cgroup]# mkdir cpuset/test
[root@rhel87 cgroup]# ls cpu/test/
cgroup.clone_children  cpuacct.usage_all          cpuacct.usage_sys   cpu.rt_period_us   notify_on_release
cgroup.procs           cpuacct.usage_percpu       cpuacct.usage_user  cpu.rt_runtime_us  tasks
cpuacct.stat           cpuacct.usage_percpu_sys   cpu.cfs_period_us   cpu.shares
cpuacct.usage          cpuacct.usage_percpu_user  cpu.cfs_quota_us    cpu.stat
[root@rhel87 cgroup]# ls cpuset/test
cgroup.clone_children  cpuset.effective_mems   cpuset.memory_spread_page        notify_on_release
cgroup.procs           cpuset.mem_exclusive    cpuset.memory_spread_slab        tasks
cpuset.cpu_exclusive   cpuset.mem_hardwall     cpuset.mems
cpuset.cpus            cpuset.memory_migrate   cpuset.sched_load_balance
cpuset.effective_cpus  cpuset.memory_pressure  cpuset.sched_relax_domain_level
[root@rhel87 cgroup]# cat cpuset/cpuset.mems > cpuset/test/cpuset.mems
[root@rhel87 cgroup]# cat cpuset/cpuset.cpus > cpuset/test/cpuset.cpus
[root@rhel87 cgroup]# echo $$ > cpuset/test/cgroup.procs 
~~~
> NOTE: Both cpuset.mems and cpuset.cpus must be set before assigning a process to `cgroup.procs`. For further details,
see [https://access.redhat.com/solutions/232153](https://access.redhat.com/solutions/232153).

~~~
[root@rhel87 cgroup]# cat cpu/test/cgroup.procs 
5682
5741
[root@rhel87 cgroup]# cat cpuset/test/cgroup.procs 
5682
5742
~~~

~~~
[root@rhel87 cgroup]# systemd-cgls | cat
Working directory /sys/fs/cgroup:
├─cpu,cpuacct
│ ├─user.slice
│ │ ├─5667 sshd: root [priv]
│ │ ├─5672 /usr/lib/systemd/systemd --user
│ │ ├─5675 (sd-pam)
│ │ └─5681 sshd: root@pts/0
│ ├─init.scope
│ │ └─1 /usr/lib/systemd/systemd --switched-root --system --deserialize 17
│ ├─system.slice
(...)
│ └─test
│   ├─5682 -bash
│   ├─5758 systemd-cgls
│   └─5759 cat
(...)
├─cpuset
(...)
│ └─test
│   ├─5682 -bash
│   ├─5758 systemd-cgls
│   └─5759 cat
(...)
~~~

#### RHEL 9 - cgroupsv2

In RHEL 9, we only have to create our cgroup and then attach the process to it.

~~~
[root@rhel9 cgroup]# mkdir test
[root@rhel9 cgroup]# ls test
cgroup.controllers      cgroup.type            cpuset.cpus.partition  memory.events.local  memory.swap.events
cgroup.events           cpu.idle               cpuset.mems            memory.high          memory.swap.high
cgroup.freeze           cpu.max                cpuset.mems.effective  memory.low           memory.swap.max
cgroup.kill             cpu.max.burst          io.bfq.weight          memory.max           pids.current
cgroup.max.depth        cpu.pressure           io.latency             memory.min           pids.events
cgroup.max.descendants  cpu.stat               io.max                 memory.numa_stat     pids.max
cgroup.procs            cpu.weight             io.pressure            memory.oom.group
cgroup.stat             cpu.weight.nice        io.stat                memory.pressure
cgroup.subtree_control  cpuset.cpus            memory.current         memory.stat
cgroup.threads          cpuset.cpus.effective  memory.events          memory.swap.current
~~~

~~~
[root@rhel9 cgroup]# cat test/cgroup.procs 
[root@rhel9 cgroup]# echo $$ > test/cgroup.procs
[root@rhel9 cgroup]# cat test/cgroup.procs 
1433
1484
~~~

~~~
[root@rhel9 cgroup]#  systemd-cgls | tail
│ │ └─getty@tty1.service (#3136)
│ │   → trusted.invocation_id: 08fe123af73c43a6a0cac73661212aa8
│ │   └─736 /sbin/agetty -o -p -- \u --noclear - linux
│ └─systemd-logind.service (#2725)
│   → trusted.invocation_id: 20aa43cd55b448ab9d9ba2d5660ad802
│   └─698 /usr/lib/systemd/systemd-logind
└─test (#4549)
  ├─1433 -bash
  ├─1486 systemd-cgls
  └─1487 tail
~~~

### Pin the cgroup to specific CPUs

We can restrict the cgroup to run on CPUs 2 and 3 only. We can then generate load with 2 processes to verify that the
cgroup is really restricted to these CPUs. Finally, use `taskset` to check the CPU set and use `mpstat` to check CPU usage.
Also read the cgroup's CPU stats.

#### RHEL 8 - cgroupsv1

~~~
[root@rhel87 cgroup]# echo "2-3" > cpuset/test/cpuset.cpus
[root@rhel87 cgroup]# 
~~~

~~~
[root@rhel87 cgroup]#  for i in 1 2 ; do bash -c "while true; do let i++; done" & done
[1] 5764
[2] 5765
~~~

In a different terminal:

~~~
[root@rhel87 ~]# taskset -c -p 5764
pid 5764's current affinity list: 2,3
[root@rhel87 ~]# mpstat  -P ALL 1 1 
Linux 4.18.0-425.13.1.el8_7.x86_64 (rhel87) 	03/24/2023 	_x86_64_	(4 CPU)

06:53:40 AM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
06:53:41 AM  all   50.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   50.00
06:53:41 AM    0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
06:53:41 AM    1    0.99    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.01
06:53:41 AM    2  100.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00
06:53:41 AM    3  100.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all   50.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   50.00
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.99    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.01
Average:       2  100.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00
Average:       3  100.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00
~~~

#### RHEL 9 - cgroupsv2

~~~
[root@rhel9 cgroup]# echo "2-3" > test/cpuset.cpus
[root@rhel9 cgroup]# 
~~~

~~~
[root@rhel9 cgroup]# for i in 1 2 ; do bash -c "while true; do let i++; done" & done
[1] 1489
[2] 1490
~~~

In a different terminal:

~~~
[root@rhel9 ~]# taskset -c -p 1489
pid 1489's current affinity list: 2,3
[root@rhel9 ~]# mpstat  -P ALL 1 1
Linux 5.14.0-162.18.1.el9_1.x86_64 (rhel9) 	03/23/23 	_x86_64_	(4 CPU)

12:40:36     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
12:40:37     all   41.15    0.00    8.73    0.00    0.25    0.00    0.00    0.00    0.00   49.88
12:40:37       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
12:40:37       1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
12:40:37       2   83.17    0.00   15.84    0.00    0.99    0.00    0.00    0.00    0.00    0.00
12:40:37       3   81.00    0.00   19.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all   41.15    0.00    8.73    0.00    0.25    0.00    0.00    0.00    0.00   49.88
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       2   83.17    0.00   15.84    0.00    0.99    0.00    0.00    0.00    0.00    0.00
Average:       3   81.00    0.00   19.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00
~~~

### Control CPU quotas

By default, CFS bandwidth control will monitor CPU bandwidth over a `period` of 100000 microseconds (0.1 seconds). The
permitted `quota` is unlimited (`-1` for cgroupsv1, `max` for cgroupsv2).

We will instruct CFS to use a period of 1 second (the maximum)
and to allow our cgroup to run for a maximum quota of 0.1 seconds. Hence, in total, we should see 10% of CPU utilization
spread over the available cores. You will have to add up `%usr` and `%sys`, given that the kernel must spend some time in
kernel space switching between our 2 processes.
Our quota can also exceed an allocation of 100%. For example by allocating `quota = 2x period`, 2 full cores worth of
CPU time will be given to the cgroup.

#### RHEL 8 - cgroupsv1

For RHEL 8, the quota is set with file `cpu.cfs_quota_us`. The period is set with file `cpu.cfs_period_us`.

~~~
[root@rhel87 cgroup]# cat cpu/test/cpu.cfs_period_us 
100000
[root@rhel87 cgroup]# cat cpu/test/cpu.cfs_quota_us 
-1
~~~

~~~
[root@rhel87 cgroup]# echo 1000000 > cpu/test/cpu.cfs_period_us
[root@rhel87 cgroup]# echo 100000 > cpu/test/cpu.cfs_quota_us 
[root@rhel87 cgroup]# 
~~~

~~~
[root@rhel87 cgroup]# mpstat  -P ALL 1 10 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    2.63    0.00    0.08    0.00    0.10    0.00    0.00    0.00    0.00   97.20
Average:       0    0.10    0.00    0.10    0.00    0.10    0.00    0.00    0.00    0.00   99.70
Average:       1    0.00    0.00    0.10    0.00    0.00    0.00    0.00    0.00    0.00   99.90
Average:       2    5.31    0.00    0.10    0.00    0.20    0.00    0.00    0.00    0.00   94.39
Average:       3    5.09    0.00    0.00    0.00    0.10    0.00    0.00    0.00    0.00   94.81
~~~

~~~
[root@rhel87 cgroup]# echo 2000000 > cpu/test/cpu.cfs_quota_us
[root@rhel87 cgroup]# 
~~~

~~~
[root@rhel87 cgroup]# mpstat  -P ALL 1 10 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all   49.86    0.00    0.03    0.00    0.18    0.00    0.03    0.00    0.00   49.91
Average:       0    0.10    0.00    0.10    0.00    0.00    0.00    0.00    0.00    0.00   99.80
Average:       1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       2   99.70    0.00    0.00    0.00    0.30    0.00    0.00    0.00    0.00    0.00
Average:       3   99.50    0.00    0.00    0.00    0.40    0.00    0.10    0.00    0.00    0.00
~~~

#### RHEL 9 - cgroupsv2

For RHEL 9, the quota and the period are set with file `cpu.max` which holds `cfs_quota_us` on the left-hand side and
`cfs_period_us` on the right-hand side.

~~~
[root@rhel9 cgroup]# echo "100000 1000000" > test/cpu.max
[root@rhel9 cgroup]# cat test/cpu.max
100000 1000000
~~~

~~~
[root@rhel9 ~]# mpstat  -P ALL 1 10 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    2.08    0.00    0.48    0.00    0.03    0.00    0.00    0.00    0.00   97.42
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.00    0.00    0.10    0.00    0.00    0.00    0.00    0.00    0.00   99.90
Average:       2    4.21    0.00    0.90    0.00    0.00    0.00    0.00    0.00    0.00   94.89
Average:       3    4.10    0.00    0.90    0.00    0.10    0.00    0.00    0.00    0.00   94.90
~~~

~~~
[root@rhel9 cgroup]# echo "2000000 1000000" > test/cpu.max
[root@rhel9 cgroup]# cat test/cpu.max
2000000 1000000
~~~

~~~
[root@rhel9 ~]# mpstat  -P ALL 1 1 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all   41.25    0.00    8.25    0.00    0.25    0.00    0.25    0.00    0.00   50.00
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       2   82.00    0.00   17.00    0.00    1.00    0.00    0.00    0.00    0.00    0.00
Average:       3   83.00    0.00   16.00    0.00    0.00    0.00    1.00    0.00    0.00    0.00
~~~

### Inspect cgroup CPU stats

The cgroup CPU stats allow us to determine - among other things - how many periods elapsed, how many of these periods
were throttled, and the total time for which entities of the cgroup have been throttled.

#### RHEL 8 - cgroupsv1

cgroupsv1 has a limited set of statistics:

~~~
[root@rhel87 cgroup]# cat cpu/test/cpu.stat
nr_periods 166
nr_throttled 108
throttled_time 186818842869
~~~

#### RHEL 9 - cgroupsv2

cgroupsv2 offers us more counters than cgroupsv1:

~~~
[root@rhel9 cgroup]# cat test/cpu.stat 
usage_usec 1702534895
user_usec 1411862526
system_usec 290672369
nr_periods 379
nr_throttled 338
throttled_usec 570835096
nr_bursts 1
burst_usec 831683
~~~

### Removing the cgroup

In order to remove the cgroup, first remove all attached processes. You can do so by attaching the process to the root
`/sys/fs/cgroup`. Then, delete the cgroup's directory.

#### RHEL 8 - cgroupsv1

~~~
[root@rhel87 cgroup]# echo $$ > cpu/cgroup.procs
[root@rhel87 cgroup]# echo $$ > cpuset/cgroup.procs
[root@rhel87 cgroup]# rmdir cpu/test
[root@rhel87 cgroup]# rmdir cpuset/test
[root@rhel87 cgroup]# 
~~~

#### RHEL 9 - cgroupsv2

~~~
[root@rhel9 cgroup]# cat test/cgroup.procs
1433
2047
[root@rhel9 cgroup]# echo 1433 > cgroup.procs 
[root@rhel9 cgroup]# cat test/cgroup.procs
[root@rhel9 cgroup]# rmdir test
[root@rhel9 cgroup]# 
~~~

### Repeating the tests with a single process on a single CPU

For the sake of clarity and simplicity, we will repeat the sequence of steps for a single thread pinned to a single
CPU. We will create the cgroup, pin it to CPU 3, limit its quota to 0.1ms with a period of 1 second for a CPU share of
10%. We then attach the current bash session to the cgroup and run a CPU consuming process in the foreground.

In a different terminal, we then check CPU utilization with `mpstat` and CPU statistics with `cat /sys/fs/cgroup/test/cpu.stat`.

#### RHEL 8 - cgroupsv1

~~~
[root@rhel87 cgroup]# mkdir cpuset/test; echo 3 > cpuset/test/cpuset.cpus; cat cpuset/cpuset.mems > cpuset/test/cpuset.mems; echo $$ > cpuset/test/cgroup.procs; mkdir cpu/test; echo 1000000 > cpu/test/cpu.cfs_period_us; echo 100000 > cpu/test/cpu.cfs_quota_us; echo $$ > cpu/test/cgroup.procs;  bash -c "while true; do let i++; done"
~~~

~~~
[root@rhel87 cgroup]# mpstat  -P ALL 1 1 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    2.50    0.00    0.00    0.00    0.25    0.00    0.00    0.00    0.00   97.25
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.00    0.00    0.00    0.00    1.00    0.00    0.00    0.00    0.00   99.00
Average:       2    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       3   10.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   90.00
~~~

~~~
[root@rhel87 cgroup]# for i in {1..4}; do date; cat /sys/fs/cgroup/cpu/test/cpu.stat ; sleep 1; done
Fri Mar 24 07:18:57 EDT 2023
nr_periods 348
nr_throttled 348
throttled_time 312188691325
Fri Mar 24 07:18:58 EDT 2023
nr_periods 349
nr_throttled 349
throttled_time 313088840213
Fri Mar 24 07:18:59 EDT 2023
nr_periods 350
nr_throttled 350
throttled_time 313986999920
Fri Mar 24 07:19:00 EDT 2023
nr_periods 351
nr_throttled 351
throttled_time 314887131053
~~~

#### RHEL 9 - cgroupsv2

~~~
[root@rhel9 cgroup]# mkdir test; echo 3 > test/cpuset.cpus; echo "100000 1000000" > test/cpu.max; echo $$ > test/cgroup.procs;  bash -c "while true; do let i++; done"
~~~

~~~
[root@rhel9 ~]# mpstat  -P ALL 1 1 | tail -n 7

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    2.50    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   97.50
Average:       0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       2    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
Average:       3   10.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   90.00
~~~

~~~
[root@rhel9 ~]# for i in {1..4}; do date; cat /sys/fs/cgroup/test/cpu.stat ; sleep 1; done
Thu Mar 23 13:10:57 EDT 2023
usage_usec 6201028
user_usec 6198154
system_usec 2873
nr_periods 61
nr_throttled 61
throttled_usec 54732627
nr_bursts 0
burst_usec 0
Thu Mar 23 13:10:58 EDT 2023
usage_usec 6300822
user_usec 6297949
system_usec 2873
nr_periods 62
nr_throttled 62
throttled_usec 55631975
nr_bursts 0
burst_usec 0
Thu Mar 23 13:10:59 EDT 2023
usage_usec 6400595
user_usec 6397722
system_usec 2873
nr_periods 63
nr_throttled 63
throttled_usec 56531360
nr_bursts 0
burst_usec 0
Thu Mar 23 13:11:00 EDT 2023
usage_usec 6500389        # <--- how many usecs was the cgroup active
user_usec 6497515
system_usec 2873
nr_periods 64             # <--- total periods elapsed
nr_throttled 64           # <--- how many of the elapsed periods was the cgroup throttled for
throttled_usec 57430724   # <--- how many usecs was the cgroup throttled for
nr_bursts 0
burst_usec 0
~~~

We can see that the cgroup was throttled for roughly 10% of the time, as expected:

~~~
usage_usec / (usage_usec+throttled_usec) = 6500389÷(57430724+6500389) = 0.10167802
~~~

## Using perf to analyze throttling

Unfortunately, due to time constraints, I won't be covering how to use perf to analyze throttling in this post.
However, I will talk about this in a follow-up post, where I'll dive deeper into this topic and explore how perf can be
used to identify and resolve throttling issues.

## Sources

* [https://lore.kernel.org/lkml/20220328132047.GD8939@worktop.programming.kicks-ass.net/T/?utm_source=pocket_saves#ma3e659f06e5b51df7e33107b66d57c843d3b6105](https://lore.kernel.org/lkml/20220328132047.GD8939@worktop.programming.kicks-ass.net/T/?utm_source=pocket_saves#ma3e659f06e5b51df7e33107b66d57c843d3b6105)
* [https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/managing_monitoring_and_updating_the_kernel/assembly_using-cgroupfs-to-manually-manage-cgroups_managing-monitoring-and-updating-the-kernel](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/managing_monitoring_and_updating_the_kernel/assembly_using-cgroupfs-to-manually-manage-cgroups_managing-monitoring-and-updating-the-kernel)
* [https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt](https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt)
* [https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt](https://www.kernel.org/doc/Documentation/cgroup-v1/cgroups.txt)
