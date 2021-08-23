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
