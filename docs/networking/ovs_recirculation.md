# OVS packet recirculation #

Packet recirculation means that a packet is sent back to the processing engine for another evaluation after it has already been processed. This can for example happen when an outer header is stripped off the packet such as VXLAN tunneling or MPLS. It is also important for connection tracking in OVS and is also used in OVS bonds when hashing packets to different ports.

[https://lwn.net/Articles/546476/](https://lwn.net/Articles/546476/)
~~~
(...)
Recirculation is a technique to allow a frame to re-enter
frame processing. This is intended to be used after actions
have been applied to the frame with modify the frame in
some way that makes it possible for richer processing to occur.

An example is and indeed targeted use case is MPLS. If an MPLS frame has an
mpls_pop action applied with the IPv4 ethernet type then it becomes
possible to decode the IPv4 portion of the frame. This may be used to
construct a facet that modifies the IPv4 portion of the frame. This is not
possible prior to the mpls_pop action as the contents of the frame after
the MPLS stack is not known to be IPv4.
(...)
~~~
Also: [https://lists.openwall.net/netdev/2013/04/16/16](https://lists.openwall.net/netdev/2013/04/16/16)

[http://www.openvswitch.org/support/dist-docs/ovs-vswitchd.8.txt](http://www.openvswitch.org/support/dist-docs/ovs-vswitchd.8.txt)
~~~
(...)
The sum of "emc hits", "smc hits", "megaflow hits" and "miss" is
              the  number  of packet lookups performed by the datapath. Beware
              that a recirculated packet experiences one additional lookup per
              recirculation, so there may be more lookups than forwarded pack‐
              ets in the datapath.
(...)
~~~

[https://mail.openvswitch.org/pipermail/ovs-dev/2017-March/330278.html](https://mail.openvswitch.org/pipermail/ovs-dev/2017-March/330278.html)
[https://patchwork.ozlabs.org/patch/709823/](https://patchwork.ozlabs.org/patch/709823/)

[https://lwn.net/Articles/679808/](https://lwn.net/Articles/679808/)
~~~
(...)
The IPv4 traffic coming from port 2 is first matched for the
non-tracked state (-trk), which means that the packet has not been
through a CT action yet.  Such traffic is run trough the conntrack in
zone 1 and all packets associated with a NATted connection are NATted
also in the return direction.  After the packet has been through
conntrack it is recirculated back to OpenFlow table 0 (which is the
default table, so all the rules above are in table 0).  The CT action
changes the 'trk' flag to being set, so the packets after
recirculation no longer match the second rule.  The third rule then
matches the recirculated packets that were marked as established by
conntrack (+est), and the packet is output on port 1.  Matching on
ct_zone is not strictly needed, but in this test case it verifies that
the ct_zone key attribute is properly set by the conntrack action.
(...)
~~~

From `openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h`:
~~~
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * Freezing and recirculation
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ==========================
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Freezing is a technique for halting and checkpointing packet translation in
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * a way that it can be restarted again later.  This file has a couple of data
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * structures related to freezing in general; their names begin with "frozen".
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is the use of freezing to allow a frame to re-enter the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * datapath packet processing path to achieve more flexible packet processing,
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * such as modifying header fields after MPLS POP action and selecting a slave
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * port for bond ports.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Data path and user space interface
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * -----------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation uses two uint32_t fields, recirc_id and dp_hash, and a RECIRC
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * action.  recirc_id is used to select the next packet processing steps among
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * multiple instances of recirculation.  When a packet initially enters the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath it is assigned with recirc_id 0, which indicates no recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirc_ids are managed by the user space, opaque to the datapath.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * On the other hand, dp_hash can only be computed by the datapath, opaque to
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * the user space, as the datapath is free to choose the hashing algorithm
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * without informing user space about it.  The dp_hash value should be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * wildcarded for newly received packets.  HASH action specifies whether the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash is computed, and if computed, how many fields are to be included in the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash computation.  The computed hash value is stored into the dp_hash field
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * prior to recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * The RECIRC action sets the recirc_id field and then reprocesses the packet
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * as if it was received again on the same input port.  RECIRC action works
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * like a function call; actions listed after the RECIRC action will be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * executed after recirculation.  RECIRC action can be nested, but datapath
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * implementation limits the number of nested recirculations to prevent
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * unreasonable nesting depth or infinite loop.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * User space recirculation context
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ---------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is usually hidden from the OpenFlow controllers.  Action
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * translation code deduces when recirculation is necessary and issues a
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath recirculation action.  All OpenFlow actions to be performed after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation are derived from the OpenFlow pipeline and are stored with the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation ID.  When the OpenFlow tables are changed in a way affecting
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * the recirculation flows, new recirculation ID with new metadata and actions
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * is allocated and the old one is timed out.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID pool
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ----------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID needs to be unique for all datapaths.  Recirculation ID
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * pool keeps track of recirculation ids and stores OpenFlow pipeline
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * translation context so that flow processing may continue after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * A Recirculation ID can be any uint32_t value, except for that the value 0 is
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * reserved for 'no recirculation' case.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "ofproto-dpif-mirror.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "openvswitch/list.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "openvswitch/ofp-actions.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "ovs-thread.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "uuid.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct ofproto_dpif;
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct rule;
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-/*
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * Freezing and recirculation
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ==========================
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Freezing is a technique for halting and checkpointing packet translation in
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * a way that it can be restarted again later.  This file has a couple of data
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * structures related to freezing in general; their names begin with "frozen".
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is the use of freezing to allow a frame to re-enter the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * datapath packet processing path to achieve more flexible packet processing,
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * such as modifying header fields after MPLS POP action and selecting a slave
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * port for bond ports.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Data path and user space interface
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * -----------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation uses two uint32_t fields, recirc_id and dp_hash, and a RECIRC
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * action.  recirc_id is used to select the next packet processing steps among
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * multiple instances of recirculation.  When a packet initially enters the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath it is assigned with recirc_id 0, which indicates no recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirc_ids are managed by the user space, opaque to the datapath.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * On the other hand, dp_hash can only be computed by the datapath, opaque to
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * the user space, as the datapath is free to choose the hashing algorithm
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * without informing user space about it.  The dp_hash value should be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * wildcarded for newly received packets.  HASH action specifies whether the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash is computed, and if computed, how many fields are to be included in the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash computation.  The computed hash value is stored into the dp_hash field
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * prior to recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * The RECIRC action sets the recirc_id field and then reprocesses the packet
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * as if it was received again on the same input port.  RECIRC action works
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * like a function call; actions listed after the RECIRC action will be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * executed after recirculation.  RECIRC action can be nested, but datapath
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * implementation limits the number of nested recirculations to prevent
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * unreasonable nesting depth or infinite loop.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * User space recirculation context
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ---------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is usually hidden from the OpenFlow controllers.  Action
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * translation code deduces when recirculation is necessary and issues a
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath recirculation action.  All OpenFlow actions to be performed after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation are derived from the OpenFlow pipeline and are stored with the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation ID.  When the OpenFlow tables are changed in a way affecting
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * the recirculation flows, new recirculation ID with new metadata and actions
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * is allocated and the old one is timed out.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID pool
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ----------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID needs to be unique for all datapaths.  Recirculation ID
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * pool keeps track of recirculation ids and stores OpenFlow pipeline
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * translation context so that flow processing may continue after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * A Recirculation ID can be any uint32_t value, except for that the value 0 is
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * reserved for 'no recirculation' case.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Thread-safety
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * --------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * All APIs are thread safe.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h:/* Metadata for restoring pipeline context after recirculation.  Helpers
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * are inlined below to keep them together with the definition for easier
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * updates. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-BUILD_ASSERT_DECL(FLOW_WC_SEQ == 40);
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct frozen_metadata {
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    /* Metadata in struct flow. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    struct flow_tnl tunnel;       /* Encapsulating tunnel parameters. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    ovs_be64 metadata;            /* OpenFlow Metadata. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    uint64_t regs[FLOW_N_XREGS];  /* Registers. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    ofp_port_t in_port;           /* Incoming port. */
--
openvswitch-2.9.0/tests/ofproto-dpif.at:# This test verifies that the table ID is preserved across recirculation
openvswitch-2.9.0/tests/ofproto-dpif.at-# when a resubmit action requires it (because the action is relative to
openvswitch-2.9.0/tests/ofproto-dpif.at-# the current table rather than specifying a table).
openvswitch-2.9.0/tests/ofproto-dpif.at:AT_SETUP([ofproto-dpif - resubmit with recirculation])
--
openvswitch-2.9.0/tests/ofproto-dpif.at-# This test verifies that tunnel metadata is preserved across
openvswitch-2.9.0/tests/ofproto-dpif.at:# recirculation.  At the time of recirculation, fields such as "tun_id"
openvswitch-2.9.0/tests/ofproto-dpif.at-# may be set before the tunnel is "valid" (ie, has a destination
openvswitch-2.9.0/tests/ofproto-dpif.at:# address), but the field should still be available after recirculation.
openvswitch-2.9.0/tests/ofproto-dpif.at-AT_SETUP([ofproto-dpif - resubmit with tun_id])
--
openvswitch-2.9.0/tests/ofproto-dpif.at:# This test verifies that "resubmit", when it triggers recirculation
openvswitch-2.9.0/tests/ofproto-dpif.at-# indirectly through the flow that it recursively invokes, is not
openvswitch-2.9.0/tests/ofproto-dpif.at:# re-executed when execution continues later post-recirculation.
openvswitch-2.9.0/tests/ofproto-dpif.at:AT_SETUP([ofproto-dpif - recirculation after resubmit])
--
openvswitch-2.9.0/tests/system-traffic.at-dnl Checks the implementation of conntrack with FTP ALGs in combination with
openvswitch-2.9.0/tests/system-traffic.at-dnl NAT, with flow tables that implement the NATing after the first round
openvswitch-2.9.0/tests/system-traffic.at:dnl of recirculation - that is, the first flow ct(table=foo) then a subsequent
openvswitch-2.9.0/tests/system-traffic.at-dnl flow will implement the NATing with ct(nat..),output:foo.
--
openvswitch-2.9.0/tests/system-traffic.at-dnl Checks the implementation of conntrack original direction tuple matching
openvswitch-2.9.0/tests/system-traffic.at-dnl with FTP ALGs in combination with NAT, with flow tables that implement
openvswitch-2.9.0/tests/system-traffic.at:dnl the NATing before the first round of recirculation - that is, the first
openvswitch-2.9.0/tests/system-traffic.at-dnl flow ct(nat, table=foo) then a subsequent flow will implement the
openvswitch-2.9.0/tests/system-traffic.at-dnl commiting of NATed and other connections with ct(nat..),output:foo.
--
openvswitch-2.9.0/tests/packet-type-aware.at:# Goto_table after pop_mpls triggers recirculation.
openvswitch-2.9.0/tests/packet-type-aware.at-AT_CHECK([
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl del-flows br0 &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl del-flows int-br &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow br0 "actions=normal"
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow int-br "table=0,in_port=tunnel,actions=pop_mpls:0x800,goto_table:20" &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow int-br "table=20,actions=dec_ttl,output:LOCAL"
openvswitch-2.9.0/tests/packet-type-aware.at-], [0], [ignore])
openvswitch-2.9.0/tests/packet-type-aware.at-
--
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "ofproto-dpif-mirror.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "openvswitch/list.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "openvswitch/ofp-actions.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "ovs-thread.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-#include "uuid.h"
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct ofproto_dpif;
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct rule;
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-/*
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * Freezing and recirculation
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ==========================
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Freezing is a technique for halting and checkpointing packet translation in
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * a way that it can be restarted again later.  This file has a couple of data
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * structures related to freezing in general; their names begin with "frozen".
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is the use of freezing to allow a frame to re-enter the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * datapath packet processing path to achieve more flexible packet processing,
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * such as modifying header fields after MPLS POP action and selecting a slave
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * port for bond ports.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Data path and user space interface
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * -----------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation uses two uint32_t fields, recirc_id and dp_hash, and a RECIRC
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * action.  recirc_id is used to select the next packet processing steps among
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * multiple instances of recirculation.  When a packet initially enters the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath it is assigned with recirc_id 0, which indicates no recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirc_ids are managed by the user space, opaque to the datapath.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * On the other hand, dp_hash can only be computed by the datapath, opaque to
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * the user space, as the datapath is free to choose the hashing algorithm
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * without informing user space about it.  The dp_hash value should be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * wildcarded for newly received packets.  HASH action specifies whether the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash is computed, and if computed, how many fields are to be included in the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * hash computation.  The computed hash value is stored into the dp_hash field
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * prior to recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * The RECIRC action sets the recirc_id field and then reprocesses the packet
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * as if it was received again on the same input port.  RECIRC action works
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * like a function call; actions listed after the RECIRC action will be
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * executed after recirculation.  RECIRC action can be nested, but datapath
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * implementation limits the number of nested recirculations to prevent
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * unreasonable nesting depth or infinite loop.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * User space recirculation context
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ---------------------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation is usually hidden from the OpenFlow controllers.  Action
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * translation code deduces when recirculation is necessary and issues a
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * datapath recirculation action.  All OpenFlow actions to be performed after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation are derived from the OpenFlow pipeline and are stored with the
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation ID.  When the OpenFlow tables are changed in a way affecting
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * the recirculation flows, new recirculation ID with new metadata and actions
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * is allocated and the old one is timed out.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID pool
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * ----------------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Recirculation ID needs to be unique for all datapaths.  Recirculation ID
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * pool keeps track of recirculation ids and stores OpenFlow pipeline
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * translation context so that flow processing may continue after
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * recirculation.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * A Recirculation ID can be any uint32_t value, except for that the value 0 is
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h: * reserved for 'no recirculation' case.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * Thread-safety
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * --------------
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- *
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * All APIs are thread safe.
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h:/* Metadata for restoring pipeline context after recirculation.  Helpers
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * are inlined below to keep them together with the definition for easier
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h- * updates. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-BUILD_ASSERT_DECL(FLOW_WC_SEQ == 40);
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-struct frozen_metadata {
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    /* Metadata in struct flow. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    struct flow_tnl tunnel;       /* Encapsulating tunnel parameters. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    ovs_be64 metadata;            /* OpenFlow Metadata. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    uint64_t regs[FLOW_N_XREGS];  /* Registers. */
openvswitch-2.9.0/ofproto/ofproto-dpif-rid.h-    ofp_port_t in_port;           /* Incoming port. */
--
openvswitch-2.9.0/tests/ofproto-dpif.at:# This test verifies that the table ID is preserved across recirculation
openvswitch-2.9.0/tests/ofproto-dpif.at-# when a resubmit action requires it (because the action is relative to
openvswitch-2.9.0/tests/ofproto-dpif.at-# the current table rather than specifying a table).
openvswitch-2.9.0/tests/ofproto-dpif.at:AT_SETUP([ofproto-dpif - resubmit with recirculation])
--
openvswitch-2.9.0/tests/ofproto-dpif.at-# This test verifies that tunnel metadata is preserved across
openvswitch-2.9.0/tests/ofproto-dpif.at:# recirculation.  At the time of recirculation, fields such as "tun_id"
openvswitch-2.9.0/tests/ofproto-dpif.at-# may be set before the tunnel is "valid" (ie, has a destination
openvswitch-2.9.0/tests/ofproto-dpif.at:# address), but the field should still be available after recirculation.
openvswitch-2.9.0/tests/ofproto-dpif.at-AT_SETUP([ofproto-dpif - resubmit with tun_id])
--
openvswitch-2.9.0/tests/ofproto-dpif.at:# This test verifies that "resubmit", when it triggers recirculation
openvswitch-2.9.0/tests/ofproto-dpif.at-# indirectly through the flow that it recursively invokes, is not
openvswitch-2.9.0/tests/ofproto-dpif.at:# re-executed when execution continues later post-recirculation.
openvswitch-2.9.0/tests/ofproto-dpif.at:AT_SETUP([ofproto-dpif - recirculation after resubmit])
--
openvswitch-2.9.0/tests/system-traffic.at-dnl Checks the implementation of conntrack with FTP ALGs in combination with
openvswitch-2.9.0/tests/system-traffic.at-dnl NAT, with flow tables that implement the NATing after the first round
openvswitch-2.9.0/tests/system-traffic.at:dnl of recirculation - that is, the first flow ct(table=foo) then a subsequent
openvswitch-2.9.0/tests/system-traffic.at-dnl flow will implement the NATing with ct(nat..),output:foo.
--
openvswitch-2.9.0/tests/system-traffic.at-dnl Checks the implementation of conntrack original direction tuple matching
openvswitch-2.9.0/tests/system-traffic.at-dnl with FTP ALGs in combination with NAT, with flow tables that implement
openvswitch-2.9.0/tests/system-traffic.at:dnl the NATing before the first round of recirculation - that is, the first
openvswitch-2.9.0/tests/system-traffic.at-dnl flow ct(nat, table=foo) then a subsequent flow will implement the
openvswitch-2.9.0/tests/system-traffic.at-dnl commiting of NATed and other connections with ct(nat..),output:foo.
--
openvswitch-2.9.0/tests/packet-type-aware.at:# Goto_table after pop_mpls triggers recirculation.
openvswitch-2.9.0/tests/packet-type-aware.at-AT_CHECK([
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl del-flows br0 &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl del-flows int-br &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow br0 "actions=normal"
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow int-br "table=0,in_port=tunnel,actions=pop_mpls:0x800,goto_table:20" &&
openvswitch-2.9.0/tests/packet-type-aware.at-    ovs-ofctl add-flow int-br "table=20,actions=dec_ttl,output:LOCAL"
openvswitch-2.9.0/tests/packet-type-aware.at-], [0], [ignore])
openvswitch-2.9.0/tests/packet-type-aware.at-
--
openvswitch-2.9.0/include/openvswitch/meta-flow.h-    /* "recirc_id".
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     *
openvswitch-2.9.0/include/openvswitch/meta-flow.h:     * ID for recirculation.  The value 0 is reserved for initially received
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * packets.  Internal use only, not programmable from controller.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     *
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * Type: be32.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * Maskable: no.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * Formatting: decimal.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * Prerequisites: none.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * Access: read-only.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * NXM: NXM_NX_RECIRC_ID(36) since v2.2.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     * OXM: none.
openvswitch-2.9.0/include/openvswitch/meta-flow.h-     */
--
openvswitch-2.9.0/lib/dpif-netdev.c-    pmd_perf_update_counter(&pmd->perf_stats, PMD_STAT_MASKED_HIT,
openvswitch-2.9.0/lib/dpif-netdev.c-                            cnt - upcall_ok_cnt - upcall_fail_cnt);
openvswitch-2.9.0/lib/dpif-netdev.c-    pmd_perf_update_counter(&pmd->perf_stats, PMD_STAT_MASKED_LOOKUP,
openvswitch-2.9.0/lib/dpif-netdev.c-                            lookup_cnt);
openvswitch-2.9.0/lib/dpif-netdev.c-    pmd_perf_update_counter(&pmd->perf_stats, PMD_STAT_MISS,
openvswitch-2.9.0/lib/dpif-netdev.c-                            upcall_ok_cnt);
openvswitch-2.9.0/lib/dpif-netdev.c-    pmd_perf_update_counter(&pmd->perf_stats, PMD_STAT_LOST,
openvswitch-2.9.0/lib/dpif-netdev.c-                            upcall_fail_cnt);
openvswitch-2.9.0/lib/dpif-netdev.c-}
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c:/* Packets enter the datapath from a port (or from recirculation) here.
openvswitch-2.9.0/lib/dpif-netdev.c- *
openvswitch-2.9.0/lib/dpif-netdev.c- * When 'md_is_valid' is true the metadata in 'packets' are already valid.
openvswitch-2.9.0/lib/dpif-netdev.c- * When false the metadata in 'packets' need to be initialized. */
openvswitch-2.9.0/lib/dpif-netdev.c-static void
openvswitch-2.9.0/lib/dpif-netdev.c-dp_netdev_input__(struct dp_netdev_pmd_thread *pmd,
openvswitch-2.9.0/lib/dpif-netdev.c-                  struct dp_packet_batch *packets,
openvswitch-2.9.0/lib/dpif-netdev.c-                  bool md_is_valid, odp_port_t port_no)
openvswitch-2.9.0/lib/dpif-netdev.c-{
openvswitch-2.9.0/lib/dpif-netdev.c-#if !defined(__CHECKER__) && !defined(_WIN32)
openvswitch-2.9.0/lib/dpif-netdev.c-    const size_t PKT_ARRAY_SIZE = dp_packet_batch_size(packets);
--
openvswitch-2.9.0/lib/dpif-netdev.c-                            md_is_valid, port_no);
openvswitch-2.9.0/lib/dpif-netdev.c-    if (!dp_packet_batch_is_empty(packets)) {
openvswitch-2.9.0/lib/dpif-netdev.c-        /* Get ingress port from first packet's metadata. */
openvswitch-2.9.0/lib/dpif-netdev.c-        in_port = packets->packets[0]->md.in_port.odp_port;
openvswitch-2.9.0/lib/dpif-netdev.c-        fast_path_processing(pmd, packets, keys,
openvswitch-2.9.0/lib/dpif-netdev.c-                             batches, &n_batches, in_port);
openvswitch-2.9.0/lib/dpif-netdev.c-    }
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c-    /* All the flow batches need to be reset before any call to
openvswitch-2.9.0/lib/dpif-netdev.c-     * packet_batch_per_flow_execute() as it could potentially trigger
openvswitch-2.9.0/lib/dpif-netdev.c:     * recirculation. When a packet matching flow ‘j’ happens to be
openvswitch-2.9.0/lib/dpif-netdev.c-     * recirculated, the nested call to dp_netdev_input__() could potentially
openvswitch-2.9.0/lib/dpif-netdev.c-     * classify the packet as matching another flow - say 'k'. It could happen
openvswitch-2.9.0/lib/dpif-netdev.c-     * that in the previous call to dp_netdev_input__() that same flow 'k' had
openvswitch-2.9.0/lib/dpif-netdev.c-     * already its own batches[k] still waiting to be served.  So if its
openvswitch-2.9.0/lib/dpif-netdev.c-     * ‘batch’ member is not reset, the recirculated packet would be wrongly
openvswitch-2.9.0/lib/dpif-netdev.c-     * appended to batches[k] of the 1st call to dp_netdev_input__(). */
openvswitch-2.9.0/lib/dpif-netdev.c-    size_t i;
openvswitch-2.9.0/lib/dpif-netdev.c-    for (i = 0; i < n_batches; i++) {
openvswitch-2.9.0/lib/dpif-netdev.c-        batches[i].flow->batch = NULL;
openvswitch-2.9.0/lib/dpif-netdev.c-    }
--
openvswitch-2.9.0/lib/dpif-netdev.c-                packet->md.recirc_id = nl_attr_get_u32(a);
openvswitch-2.9.0/lib/dpif-netdev.c-            }
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c-            (*depth)++;
openvswitch-2.9.0/lib/dpif-netdev.c-            dp_netdev_recirculate(pmd, packets_);
openvswitch-2.9.0/lib/dpif-netdev.c-            (*depth)--;
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c-            return;
openvswitch-2.9.0/lib/dpif-netdev.c-        }
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c:        VLOG_WARN("Packet dropped. Max recirculation depth exceeded.");
openvswitch-2.9.0/lib/dpif-netdev.c-        break;
openvswitch-2.9.0/lib/dpif-netdev.c-
openvswitch-2.9.0/lib/dpif-netdev.c-    case OVS_ACTION_ATTR_CT: {
openvswitch-2.9.0/lib/dpif-netdev.c-        const struct nlattr *b;
openvswitch-2.9.0/lib/dpif-netdev.c-        bool force = false;
openvswitch-2.9.0/lib/dpif-netdev.c-        bool commit = false;
openvswitch-2.9.0/lib/dpif-netdev.c-        unsigned int left;
openvswitch-2.9.0/lib/dpif-netdev.c-        uint16_t zone = 0;
openvswitch-2.9.0/lib/dpif-netdev.c-        const char *helper = NULL;
openvswitch-2.9.0/lib/dpif-netdev.c-        const uint32_t *setmark = NULL;
--
openvswitch-2.9.0/lib/odp-util.c-            break;
openvswitch-2.9.0/lib/odp-util.c-        default:
openvswitch-2.9.0/lib/odp-util.c-            /* Only the above protocols are supported for encap.
openvswitch-2.9.0/lib/odp-util.c-             * The check is done at action translation. */
openvswitch-2.9.0/lib/odp-util.c-            OVS_NOT_REACHED();
openvswitch-2.9.0/lib/odp-util.c-        }
openvswitch-2.9.0/lib/odp-util.c-    } else {
openvswitch-2.9.0/lib/odp-util.c-        /* This is an explicit or implicit decap case. */
openvswitch-2.9.0/lib/odp-util.c-        if (pt_ns(flow->packet_type) == OFPHTN_ETHERTYPE &&
openvswitch-2.9.0/lib/odp-util.c-            base_flow->packet_type == htonl(PT_ETH)) {
openvswitch-2.9.0/lib/odp-util.c:            /* Generate pop_eth and continue without recirculation. */
openvswitch-2.9.0/lib/odp-util.c-            odp_put_pop_eth_action(odp_actions);
openvswitch-2.9.0/lib/odp-util.c-            base_flow->packet_type = flow->packet_type;
openvswitch-2.9.0/lib/odp-util.c-            base_flow->dl_src = eth_addr_zero;
openvswitch-2.9.0/lib/odp-util.c-            base_flow->dl_dst = eth_addr_zero;
openvswitch-2.9.0/lib/odp-util.c-        } else {
openvswitch-2.9.0/lib/odp-util.c:            /* All other decap cases require recirculation.
openvswitch-2.9.0/lib/odp-util.c-             * No need to update the base flow here. */
openvswitch-2.9.0/lib/odp-util.c-            switch (ntohl(base_flow->packet_type)) {
openvswitch-2.9.0/lib/odp-util.c-            case PT_NSH:
openvswitch-2.9.0/lib/odp-util.c-                /* pop_nsh. */
openvswitch-2.9.0/lib/odp-util.c-                odp_put_pop_nsh_action(odp_actions);
openvswitch-2.9.0/lib/odp-util.c-                break;
openvswitch-2.9.0/lib/odp-util.c-            default:
openvswitch-2.9.0/lib/odp-util.c-                /* Checks are done during translation. */
openvswitch-2.9.0/lib/odp-util.c-                OVS_NOT_REACHED();
openvswitch-2.9.0/lib/odp-util.c-            }
--
openvswitch-2.9.0/ovn/lib/actions.c-            nat->flags |= NX_NAT_F_DST;
openvswitch-2.9.0/ovn/lib/actions.c-        }
openvswitch-2.9.0/ovn/lib/actions.c-    }
openvswitch-2.9.0/ovn/lib/actions.c-
openvswitch-2.9.0/ovn/lib/actions.c-    ofpacts->header = ofpbuf_push_uninit(ofpacts, nat_offset);
openvswitch-2.9.0/ovn/lib/actions.c-    ct = ofpacts->header;
openvswitch-2.9.0/ovn/lib/actions.c-    if (cn->ip) {
openvswitch-2.9.0/ovn/lib/actions.c-        ct->flags |= NX_CT_F_COMMIT;
openvswitch-2.9.0/ovn/lib/actions.c-    } else if (snat && ep->is_gateway_router) {
openvswitch-2.9.0/ovn/lib/actions.c-        /* For performance reasons, we try to prevent additional
openvswitch-2.9.0/ovn/lib/actions.c:         * recirculations.  ct_snat which is used in a gateway router
openvswitch-2.9.0/ovn/lib/actions.c:         * does not need a recirculation.  ct_snat(IP) does need a
openvswitch-2.9.0/ovn/lib/actions.c:         * recirculation.  ct_snat in a distributed router needs
openvswitch-2.9.0/ovn/lib/actions.c:         * recirculation regardless of whether an IP address is
openvswitch-2.9.0/ovn/lib/actions.c-         * specified.
openvswitch-2.9.0/ovn/lib/actions.c-         * XXX Should we consider a method to let the actions specify
openvswitch-2.9.0/ovn/lib/actions.c:         * whether an action needs recirculation if there are more use
openvswitch-2.9.0/ovn/lib/actions.c-         * cases?. */
openvswitch-2.9.0/ovn/lib/actions.c-        ct->recirc_table = NX_CT_RECIRC_NONE;
openvswitch-2.9.0/ovn/lib/actions.c-    }
openvswitch-2.9.0/ovn/lib/actions.c-    ofpact_finish(ofpacts, &ct->ofpact);
openvswitch-2.9.0/ovn/lib/actions.c-    ofpbuf_push_uninit(ofpacts, ct_offset);
openvswitch-2.9.0/ovn/lib/actions.c-}
openvswitch-2.9.0/ovn/lib/actions.c-
openvswitch-2.9.0/ovn/lib/actions.c-static void
openvswitch-2.9.0/ovn/lib/actions.c-encode_CT_DNAT(const struct ovnact_ct_nat *cn,
openvswitch-2.9.0/ovn/lib/actions.c-               const struct ovnact_encode_params *ep,
--
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * later.  We call the checkpointing process "freezing" and the restarting
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * process "thawing".
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * The use cases for freezing are:
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *     - "Recirculation", where the translation process discovers that it
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       doesn't have enough information to complete translation without
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       actually executing the actions that have already been translated,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       which provides the additionally needed information.  In these
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       situations, translation freezes translation and assigns the frozen
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    *       data a unique "recirculation ID", which it associates with the data
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       in a table in userspace (see ofproto-dpif-rid.h).  It also adds a
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       OVS_ACTION_ATTR_RECIRC action specifying that ID to the datapath
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       actions.  When a packet hits that action, the datapath looks its
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       flow up again using the ID.  If there's a miss, it comes back to
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    *       userspace, which find the recirculation table entry for the ID,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       thaws the associated frozen data, and continues translation from
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       that point given the additional information that is now known.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       The archetypal example is MPLS.  As MPLS is implemented in
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       OpenFlow, the protocol that follows the last MPLS label becomes
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       known only when that label is popped by an OpenFlow action.  That
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       means that Open vSwitch can't extract the headers beyond the MPLS
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       labels until the pop action is executed.  Thus, at that point
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    *       translation uses the recirculation process to extract the headers
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       beyond the MPLS labels.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       (OVS also uses OVS_ACTION_ATTR_RECIRC to implement hashing for
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       output to bonds.  OVS pre-populates all the datapath flows for bond
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       output in the datapath, though, which means that the elaborate
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       process of coming back to userspace for a second round of
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       translation isn't needed, and so bonds don't follow the above
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *       process.)
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *     - "Continuation".  A continuation is a way for an OpenFlow controller
--
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * translation process:
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * 1. Sets 'freezing' to true.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * 2. Sets 'exit' to true to tell later steps that we're exiting from the
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    translation process.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * 3. Adds an OFPACT_UNROLL_XLATE action to 'frozen_actions', and points
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    frozen_actions.header to the action to make it easy to find it later.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    This action holds the current table ID and cookie so that they can be
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    *    restored during a post-recirculation upcall translation.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    * 4. Adds the action that prompted recirculation and any actions following
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    it within the same flow to 'frozen_actions', so that they can be
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    *    executed during a post-recirculation upcall translation.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * 5. Returns.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    * 6. The action that prompted recirculation might be nested in a stack of
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    nested "resubmit"s that have actions remaining.  Each of these notices
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    that we're exiting and freezing and responds by adding more
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    OFPACT_UNROLL_XLATE actions to 'frozen_actions', as necessary,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *    followed by any actions that were yet unprocessed.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    *
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    * If we're freezing because of recirculation, the caller generates a
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    * recirculation ID and associates all the state produced by this process
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    * with it.  For post-recirculation upcall translation, the caller passes it
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * back in for the new translation to execute.  The process yielded a set of
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * ofpacts that can be translated directly, so it is not much of a special
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    * case at that point.
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    bool freezing;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    bool recirc_update_dp_hash;    /* Generated recirculation will be preceded
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                    * by datapath HASH action to get an updated
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:                                    * dp_hash after recirculation. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    uint32_t dp_hash_alg;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    uint32_t dp_hash_basis;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    struct ofpbuf frozen_actions;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    const struct ofpact_controller *pause;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    /* True if a packet was but is no longer MPLS (due to an MPLS pop action).
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:     * This is a trigger for recirculation in cases where translating an action
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-     * or looking up a flow requires access to the fields of the packet after
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-     * the MPLS label stack that was originally present. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    bool was_mpls;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    /* True if conntrack has been performed on this packet during processing
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-     * on the current bridge. This is used to determine whether conntrack
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-     * state from the datapath should be honored after thawing. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    bool conntracked;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    /* Pointer to an embedded NAT action in a conntrack action, or NULL. */
--
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static void xvlan_put(struct flow *, const struct xvlan *);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static void xvlan_input_translate(const struct xbundle *,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  const struct xvlan *in,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  struct xvlan *xvlan);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static void xvlan_output_translate(const struct xbundle *,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                   const struct xvlan *xvlan,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                   struct xvlan *out);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static void output_normal(struct xlate_ctx *, const struct xbundle *,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                          const struct xvlan *);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:/* Optional bond recirculation parameter to compose_output_action(). */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-struct xlate_bond_recirc {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:    uint32_t recirc_id;  /* !0 Use recirculation instead of output. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    uint8_t  hash_alg;   /* !0 Compute hash for recirc before. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    uint32_t hash_basis;  /* Compute hash for recirc before. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-};
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static void compose_output_action(struct xlate_ctx *, ofp_port_t ofp_port,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  const struct xlate_bond_recirc *xr,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  bool is_last_action, bool truncate);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-static struct xbridge *xbridge_lookup(struct xlate_cfg *,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                      const struct ofproto_dpif *);
--
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    /* If packet is recirculated, xport can be retrieved from frozen state. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    if (flow->recirc_id) {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        const struct recirc_id_node *recirc_id_node;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        recirc_id_node = recirc_id_node_find(flow->recirc_id);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        if (OVS_UNLIKELY(!recirc_id_node)) {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-            return NULL;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        }
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:        /* If recirculation was initiated due to bond (in_port = OFPP_NONE)
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-         * then frozen state is static and xport_uuid is not defined, so xport
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-         * cannot be restored from frozen state. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        if (recirc_id_node->state.metadata.in_port != OFPP_NONE) {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-            struct uuid xport_uuid = recirc_id_node->state.xport_uuid;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-            xport = xport_lookup_by_uuid(xcfg, &xport_uuid);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-            if (xport && xport->xbridge && xport->xbridge->ofproto) {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                goto out;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-            }
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        }
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    }
--
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    netdev_init_tnl_build_header_params(&tnl_params, flow, &s_ip6, dmac, smac);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    err = tnl_port_build_header(xport->ofport, &tnl_push_data, &tnl_params);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    if (err) {
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-        return err;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    }
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    tnl_push_data.tnl_port = tunnel_odp_port;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    tnl_push_data.out_port = out_dev->odp_port;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    /* After tunnel header has been added, MAC and IP data of flow and
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c:     * base_flow need to be set properly, since there is not recirculation
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-     * any more when sending packet to tunnel. */
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    propagate_tunnel_data_to_flow(ctx, dmac, smac, s_ip6,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  s_ip, tnl_params.is_ipv6,
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-                                  tnl_push_data.tnl_type);
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    size_t clone_ofs = 0;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    size_t push_action_size;
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-
openvswitch-2.9.0/ofproto/ofproto-dpif-xlate.c-    clone_ofs = nl_msg_start_nested(ctx->odp_actions, OVS_ACTION_ATTR_CLONE);
--
penvswitch-2.9.0/ofproto/bond.h-void *bond_choose_output_slave(struct bond *, const struct flow *,
openvswitch-2.9.0/ofproto/bond.h-                               struct flow_wildcards *, uint16_t vlan);
openvswitch-2.9.0/ofproto/bond.h-
openvswitch-2.9.0/ofproto/bond.h-/* Rebalancing. */
openvswitch-2.9.0/ofproto/bond.h-void bond_account(struct bond *, const struct flow *, uint16_t vlan,
openvswitch-2.9.0/ofproto/bond.h-                  uint64_t n_bytes);
openvswitch-2.9.0/ofproto/bond.h-void bond_rebalance(struct bond *);
openvswitch-2.9.0/ofproto/bond.h-
openvswitch-2.9.0/ofproto/bond.h-/* Recirculation
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h: * Only balance_tcp mode uses recirculation.
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h: * When recirculation is used, each bond port is assigned with a unique
openvswitch-2.9.0/ofproto/bond.h- * recirc_id. The output action to the bond port will be replaced by
openvswitch-2.9.0/ofproto/bond.h- * a Hash action, followed by a RECIRC action.
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h- *   ... actions= ... HASH(hash(L4)), RECIRC(recirc_id) ....
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h: * On handling first output packet, 256 post recirculation flows are installed:
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h- *  recirc_id=<bond_recirc_id>, dp_hash=<[0..255]>/0xff, actions: output<slave>
openvswitch-2.9.0/ofproto/bond.h- *
openvswitch-2.9.0/ofproto/bond.h: * Bond module pulls stats from those post recirculation rules. If rebalancing
openvswitch-2.9.0/ofproto/bond.h- * is needed, those rules are updated with new output actions.
openvswitch-2.9.0/ofproto/bond.h-*/
openvswitch-2.9.0/ofproto/bond.h-void bond_update_post_recirc_rules(struct bond *, uint32_t *recirc_id,
openvswitch-2.9.0/ofproto/bond.h-                                   uint32_t *hash_basis);
openvswitch-2.9.0/ofproto/bond.h-#endif /* bond.h */
~~~
