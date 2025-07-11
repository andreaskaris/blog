# Setting TAP interface MAC address without NET_ADMIN capabilities

When working with TAP interfaces, you might need to change MAC addresses without having NET_ADMIN capabilities. This post explores an alternative approach using the same method that DPDK employs.

## The Standard Approach and Its Limitations

Changing the MAC address of an interface normally requires net_admin capabilities. Let's take the `ip link` command
for example:

```
$ ip link ls dev tap0
10: tap0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc fq_codel state DOWN mode DEFAULT group default qlen 1000
    link/ether d6:27:b8:30:55:92 brd ff:ff:ff:ff:ff:ff
$ ip link set dev tap0 address d6:27:b8:30:55:93
RTNETLINK answers: Operation not permitted
```

The reason for this is that `ip link` uses netlink to achieve the MAC address change:

```
$ ip link set dev tap0 address d6:27:b8:30:55:93
(...)
sendmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base=[{nlmsg_len=44, nlmsg_type=RTM_NEWLINK, nlmsg_flags=NLM_F_REQUEST|NLM_F_ACK, nlmsg_seq=1752249385, nlmsg_pid=0}, {ifi_family=AF_UNSPEC, ifi_type=ARPHRD_NETROM, ifi_index=if_nametoindex("tap0"), ifi_flags=0, ifi_change=0}, [{nla_len=10, nla_type=IFLA_ADDRESS}, d6:27:b8:30:55:93]], iov_len=44}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 44
recvmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base=NULL, iov_len=0}], msg_iovlen=1, msg_controllen=0, msg_flags=MSG_TRUNC}, MSG_PEEK|MSG_TRUNC) = 64
recvmsg(3, {msg_name={sa_family=AF_NETLINK, nl_pid=0, nl_groups=00000000}, msg_namelen=12, msg_iov=[{iov_base=[{nlmsg_len=64, nlmsg_type=NLMSG_ERROR, nlmsg_flags=0, nlmsg_seq=1752249385, nlmsg_pid=585879}, {error=-EPERM, msg=[{nlmsg_len=44, nlmsg_type=RTM_NEWLINK, nlmsg_flags=NLM_F_REQUEST|NLM_F_ACK, nlmsg_seq=1752249385, nlmsg_pid=0}, {ifi_family=AF_UNSPEC, ifi_type=ARPHRD_NETROM, ifi_index=if_nametoindex("tap0"), ifi_flags=0, ifi_change=0}, [{nla_len=10, nla_type=IFLA_ADDRESS}, d6:27:b8:30:55:93]]}], iov_len=32768}], msg_iovlen=1, msg_controllen=0, msg_flags=0}, 0) = 64
write(2, "RTNETLINK answers: Operation not"..., 43RTNETLINK answers: Operation not permitted
) = 43
exit_group(2)                           = ?
+++ exited with 2 +++
```

## The Alternative: Using SIOCSIFHWADDR

There's actually another way to set the MAC address on Linux for TAP interfaces, via the SIOCSIFHWADDR ioctl call. This is the same approach that DPDK uses for TAP interface manipulation.

The key is that we can open the TAP interface through `/dev/net/tun` and use the SIOCSIFHWADDR ioctl directly, bypassing the netlink interface that requires NET_ADMIN capabilities.

### Writing the tool

Let's examine this approach:

```
/*
 * set-mac.c - Change MAC address of a TAP interface without NET_ADMIN capabilities
 *
 * This program allows changing the MAC address of an existing TAP interface
 * by opening it through /dev/net/tun without requiring NET_ADMIN capabilities.
 * It uses the same approach as DPDK for TAP interface manipulation.
 *
 * Build with: gcc set-mac.c -o set-mac
 * Usage: ./set-mac <interface_name> <mac_address>
 * Example: ./set-mac tap0 aa:bb:cc:dd:ee:ff
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/if_arp.h>
#include <errno.h>

#define PATH_NET_TUN "/dev/net/tun"

// See DPDK code for how it's done there:
// Open (exisitng) tap interface:
// https://github.com/DPDK/dpdk/blob/9fe9c0b231c8da5435bfccb1963121b4277f961c/drivers/net/virtio/virtio_user/vhost_kernel_tap.c#L45
// Set MAC address on tap interface:
// https://github.com/DPDK/dpdk/blob/9fe9c0b231c8da5435bfccb1963121b4277f961c/drivers/net/virtio/virtio_user/vhost_kernel_tap.c#L115

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <interface_name> <mac_address>\n", argv[0]);
        return 1;
    }

    int fd = open(PATH_NET_TUN, O_RDWR);
    if (fd < 0) {
        perror("ERROR: to open /dev/net/tun");
        return 1;
    }

    if (fcntl(fd, F_SETFL, O_RDONLY|O_NONBLOCK) < 0) {
        perror("ERROR: to set F_SETFL");
        return 1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ-1);
    
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_VNET_HDR;
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        printf("INFO: Could not do TUNSETIFF on tap interface without multiqueue, will try with multiqueue\n");
        ifr.ifr_flags |= IFF_MULTI_QUEUE;
        if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
            perror("ERROR: to TUNSETIFF on tap interface with multiqueue - cannot continue");
            return 1;
        }
    }

    unsigned char mac[6];
    sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, mac, 6);
    if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
        perror("ERROR: to set MAC address");
        return 1;
    }
    printf("SUCCESS\n");

    close(fd);
    return 0;
}
```

### Building and Using the Tool

Compile the program:

```
$ gcc set-mac.c -o set-mac
```

Now you can change the MAC address without requiring NET_ADMIN capabilities:

```
$ ip link ls dev tap0
10: tap0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc fq_codel state DOWN mode DEFAULT group default qlen 1000
    link/ether d6:27:b8:30:55:69 brd ff:ff:ff:ff:ff:ff
$ ./set-mac tap0 d6:27:b8:30:55:70
SUCCESS
$ ip link ls dev tap0
10: tap0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc fq_codel state DOWN mode DEFAULT group default qlen 1000
    link/ether d6:27:b8:30:55:70 brd ff:ff:ff:ff:ff:ff
```
