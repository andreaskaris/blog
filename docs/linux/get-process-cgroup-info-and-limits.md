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
