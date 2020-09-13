## Prerequisites ##

### Creating additional OpenStack infrastructure ###

Add an additional port to each of the 3 workers:
~~~
source overcloudrc  # or respective RC file
openstack network create akaris-backend
openstack subnet create --network akaris-backend akaris-backend-subnet --subnet-range 172.31.254.0/24 --no-dhcp
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-0
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-1
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-2
openstack server add port akaris-osc-r4gsx-worker-6x48k akaris-backend-port-0
openstack server add port akaris-osc-r4gsx-worker-9nk25 akaris-backend-port-1
openstack server add port akaris-osc-r4gsx-worker-qz5g6 akaris-backend-port-2
~~~

### Making sure that additional ports work in the workers ###

Connect to the master node:
~~~
[akaris@linux clouds]$ ssh core@<API URL> -A
~~~

From there, connect to each worker and configure the new interface (in this case, ens6):
~~~
ssh core@akaris-osc-r4gsx-worker-9nk25
~~~

Enable ports with nmcli:
~~~
nmcli connection # this shows a 'Wired conection 2' that is connected to ens6

# I prefer a different name, so delete this one:
nmcli connection delete 'Wired connection 2'

# add a new connection
nmcli connection add type ethernet ifname ens6 con-name ens6

# assign a static IP
nmcli connection modify ens6 ipv4.addresses 172.31.254.12/24 
nmcli connection modify ens6 ipv4.method static
nmcli connection up ens6
~~~

Once you repeated these steps on all workers, ping between the 3 workers to make sure that network and ports work:
~~~
ip a ls dev ens6
ping 172.31.254.11
ping 172.31.254.10
~~~

## Adding jump server as DHCP server ##

When using a jump server to install OCP as described in [https://github.com/andreaskaris/blog/blob/master/installing_openshift_4.3_on_osp.md](https://github.com/andreaskaris/blog/blob/master/installing_openshift_4.3_on_osp.md), one can reuse the jump server as a DHCP server for multus networks. 

~~~
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-dhcp
openstack server add port akaris_jump_server akaris-backend-port-dhcp
~~~

Then, configure the new port within the jump server and install and instruct dnsmasq as the DHCP server:
~~~
yum install dnsmasq -y
~~~

~~~
[root@akaris-jump-server ~]# egrep -v '^#|^$' /etc/dnsmasq.conf 
interface=eth1
listen-address=172.31.254.1
dhcp-range=172.31.254.50,172.31.254.150,12h
dhcp-leasefile=/var/lib/dnsmasq/dnsmasq.leases
conf-dir=/etc/dnsmasq.d,.rpmnew,.rpmsave,.rpmorig
[root@akaris-jump-server ~]# cat /etc/sysconfig/network-scripts/ifcfg-eth1
# Created by cloud-init on instance boot automatically, do not edit.
#
BOOTPROTO=static
DEVICE=eth1
HWADDR=fa:16:3e:9d:28:1d
MTU=1500
ONBOOT=yes
TYPE=Ethernet
USERCTL=no
IPADDR=172.31.254.1
PREFIX=24
~~~

~~~
systemctl enable --now dnsmasq
~~~

## Adding ports to master nodes ##

The dhcp agents run on all master nodes and all worker nodes:
~~~

~~~

Therefore, the master nodes need to be connected to the same network interfaces as the workers. Otherwise, DHCP requests might not be sent to the DHCP server:
~~~
root@akaris-jump-server ~]# tcpdump -nne -i eth1 port 67 or port 68 &
[1] 22543
[root@akaris-jump-server ~]# tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth1, link-type EN10MB (Ethernet), capture size 262144 bytes

[root@akaris-jump-server ~]# 
[root@akaris-jump-server ~]# journalctl -f -u dnsmasq &
[2] 22544
[root@akaris-jump-server ~]# -- Logs begin at Fri 2020-01-24 12:27:41 EST. --
Jan 27 14:36:55 akaris-jump-server systemd[1]: Stopping DNS caching server....
Jan 27 14:36:55 akaris-jump-server systemd[1]: Stopped DNS caching server..
Jan 27 14:36:55 akaris-jump-server systemd[1]: Started DNS caching server..
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: started, version 2.76 cachesize 150
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: compile time options: IPv6 GNU-getopt DBus no-i18n IDN DHCP DHCPv6 no-Lua TFTP no-conntrack ipset auth no-DNSSEC loop-detect inotify
Jan 27 14:36:55 akaris-jump-server dnsmasq-dhcp[22088]: DHCP, IP range 172.31.254.50 -- 172.31.254.150, lease time 12h
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: reading /etc/resolv.conf
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: using nameserver 10.11.142.1#53
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: using nameserver 10.11.142.2#53
Jan 27 14:36:55 akaris-jump-server dnsmasq[22088]: read /etc/hosts - 7 addresses

[root@akaris-jump-server ~]# 
[root@akaris-jump-server ~]# 
[root@akaris-jump-server ~]# 
~~~

Adding ports:
~~~
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-master-0
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-master-1
openstack port create --disable-port-security --no-security-group --no-fixed-ip --network akaris-backend akaris-backend-port-master-2
openstack server add port akaris-osc-qhg2f-master-0 akaris-backend-port-master-0
openstack server add port akaris-osc-qhg2f-master-1 akaris-backend-port-master-1
openstack server add port akaris-osc-qhg2f-master-2 akaris-backend-port-master-2
~~~

Configurating interface on master nodes:
~~~
nmcli connection delete 'Wired connection 2'
nmcli connection add type ethernet ifname ens6 con-name ens6
nmcli connection modify ens6 ipv4.addresses 172.31.254.22/24
nmcli connection modify ens6 ipv4.method static
nmcli connection up ens6
~~~

...

## Testing multus ##

* [https://docs.openshift.com/container-platform/4.2/networking/multiple-networks/configuring-ipvlan.html](https://docs.openshift.com/container-platform/4.2/networking/multiple-networks/configuring-ipvlan.html)
* [https://docs.openshift.com/container-platform/4.2/networking/multiple-networks/attaching-pod.html](https://docs.openshift.com/container-platform/4.2/networking/multiple-networks/attaching-pod.html)
