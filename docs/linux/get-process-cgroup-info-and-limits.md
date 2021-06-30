## How to get a processes' cgroup resource usage and limit info

### Memory

In order to retrieve a processes' cgroup memory usage and its limits, determine the processes' PID and then run something like the following:
~~~
for pid in 16595; do 
  echo === $pid ===
  cgroup=$(cat /proc/$pid/cgroup | awk -F ':' '/memory/ {print $NF}')
  scope=$(cat /proc/$pid/cgroup | awk -F ':' '/memory/ {print $NF}' | awk -F '/' '{print $NF}')
  grep '' /sys/fs/cgroup/memory/$cgroup/*
  systemctl status $scope | cat
done
~~~

Example output:
~~~
[root@openshift-worker-1 ~]# for pid in 16595; do 
>   echo === $pid ===
>   cgroup=$(cat /proc/$pid/cgroup | awk -F ':' '/memory/ {print $NF}')
>   scope=$(cat /proc/$pid/cgroup | awk -F ':' '/memory/ {print $NF}' | awk -F '/' '{print $NF}')
>   grep '' /sys/fs/cgroup/memory/$cgroup/*
>   systemctl status $scope | cat
> done
=== 16595 ===
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.clone_children:0
grep: /sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.event_control: Invalid argument
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:15905
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16585
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16586
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16587
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16588
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16589
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16590
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16591
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16592
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16593
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16594
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16595
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16596
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16597
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16598
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16599
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16600
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16601
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16602
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16603
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16604
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16605
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16606
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16607
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16608
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16609
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16610
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16611
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/cgroup.procs:16612
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.failcnt:0
grep: /sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.force_empty: Invalid argument
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.failcnt:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.limit_in_bytes:9223372036854771712
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.max_usage_in_bytes:5132288
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:slabinfo - version: 2.1
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-16           256    256     16  256    1 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:TCPv6                  0      0   2624   12    8 : tunables    0    0    0 : slabdata      0      0      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:TCP                    0      0   2432   13    8 : tunables    0    0    0 : slabdata      0      0      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:pde_opener           102    102     40  102    1 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:radix_tree_node      112    112    584   28    4 : tunables    0    0    0 : slabdata      4      4      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-8              0      0      8  512    1 : tunables    0    0    0 : slabdata      0      0      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:shmem_inode_cache     43     43    760   43    8 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:ovl_inode            132    132    736   44    8 : tunables    0    0    0 : slabdata      3      3      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:xfs_inode            120    120   1088   30    8 : tunables    0    0    0 : slabdata      4      4      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-64            64     64     64   64    1 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-32           128    128     32  128    1 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:eventpoll_pwq        168    168     72   56    1 : tunables    0    0    0 : slabdata      3      3      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-192           42     42    192   42    2 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-1k            32     32   1024   32    8 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:inode_cache           49     49    656   49    8 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-4k             8      8   4096    8    8 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:kmalloc-512          384    384    512   32    4 : tunables    0    0    0 : slabdata     12     12      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:skbuff_head_cache    384    384    256   32    2 : tunables    0    0    0 : slabdata     12     12      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:pid                  192    192    128   32    1 : tunables    0    0    0 : slabdata      6      6      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:signal_cache          52     52   1216   26    8 : tunables    0    0    0 : slabdata      2      2      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:sighand_cache         30     30   2112   15    8 : tunables    0    0    0 : slabdata      2      2      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:files_cache           46     46    704   46    8 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:task_delay_info       51     51     80   51    1 : tunables    0    0    0 : slabdata      1      1      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:task_struct           44     44   7872    4    8 : tunables    0    0    0 : slabdata     11     11      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:PING                  84     84   1152   28    8 : tunables    0    0    0 : slabdata      3      3      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:sock_inode_cache      92     92    704   46    8 : tunables    0    0    0 : slabdata      2      2      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:proc_inode_cache      90     90    728   45    8 : tunables    0    0    0 : slabdata      2      2      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:dentry               798    798    192   42    2 : tunables    0    0    0 : slabdata     19     19      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:anon_vma_chain      1920   1920     64   64    1 : tunables    0    0    0 : slabdata     30     30      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:filp                 454    608    256   32    2 : tunables    0    0    0 : slabdata     19     19      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:anon_vma            1012   1012     88   46    1 : tunables    0    0    0 : slabdata     22     22      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:vm_area_struct      2205   2205    232   35    2 : tunables    0    0    0 : slabdata     63     63      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:mm_struct             56     56   1152   28    8 : tunables    0    0    0 : slabdata      2      2      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.slabinfo:cred_jar             294    294    192   42    2 : tunables    0    0    0 : slabdata      7      7      0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.tcp.failcnt:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.tcp.limit_in_bytes:9223372036854771712
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.tcp.max_usage_in_bytes:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.tcp.usage_in_bytes:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.kmem.usage_in_bytes:5091328
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.limit_in_bytes:1073741824
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.max_usage_in_bytes:34414592
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.memsw.failcnt:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.memsw.limit_in_bytes:1073741824
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.memsw.max_usage_in_bytes:34414592
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.memsw.usage_in_bytes:34373632
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.move_charge_at_immigrate:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:total=7062 N0=0 N1=7115
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:file=2805 N0=0 N1=2841
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:anon=4257 N0=0 N1=4274
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:unevictable=0 N0=0 N1=0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:hierarchical_total=7062 N0=0 N1=7115
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:hierarchical_file=2805 N0=0 N1=2841
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:hierarchical_anon=4257 N0=0 N1=4274
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.numa_stat:hierarchical_unevictable=0 N0=0 N1=0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.oom_control:oom_kill_disable 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.oom_control:under_oom 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.oom_control:oom_kill 0
grep: /sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.pressure_level: Invalid argument
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.soft_limit_in_bytes:9223372036854771712
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:cache 11624448
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:rss 17403904
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:rss_huge 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:shmem 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:mapped_file 6352896
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:dirty 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:writeback 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:swap 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:pgpgin 9339
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:pgpgout 2229
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:pgfault 9801
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:pgmajfault 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:inactive_anon 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:active_anon 17436672
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:inactive_file 7704576
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:active_file 3784704
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:unevictable 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:hierarchical_memory_limit 1073741824
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:hierarchical_memsw_limit 1073741824
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_cache 11624448
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_rss 17403904
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_rss_huge 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_shmem 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_mapped_file 6352896
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_dirty 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_writeback 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_swap 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_pgpgin 9339
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_pgpgout 2229
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_pgfault 9801
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_pgmajfault 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_inactive_anon 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_active_anon 17436672
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_inactive_file 7704576
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_active_file 3784704
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.stat:total_unevictable 0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.swappiness:60
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.usage_in_bytes:34373632
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/memory.use_hierarchy:1
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/notify_on_release:0
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:15905
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16585
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16586
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16587
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16588
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16589
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16590
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16591
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16592
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16593
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16594
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16595
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16596
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16597
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16598
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16599
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16600
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16601
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16602
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16603
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16604
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16605
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16606
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16607
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16608
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16609
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16610
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16611
/sys/fs/cgroup/memory//kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope/tasks:16612
Warning: The unit file, source configuration file or drop-ins of crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope changed on disk. Run 'systemctl daemon-reload' to reload units.
● crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope - libcontainer container 35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a
   Loaded: loaded (/run/systemd/transient/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope; transient)
Transient: yes
  Drop-In: /run/systemd/transient/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope.d
           └─50-DevicePolicy.conf, 50-DeviceAllow.conf, 50-MemoryLimit.conf, 50-CPUShares.conf, 50-CPUQuota.conf, 50-TasksAccounting.conf, 50-TasksMax.conf
   Active: active (running) since Wed 2021-06-30 12:34:47 UTC; 4h 3min ago
    Tasks: 29 (limit: 1024)
   Memory: 32.7M (limit: 1.0G)
      CPU: 162ms
   CGroup: /kubepods.slice/kubepods-podf00f1a6b_5fe6_4fca_b005_ba162aa468ef.slice/crio-35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.scope
           ├─15905 nginx: master process nginx -g daemon off;
           ├─16585 nginx: worker process
           ├─16586 nginx: worker process
           ├─16587 nginx: worker process
           ├─16588 nginx: worker process
           ├─16589 nginx: worker process
           ├─16590 nginx: worker process
           ├─16591 nginx: worker process
           ├─16592 nginx: worker process
           ├─16593 nginx: worker process
           ├─16594 nginx: worker process
           ├─16595 nginx: worker process
           ├─16596 nginx: worker process
           ├─16597 nginx: worker process
           ├─16598 nginx: worker process
           ├─16599 nginx: worker process
           ├─16600 nginx: worker process
           ├─16601 nginx: worker process
           ├─16602 nginx: worker process
           ├─16603 nginx: worker process
           ├─16604 nginx: worker process
           ├─16605 nginx: worker process
           ├─16606 nginx: worker process
           ├─16607 nginx: worker process
           ├─16608 nginx: worker process
           ├─16609 nginx: worker process
           ├─16610 nginx: worker process
           ├─16611 nginx: worker process
           └─16612 nginx: worker process

Jun 30 12:34:47 openshift-worker-1 systemd[1]: Started libcontainer container 35c9d39b58f5a7ad7b9294a67cb3c7a9df219df7bf3b712ff43524254af6664a.
~~~
