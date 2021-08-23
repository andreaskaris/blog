# Install Open vSwitch on Rocky Linux 8

Required RPMs can be found at [http://ftp.redhat.com/pub/redhat/linux/enterprise/8Base/en/Fast-Datapath/SRPMS/]( http://ftp.redhat.com/pub/redhat/linux/enterprise/8Base/en/Fast-Datapath/SRPMS/)

~~~
yum install '@Development Tools'
yum install desktop-file-utils libcap-ng-devel libmnl-devel numactl-devel openssl-devel python3-devel python3-pyOpenSSL python3-setuptools python3-sphinx rdma-core-devel unbound-devel -y
rpmbuild --rebuild  http://ftp.redhat.com/pub/redhat/linux/enterprise/8Base/en/Fast-Datapath/SRPMS/openvswitch2.13-2.13.0-79.el8fdp.src.rpm
yum install selinux-policy-devel -y
rpmbuild --rebuild http://ftp.redhat.com/pub/redhat/linux/enterprise/8Base/en/Fast-Datapath/SRPMS/openvswitch-selinux-extra-policy-1.0-28.el8fdp.src.rpm
yum localinstall /root/rpmbuild/RPMS/noarch/openvswitch-selinux-extra-policy-1.0-28.el8.noarch.rpm /root/rpmbuild/RPMS/x86_64/openvswitch2.13-2.13.0-79.el8.x86_64.rpm -y
systemctl enable --now openvswitch
~~~

Then, set up Open vSwitch with NetworkManager:
~~~
yum install NetworkManager-ovs.x86_64 -y

BRIDGE_NAME=br-ex
INTERFACE_NAME=enp7s0
IP_ADDRESS=192.168.123.3/24

nmcli c add type ovs-bridge conn.interface ${BRIDGE_NAME} con-name ${BRIDGE_NAME}
nmcli c add type ovs-port conn.interface ${BRIDGE_NAME} master ${BRIDGE_NAME} con-name ovs-port-${BRIDGE_NAME}
nmcli c add type ovs-interface slave-type ovs-port conn.interface ${BRIDGE_NAME} master ovs-port-${BRIDGE_NAME}  con-name ovs-if-${BRIDGE_NAME}
nmcli c add type ovs-port conn.interface ${INTERFACE_NAME} master ${BRIDGE_NAME} con-name ovs-port-${INTERFACE_NAME}
nmcli c add type ethernet conn.interface ${INTERFACE_NAME} master ovs-port-${INTERFACE_NAME} con-name ovs-if-${INTERFACE_NAME}
nmcli conn delete ${INTERFACE_NAME}
nmcli conn mod ${BRIDGE_NAME} connection.autoconnect yes
nmcli conn mod ovs-if-${BRIDGE_NAME} connection.autoconnect yes
nmcli conn mod ovs-if-${INTERFACE_NAME} connection.autoconnect yes
nmcli conn mod ovs-port-${INTERFACE_NAME} connection.autoconnect yes
nmcli conn mod ovs-port-${BRIDGE_NAME} connection.autoconnect yes
nmcli conn m ovs-if-${BRIDGE_NAME} ipv4.address ${IP_ADDRESS}
nmcli conn m ovs-if-${BRIDGE_NAME} ipv4.method static

systemctl restart NetworkManager
~~~

### Sources

Partially adapted from:
* [https://blog.oddbit.com/post/2020-02-15-configuring-open-vswitch-with/](https://blog.oddbit.com/post/2020-02-15-configuring-open-vswitch-with/)
