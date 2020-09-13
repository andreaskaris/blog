#!/bin/bash

for netns in $(ip netns | awk '{print $1}'); do 
  echo " =========== $netns =============="

  netns_inode=$(stat /var/run/netns/$netns -c %i) 
  netns_pids=$(lsns -o NS,PID | grep $netns_inode | awk '{print $NF}' | tr '\n' ' ' | sed 's/ $//') 

  echo -n -e "NETNS: $netns (/var/run/netns/$netns)\n\tpoints to\tNETNS INODE: $netns_inode\n\tused by\t\tPIDs: $netns_pids\n\n"

  if  [ $(stat -f -c %T /var/run/netns/$netns) == 'tmpfs' ] ; then
    echo -e "WARNING: /var/run/netns/$netns is not mounted and is a tmpfs\n"
    continue
  fi

  echo "Main process and children:"
  for netns_pid in $netns_pids ; do 
      echo "${netns_pid}:"
      ps -T -f -p $netns_pid
  done

  echo ""

  more_netns_pids=$(ls -al /proc/*/ns/net 2>/dev/null | grep $netns_inode | awk -F '/' '{print $3}' | tr '\n' ' ')
  echo "All processes and children:"
  for more_netns_pid in $more_netns_pids ; do 
      echo "${more_netns_pid}:"
      ps -T -f -p $more_netns_pid
  done

  echo ""
  echo ""
done

