# Juniper commands

The following commands were validated against a qfx5100-48s-6q with Junos 17.3R3-S4.2:
~~~
root@gss-sw02> show version 
fpc0:
--------------------------------------------------------------------------
Hostname: sw02
Model: qfx5100-48s-6q
Junos: 17.3R3-S4.2
JUNOS Base OS boot [17.3R3-S4.2]
JUNOS Base OS Software Suite [17.3R3-S4.2]
JUNOS Crypto Software Suite [17.3R3-S4.2]
JUNOS Crypto Software Suite [17.3R3-S4.2]
JUNOS Online Documentation [17.3R3-S4.2]
JUNOS Kernel Software Suite [17.3R3-S4.2]
JUNOS Packet Forwarding Engine Support (qfx-ex-x86-32) [17.3R3-S4.2]
JUNOS Routing Software Suite [17.3R3-S4.2]
JUNOS jsd [i386-17.3R3-S4.2-jet-1]
JUNOS SDN Software Suite [17.3R3-S4.2]
JUNOS Enterprise Software Suite [17.3R3-S4.2]
JUNOS Web Management Platform Package [17.3R3-S4.2]
JUNOS py-base-i386 [17.3R3-S4.2]
JUNOS py-extensions-i386 [17.3R3-S4.2]
JUNOS Host Software [17.3R3-S4.2]
~~~

## How to label interfaces on a Juniper switch with X520 cards ##

Enable console logging on the Juniper switch:
~~~
root@-sw02> monitor start messages | match ifOperStatus
~~~

Disable console logging on the Juniper switch:
~~~
root@-sw02> monitor stop messages
~~~

Flap server interface:
~~~
[root@02 ~]#  ethtool -r p1p3
~~~

This will log the following in the switch:
~~~
root@-sw02> Aug  9 05:40:40  -sw02 mib2d[2112]: SNMP_TRAP_LINK_DOWN: ifIndex 521, ifAdminStatus up(1), ifOperStatus down(2), ifName xe-0/0/2
Aug  9 05:40:40  -sw02 mib2d[2112]: SNMP_TRAP_LINK_UP: ifIndex 521, ifAdminStatus up(1), ifOperStatus up(1), ifName xe-0/0/2
~~~

Now, one can set an interface description:
~~~
root@-sw02> edit     
Entering configuration mode

{master:0}[edit]
root@-sw02# set interfaces xe-0/0/2 description "02 p1p3" 

root@-sw02# commit  
configuration check succeeds
commit complete

{master:0}[edit]
root@-sw02# exit 
Exiting configuration mode

{master:0}
root@-sw02> show interfaces descriptions    
Interface       Admin Link Description
xe-0/0/0        up    up   01 p1p3
xe-0/0/1        up    up   01 p1p4
xe-0/0/2        up    up   02 p1p3
~~~

## How to configure interface VLANs ##

### Access VLANs ###

~~~
{master:0}[edit]
root@-sw02# set interfaces xe-0/0/0 unit 0 family
ethernet-switching vlan members vlan259
{master:0}[edit]
root@-sw02# commit
{master:0}[edit]
root@-sw02# exit
~~~

Verify:
~~~
{master:0}
root@-sw02> show vlans brief

Routing instance        VLAN name             Tag          Interfaces
default-switch          default               1

default-switch          native_vlan515        515

default-switch          vlan250               250

default-switch          vlan251               251

default-switch          vlan252               252

default-switch          vlan253               253

default-switch          vlan254               254

default-switch          vlan255               255

default-switch          vlan256               256

default-switch          vlan257               257

default-switch          vlan258               258

default-switch          vlan259               259
                                                           xe-0/0/0.0*

{master:0}
~~~

### Trunks ###

~~~
root@-sw02> edit
Entering configuration mode
The configuration has been changed but not committed

{master:0}[edit]
root@-sw02# set interfaces xe-0/0/2 unit 0 family ethernet-switching vlan members [ 251 257-259 ]

{master:0}[edit]
root@-sw02# set interfaces xe-0/0/2 unit 0 family ethernet-switching interface-mode trunk

{master:0}[edit]
root@-sw02# set interfaces xe-0/0/2 native-vlan-id 259

{master:0}[edit]
root@-sw02# commit
[edit interfaces xe-0/0/2 unit 0 family]
  'ethernet-switching'
    Family ethernet-switching and rest of the families are mutually exclusive
error: commit failed: (statements constraint check failed)

{master:0}[edit]
root@-sw02# show interfaces xe-0/0/2
description "02 p1p3";
native-vlan-id 259;
unit 0 {
    family inet {
        dhcp {
            vendor-id Juniper-qfx5100-48s-6q;
        }
    }
    ##
    ## Warning: Family ethernet-switching and rest of the families are
mutually exclusive
    ##
    family ethernet-switching {
        interface-mode trunk;
        vlan {
            members [ 251 257-259 ];
        }
    }
}

{master:0}[edit]

root@-sw02# delete interfaces xe-0/0/2 unit 0 family inet

{master:0}[edit]
root@-sw02# commit
configuration check succeeds
commit complete

{master:0}[edit]
root@-sw02# exit
Exiting configuration mode

{master:0}
~~~

Verification:
~~~
root@-sw02> show vlans brief

Routing instance        VLAN name             Tag          Interfaces
default-switch          default               1

default-switch          native_vlan515        515

default-switch          vlan250               250

default-switch          vlan251               251
                                                           xe-0/0/2.0
default-switch          vlan252               252

default-switch          vlan253               253

default-switch          vlan254               254

default-switch          vlan255               255

default-switch          vlan256               256

default-switch          vlan257               257
                                                           xe-0/0/2.0
default-switch          vlan258               258
                                                           xe-0/0/2.0
default-switch          vlan259               259
                                                           xe-0/0/0.0*
                                                           xe-0/0/2.0

{master:0}
root@-sw02>
~~~
