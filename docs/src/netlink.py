#!/usr/bin/env python3
# We can get some significant inspiration from:
# https://gist.github.com/Lukasa/6209575d588f1584c374
# https://www.systutorials.com/docs/linux/man/7-netlink/

# define constants according to:
# /usr/include/linux/rtnetlink.h

# /* RTnetlink multicast groups - backwards compatibility for userspace */
#define RTMGRP_LINK             1
#define RTMGRP_NOTIFY           2
#define RTMGRP_NEIGH            4
#define RTMGRP_TC               8

#define RTMGRP_IPV4_IFADDR      0x10
#define RTMGRP_IPV4_MROUTE      0x20
#define RTMGRP_IPV4_ROUTE       0x40
#define RTMGRP_IPV4_RULE        0x80

#define RTMGRP_IPV6_IFADDR      0x100
#define RTMGRP_IPV6_MROUTE      0x200
#define RTMGRP_IPV6_ROUTE       0x400
#define RTMGRP_IPV6_IFINFO      0x800

#define RTMGRP_DECnet_IFADDR    0x1000
#define RTMGRP_DECnet_ROUTE     0x4000

#define RTMGRP_IPV6_PREFIX      0x20000

RTMGRP_IPV4_ROUTE = 0x40

# message types
#         RTM_NEWROUTE    = 24,
# #define RTM_NEWROUTE    RTM_NEWROUTE
#         RTM_DELROUTE,
# #define RTM_DELROUTE    RTM_DELROUTE
#         RTM_GETROUTE,
# #define RTM_GETROUTE    RTM_GETROUTE
RTM_NEWROUTE = 24
RTM_DELROUTE = 25
RTM_GETROUTE = 26
MESSAGE_TYPE = { RTM_NEWROUTE : 'RTM_NEWROUTE',
                 RTM_DELROUTE : 'RTM_DELROUTE',
                 RTM_GETROUTE : 'RTM_GETROUTE' }

import socket
import sys
import struct
import os

s = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, socket.NETLINK_ROUTE)
# listen to multicast group RTMGRP_IPV4_ROUTE so that we see all route related
# updates from the kernel
s.bind((os.getpid(), RTMGRP_IPV4_ROUTE))

while True:
    data = s.recv(65535)
    # the first 16 Bytes are the netlink header
    # https://docs.python.org/2/library/struct.html
    # https://tools.ietf.org/html/rfc3549#page-10
    netlink_header = data[:16]
    msg_len, msg_type, flags, seq, pid = struct.unpack("=LHHLL", netlink_header)
    print("Netlink header: ", msg_len, MESSAGE_TYPE[msg_type], flags, seq, pid)
    # unpacking the payload
    # see rtnetlink(7)
    #              struct rtmsg {
    #              unsigned char rtm_family;   /* Address family of route */
    #              unsigned char rtm_dst_len;  /* Length of destination */
    #              unsigned char rtm_src_len;  /* Length of source */
    #              unsigned char rtm_tos;      /* TOS filter */

    #              unsigned char rtm_table;    /* Routing table ID */
    #              unsigned char rtm_protocol; /* Routing protocol; see below */
    #              unsigned char rtm_scope;    /* See below */
    #              unsigned char rtm_type;     /* See below */

    #              unsigned int  rtm_flags;
    #          };
    netlink_service_template_header = data[16:28]
    rtm_family, rtm_dst_len, rtm_src_len, rtm_tos, \
        rtm_table, rtm_protocol, rtm_scope, rtm_type, rtm_flags = struct.unpack("=BBBBBBBBI", netlink_service_template_header)
    print("Netlink payload: ", rtm_family, rtm_dst_len, rtm_src_len, 
            rtm_tos, rtm_table, rtm_protocol, rtm_scope, rtm_type, rtm_flags)
    print("Netmask:", rtm_dst_len)
    print("IP: ", data[40:44])
