# Setting up an iPXE boot environment under RHEL

In order to get a working iPXE boot environment, you need to set up dnsmasq/tftp and httpd. dnsmasq is used as the DHCP server, if needed as the DNS server (although disabled in the following example) and as the tftp server. httpd will server the actual image to boot from.

The following steps are a manual walkthrough of the steps taken in [https://github.com/andreaskaris/fedora-ipxe-container](https://github.com/andreaskaris/fedora-ipxe-container).

## Install dependencies

Install required dependencies:
~~~
yum install ipxe-bootimgs dnsmasq httpd -y
~~~

## Setting up dnsmasq and tftp

Create requried directories:
~~~
mkdir /tftpboot
mkdir /dhcphosts
~~~

Create the following configuration for dnsmasq and adjust according to your needs.
~~~
cat <<'EOF' > /etc/dnsmasq.conf
# enable logs if required
#log-queries
#log-dhcp

# disable DNS server
port=0

listen-address=192.168.123.1
interface=baremetal

# enable built-in tftp server
enable-tftp
tftp-root=/tftpboot

# DHCP range 192.168.123.200 ~ 192.168.123.250
dhcp-range=192.168.123.200,192.168.123.250,255.255.255.0,24h

# Default gateway
dhcp-option=3,192.168.123.1

dhcp-option=6,192.168.123.1

# Domain name - homelab.net
dhcp-option=15,example.com

# Broadcast address
dhcp-option=28,192.168.123.255

# Set interface MTU to 9000 bytes (jumbo frame)
# Enable only when your network supports it
# dhcp-option=26,9000
dhcp-option-force=26,9000


# Tag dhcp request from iPXE
dhcp-match=set:ipxe,175

# inspect the vendor class string and tag BIOS client
dhcp-vendorclass=BIOS,PXEClient:Arch:00000

# 1st boot file - Legacy BIOS client
dhcp-boot=tag:!ipxe,tag:BIOS,undionly.kpxe,192.168.123.1

# 1st boot file - EFI client
# at the moment all non-BIOS clients are considered
# EFI client
dhcp-boot=tag:!ipxe,tag:!BIOS,ipxe.efi,192.168.123.1

# 2nd boot file
dhcp-boot=tag:ipxe,menu/boot.ipxe
EOF
~~~

Within the DHCP range, create a range of static leases:
~~~
cat <<'EOF' > /dhcphosts/hosts 
52:54:00:00:00:01,192.168.123.200,node-0
52:54:00:00:00:01,192.168.123.201,node-1
EOF
~~~

Run dnsmasq - obviously, this should be run as a service:
~~~
/usr/sbin/dnsmasq -k --dhcp-hostsdir=/dhcphosts --bind-interfaces --log-facility=/dev/stdout
~~~

Create files `/tftpboot/ipxe.efi`, `/tftpboot/undionly.kpxe`:
~~~
RUN cp /usr/share/ipxe/undionly.kpxe /tftpboot
RUN cp /usr/share/ipxe/ipxe-x86_64.efi /tftpboot/ipxe.efi
~~~

Create the PXE boot menu:
~~~
cat <<'EOF' > /tftpboot/menu/boot.ipxe 
#!ipxe 

menu PXE Boot Options

item fedora fedora
item openshift-master openshift-master
item openshift-bootstrap openshift-bootstrap
item openshift-worker openshift-worker

item shell iPXE shell
item exit  Exit to BIOS

choose --default exit --timeout 10000 option && goto ${option}

:fedora
set server_root http://192.168.123.1:8081/fedora/
initrd ${server_root}/initrd.img
kernel ${server_root}/vmlinuz ip=dhcp ipv6.disable initrd=initrd.img inst.ks=${server_root}/kickstart.cfg
boot

:shell
shell

:exit
exit
~~~

## Configure httpd and images

Prepare httpd:
~~~
systemctl enable --now httpd
~~~

Now, download and add Fedora to be booted from iPXE:
~~~
cat <<'EOF' > example_netinstall.sh
#!/bin/bash

echo "Installing Fedora PXE boot image for debugging"
curl -L -o /tmp/fedora.iso "https://download.fedoraproject.org/pub/fedora/linux/releases/32/Server/x86_64/iso/Fedora-Server-dvd-x86_64-32-1.6.iso"
mkdir /mnt/fedora
mkdir /var/www/html/fedora/dvd -p
mount -o loop /tmp/fedora.iso /mnt/fedora
\cp -a /mnt/fedora/* /var/www/html/fedora/dvd/.
\cp /var/www/html/fedora/dvd/images/pxeboot/* /var/www/html/fedora/.
umount /mnt/fedora
source config 
cat << EOF > /var/www/html/fedora/kickstart.cfg
url --url http://${PXE_LISTEN_ADDRESS}:${PXE_LISTEN_PORT}/fedora/dvd/
timezone --utc America/New_York
lang en_US.UTF-8
keyboard us
rootpw --plaintext password
reboot
firstboot --disable
services --enabled="sshd,chronyd"
zerombr
clearpart --all
autopart --type lvm
%packages
@core
%end
EOF
bash example_netinstall.sh
~~~
