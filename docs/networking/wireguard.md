# Wireguard demo on Fedora 32 #

## Prerequisites ##

2 Fedora 32 nodes with kernel:
~~~
[root@wireguard01 ~]# uname -a
Linux wireguard01 5.6.6-300.fc32.x86_64 #1 SMP Tue Apr 21 13:44:19 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
[root@wireguard01 ~]# ip a | grep 192.168.122
    inet 192.168.122.250/24 brd 192.168.122.255 scope global dynamic noprefixroute eth0
~~~

~~~
[root@wireguard02 wireguard]# uname -a
Linux wireguard02 5.6.6-300.fc32.x86_64 #1 SMP Tue Apr 21 13:44:19 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
[root@wireguard02 wireguard]# ip a | grep 192.168.122
    inet 192.168.122.81/24 brd 192.168.122.255 scope global dynamic noprefixroute eth0
~~~

Install wireguard-tools, wireshark and tcpdump (the latter tools for monitoring):
~~~
yum install -y wireguard-tools wireshark tcpdump
~~~

## Create keys ##

wireguard01:
~~~
wg genkey > /etc/wireguard/private
chmod 600 /etc/wireguard/private
wg pubkey < /etc/wireguard/private > /etc/wireguard/publickey
~~~

~~~
[root@wireguard01 ~]# cat /etc/wireguard/private
0JYD7daZL+Bh7u77vFnwXHs5Rdi7iFfpk8NC1pg542Q=
[root@wireguard01 ~]# cat /etc/wireguard/publickey 
pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
~~~

wireguard02:
~~~
wg genkey > /etc/wireguard/private
chmod 600 /etc/wireguard/private
wg pubkey < /etc/wireguard/private > /etc/wireguard/publickey
~~~

~~~
[root@wireguard02 wireguard]# cat /etc/wireguard/private
wHpqAYwqSp6F4lO09uFj8BQGVJ6OjZIkvVWUQdy+GGk=
[root@wireguard02 wireguard]# cat /etc/wireguard/publickey 
uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
~~~

## Write down IP addresses and keys for later ##

wireguard01:
~~~
/etc/wireguard/private: 0JYD7daZL+Bh7u77vFnwXHs5Rdi7iFfpk8NC1pg542Q=
/etc/wireguard/publickey: pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
Outer IP: 192.168.122.250/24
Inner IP: 192.168.123.1/24
~~~

wireguard02:
~~~
/etc/wireguard/private: wHpqAYwqSp6F4lO09uFj8BQGVJ6OjZIkvVWUQdy+GGk=
/etc/wireguard/publickey: uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
Outer IP: 192.168.122.81/24
Inner IP: 192.168.123.2/32
~~~

## Manual tunnel setup ##

Follow these steps to create tunnels manually.

wireguard01:
~~~
ip link add wg0 type wireguard
ip a a 192.168.123.1/24 dev wg0
wg set wg0 private-key /etc/wireguard/private
wg set wg0 listen-port 51820
wg set wg0 peer uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc= allowed-ips 192.168.123.2/32 endpoint 192.168.122.81:51820 persistent-keepalive 30
ip link set dev wg0 up
~~~

wireguard02:
~~~
ip link add wg0 type wireguard
ip a a 192.168.123.2/24 dev wg0
wg set wg0 private-key /etc/wireguard/private
wg set wg0 listen-port 51820
wg set wg0 peer pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw= allowed-ips 192.168.123.1/32 endpoint 192.168.122.250:51820 persistent-keepalive 30
ip link set dev wg0 up
~~~

Verify wireguard:
~~~
[root@wireguard01 ~]# wg
interface: wg0
  public key: pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
  private key: (hidden)
  listening port: 51820

peer: uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
  endpoint: 192.168.122.81:51820
  allowed ips: 192.168.123.2/32
  latest handshake: 20 seconds ago
  transfer: 180 B received, 568 B sent
  persistent keepalive: every 30 seconds
~~~

~~~
[root@wireguard02 wireguard]# wg
interface: wg0
  public key: uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
  private key: (hidden)
  listening port: 51820

peer: pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
  endpoint: 192.168.122.250:51820
  allowed ips: 192.168.123.1/32
  latest handshake: 34 seconds ago
  transfer: 156 B received, 180 B sent
  persistent keepalive: every 30 seconds
~~~

~~~
[root@wireguard01 ~]# ping -c1 -W1 192.168.123.2
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=0.383 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.383/0.383/0.383/0.000 ms
~~~

Delete interfaces again, run on both hosts:
~~~
ip link del dev wg0
~~~

## Tunnel setup with config files ##

Follow these steps to set up tunnels with systemd:

~~~
[root@wireguard01 ~]# cat <<'EOF' > /etc/wireguard/wg0.conf
[Interface]
Address    = 192.168.123.1/24
PrivateKey = 0JYD7daZL+Bh7u77vFnwXHs5Rdi7iFfpk8NC1pg542Q=
ListenPort = 51820

[Peer]
PublicKey    = uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
AllowedIPs   = 192.168.123.2/32
Endpoint     = 192.168.122.81:51820
PersistentKeepalive = 30
EOF
~~~

~~~
[root@wireguard02 wireguard]# cat <<'EOF' > /etc/wireguard/wg0.conf
[Interface]
Address    = 192.168.123.2/32
PrivateKey = wHpqAYwqSp6F4lO09uFj8BQGVJ6OjZIkvVWUQdy+GGk=

[Peer]
PublicKey    = pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
AllowedIPs   = 192.168.123.1/32
Endpoint     = 192.168.122.250:51820
PersistentKeepalive = 30
EOF
~~~

~~~
[root@wireguard01 ~]# journalctl -u wg-quick@wg0 -f -n0 &
[1] 11296
[root@wireguard01 ~]# -- Logs begin at Wed 2020-08-05 15:44:13 UTC. --

[root@wireguard01 ~]# systemctl start wg-quick@wg0
[root@wireguard01 ~]# Aug 05 20:06:57 wireguard01 systemd[1]: Starting WireGuard via wg-quick(8) for wg0...
Aug 05 20:06:57 wireguard01 wg-quick[11299]: [#] ip link add wg0 type wireguard
Aug 05 20:06:57 wireguard01 wg-quick[11299]: [#] wg setconf wg0 /dev/fd/63
Aug 05 20:06:57 wireguard01 wg-quick[11299]: [#] ip -4 address add 192.168.123.1/24 dev wg0
Aug 05 20:06:57 wireguard01 wg-quick[11299]: [#] ip link set mtu 1420 up dev wg0
Aug 05 20:06:57 wireguard01 systemd[1]: Finished WireGuard via wg-quick(8) for wg0.

[root@wireguard01 ~]# fg
journalctl -u wg-quick@wg0 -f -n0
^C
[root@wireguard01 ~]# wg
interface: wg0
  public key: pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
  private key: (hidden)
  listening port: 51820

peer: uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
  endpoint: 192.168.122.81:34730
  allowed ips: 192.168.123.2/32
  latest handshake: 1 minute, 7 seconds ago
  transfer: 180 B received, 484 B sent
  persistent keepalive: every 30 seconds
~~~

~~~
[root@wireguard02 wireguard]# -- Logs begin at Wed 2020-08-05 15:44:47 UTC. --

[root@wireguard02 wireguard]# systemctl start wg-quick@wg0
[root@wireguard02 wireguard]# Aug 05 20:07:04 wireguard02 systemd[1]: Starting WireGuard via wg-quick(8) for wg0...
Aug 05 20:07:04 wireguard02 wg-quick[10492]: [#] ip link add wg0 type wireguard
Aug 05 20:07:04 wireguard02 wg-quick[10492]: [#] wg setconf wg0 /dev/fd/63
Aug 05 20:07:04 wireguard02 wg-quick[10492]: [#] ip -4 address add 192.168.123.2/32 dev wg0
Aug 05 20:07:04 wireguard02 wg-quick[10492]: [#] ip link set mtu 1420 up dev wg0
Aug 05 20:07:04 wireguard02 wg-quick[10492]: [#] ip -4 route add 192.168.123.1/32 dev wg0
Aug 05 20:07:04 wireguard02 systemd[1]: Finished WireGuard via wg-quick(8) for wg0.
^C
[root@wireguard02 wireguard]# fg
journalctl -u wg-quick@wg0 -f -n0
^C
[root@wireguard02 wireguard]# wg
interface: wg0
  public key: uD48Km4aRYXD7OvUtiatwqvvBG35lAad2j4hCpgNYEc=
  private key: (hidden)
  listening port: 34730

peer: pkyJK9ePlOTW9+GK73UnCZ4/b7/3xOthdfdbF7OQUzw=
  endpoint: 192.168.122.250:51820
  allowed ips: 192.168.123.1/32
  latest handshake: 1 minute, 8 seconds ago
  transfer: 188 B received, 180 B sent
  persistent keepalive: every 30 seconds
~~~

And test ping:
~~~
[root@wireguard01 ~]# ping -c1 -W1 192.168.123.2
PING 192.168.123.2 (192.168.123.2) 56(84) bytes of data.
64 bytes from 192.168.123.2: icmp_seq=1 ttl=64 time=0.356 ms

--- 192.168.123.2 ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.356/0.356/0.356/0.000 ms
[root@wireguard01 ~]# 
~~~

### Starting tunnels on system boot ###

Simply run:
~~~
systemctl enable wg-quick@wg0
~~~

## Resources ##

* [https://www.wireguard.com/quickstart/](https://www.wireguard.com/quickstart/)
* [https://fedoramagazine.org/build-a-virtual-private-network-with-wireguard/](https://fedoramagazine.org/build-a-virtual-private-network-with-wireguard/)
* [https://lore.kernel.org/lkml/CAHk-=wi9ZT7Stg-uSpX0UWQzam6OP9Jzz6Xu1CkYu1cicpD5OA@mail.gmail.com/](https://lore.kernel.org/lkml/CAHk-=wi9ZT7Stg-uSpX0UWQzam6OP9Jzz6Xu1CkYu1cicpD5OA@mail.gmail.com/)
* [https://arstechnica.com/gadgets/2020/03/wireguard-vpn-makes-it-to-1-0-0-and-into-the-next-linux-kernel/](https://arstechnica.com/gadgets/2020/03/wireguard-vpn-makes-it-to-1-0-0-and-into-the-next-linux-kernel/)
* [https://arstechnica.com/gadgets/2020/03/wireguard-vpn-makes-it-to-1-0-0-and-into-the-next-linux-kernel/](https://www.wireguard.com/papers/wireguard.pdf)
