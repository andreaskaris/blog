# SELinux Cheat Sheet

### Reading SELinux status

| Command  | Description   | Examples |
|---|---|---|
| cat /etc/selinux/config | Get SELinux boot configuration | # cat /etc/selinux/config <br /><br /># This file controls the state of SELinux on the system.<br /># SELINUX= can take one of these three values:<br />#     enforcing - SELinux security policy is enforced.<br />#     permissive - SELinux prints warnings instead of enforcing.<br />#     disabled - No SELinux policy is loaded.<br /># See also:<br /># https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/using_selinux/changing-selinux-states-and-modes_using-selinux#changing-selinux-modes-at-boot-time_changing-selinux-states-and-modes<br />#<br /># NOTE: Up to RHEL 8 release included, SELINUX=disabled would also<br /># fully disable SELinux during boot. If you need a system with SELinux<br /># fully disabled instead of SELinux running with no policy loaded, you<br /># need to pass selinux=0 to the kernel command line. You can use grubby<br /># to persistently set the bootloader to boot with selinux=0:<br />#<br />#    grubby --update-kernel ALL --args selinux=0<br />#<br /># To revert back to SELinux enabled:<br />#<br />#    grubby --update-kernel ALL --remove-args selinux<br />#<br />SELINUX=enforcing<br /># SELINUXTYPE= can take one of these three values:<br />#     targeted - Targeted processes are protected,<br />#     minimum - Modification of targeted policy. Only selected processes are protected.<br />#     mls - Multi Level Security protection.<br />SELINUXTYPE=targeted |
| cat /proc/cmdline | Check if SELinux is disabled on boot | # cat /proc/cmdline<br />BOOT_IMAGE=(hd0,gpt3)/vmlinuz-5.14.0-427.26.1.el9_4.x86_64 root=UUID=aec1c1e8-3576-4eb2-ab62-f62984e655a2 console=tty0 console=ttyS0,115200n8 no_timer_check net.ifnames=0 crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M selinux=0 |
| getenforce | Get current enforcement status | # getenforce<br />Enforcing |
| sestatus | Get current SELinux status | # sestatus<br />SELinux status:                 enabled<br />SELinuxfs mount:                /sys/fs/selinux<br />SELinux root directory:         /etc/selinux<br />Loaded policy name:             targeted<br />Current mode:                   enforcing<br />Mode from config file:          enforcing<br />Policy MLS status:              enabled<br />Policy deny_unknown status:     allowed<br />Memory protection checking:     actual (secure)<br />Max kernel policy version:      33 |

### Enabling / disabling SELinux

| Command  | Description   | Examples |
|---|---|---|
| setenforce | Set current enforcement status | # setenforce Enforcing |
| grubby --update-kernel ALL --args selinux=0 | Disable SELinux permanently starting with next boot | |
| grubby --update-kernel ALL --remove-args selinux | Enable SELinux permanently starting with next boot (if if was disabled) | |

### Finding SELinux violations

| Command  | Description   | Examples |
|---|---|---|
| journalctl -t setroubleshoot | Get journal logs for SELinux issues | Jul 19 07:09:41 rhel9-training setroubleshoot[47738]: SELinux is preventing /usr/lib/systemd/systemd from execute access on the file cause-violation. For co><br />Jul 19 07:09:41 rhel9-training setroubleshoot[47738]: SELinux is preventing /usr/lib/systemd/systemd from execute access on the file cause-violation.<br />                                                      <br />                                                      *****  Plugin catchall (100. confidence) suggests   **************************<br />                                                      <br />                                                      If you believe that systemd should be allowed execute access on the cause-violation file by default.<br />                                                      Then you should report this as a bug.<br />                                                      You can generate a local policy module to allow this access.<br />                                                      Do<br />                                                      allow this access for now by executing:<br />                                                      # ausearch -c '(iolation)' --raw | audit2allow -M my-iolation<br />                                                      # semodule -X 300 -i my-iolation.pp<br /> |
| grep denied /var/log/audit/audit.log | Print raw SELinux denials | # grep denied /var/log/audit/audit.log<br \>type=AVC msg=audit(1703120795.400:102): avc:  denied  { execute } for  pid=1586 comm="(iolation)" name="cause-violation" dev="vda4" ino=58946753 scontext=system_u:system_r:init_t:s0 tcontext=system_u:object_r:httpd_sys_content_t:s0 tclass=file permissive=0 |
| ausearch -m AVC,USER_AVC,SELINUX_ERR,USER_SELINUX_ERR -ts recent | Find recent denials | # ausearch -m AVC,USER_AVC,SELINUX_ERR,USER_SELINUX_ERR -ts recent<br />----<br />time->Fri Jul 19 07:29:17 2024<br />type=PROCTITLE msg=audit(1721388557.563:621): proctitle="(iolation)"<br />type=SYSCALL msg=audit(1721388557.563:621): arch=c000003e syscall=21 success=no exit=-13 a0=7fff382a8ae0 a1=1 a2=0 a3=3 items=0 ppid=1 pid=47806 auid=4294967295 uid=0 gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 tty=(none) ses=4294967295 comm="(iolation)" exe="/usr/lib/systemd/systemd" subj=system_u:system_r:init_t:s0 key=(null)<br />type=AVC msg=audit(1721388557.563:621): avc:  denied  { execute } for  pid=47806 comm="(iolation)" name="cause-violation" dev="vda4" ino=58946753 scontext=system_u:system_r:init_t:s0 tcontext=system_u:object_r:httpd_sys_content_t:s0 tclass=file permissive=0 |
| ausearch -m AVC,USER_AVC,SELINUX_ERR,USER_SELINUX_ERR -ts today | Find denials for today | # ausearch -m AVC,USER_AVC,SELINUX_ERR,USER_SELINUX_ERR -ts today<br />----<br />time->Fri Jul 19 07:29:17 2024<br />type=PROCTITLE msg=audit(1721388557.563:621): proctitle="(iolation)"<br />type=SYSCALL msg=audit(1721388557.563:621): arch=c000003e syscall=21 success=no exit=-13 a0=7fff382a8ae0 a1=1 a2=0 a3=3 items=0 ppid=1 pid=47806 auid=4294967295 uid=0 gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 tty=(none) ses=4294967295 comm="(iolation)" exe="/usr/lib/systemd/systemd" subj=system_u:system_r:init_t:s0 key=(null)<br />type=AVC msg=audit(1721388557.563:621): avc:  denied  { execute } for  pid=47806 comm="(iolation)" name="cause-violation" dev="vda4" ino=58946753 scontext=system_u:system_r:init_t:s0 tcontext=system_u:object_r:httpd_sys_content_t:s0 tclass=file permissive=0 |
| sealert -l "*" | sealert -a /var/log/audit/audit.log | setroubleshoot client tool | SELinux is preventing /usr/bin/bash from 'read, open' accesses on the file /opt/tutorial/bin/cause-violation.<br /><br />*****  Plugin catchall (100. confidence) suggests   **************************<br /><br />If you believe that bash should be allowed read open access on the cause-violation file by default.<br />Then you should report this as a bug.<br />You can generate a local policy module to allow this access.<br />Do<br />allow this access for now by executing:<br /># ausearch -c 'cause-violation' --raw | audit2allow -M my-causeviolation<br /># semodule -X 300 -i my-causeviolation.pp<br /><br /><br />Additional Information:<br />Source Context                system_u:system_r:init_t:s0<br />Target Context                system_u:object_r:httpd_sys_content_t:s0<br />Target Objects                /opt/tutorial/bin/cause-violation [ file ]<br />Source                        cause-violation<br />Source Path                   /usr/bin/bash<br />Port                          <Unknown><br />Host                          rhel9-training<br />Source RPM Packages           bash-5.1.8-6.el9_1.x86_64<br />Target RPM Packages           <br />SELinux Policy RPM            selinux-policy-targeted-38.1.23-1.el9.noarch<br />Local Policy RPM              selinux-policy-targeted-38.1.23-1.el9.noarch<br />Selinux Enabled               True<br />Policy Type                   targeted<br />Enforcing Mode                Permissive<br />Host Name                     rhel9-training<br />Platform                      Linux rhel9-training 5.14.0-362.8.1.el9_3.x86_64<br />                              #1 SMP PREEMPT_DYNAMIC Tue Oct 3 11:12:36 EDT 2023<br />                              x86_64 x86_64<br />Alert Count                   1<br />First Seen                    2023-12-20 20:02:11 EST<br />Last Seen                     2023-12-20 20:02:11 EST<br />Local ID                      b418bb7b-9d58-420d-bd63-0ee50c648483<br /><br />Raw Audit Messages<br />type=AVC msg=audit(1703120531.172:127): avc:  denied  { read open } for  pid=1666 comm="(iolation)" path="/opt/tutorial/bin/cause-violation" dev="vda4" ino=58946753 scontext=system_u:system_r:init_t:s0 tcontext=system_u:object_r:httpd_sys_content_t:s0 tclass=file permissive=1<br /><br /><br />type=AVC msg=audit(1703120531.172:127): avc:  denied  { execute_no_trans } for  pid=1666 comm="(iolation)" path="/opt/tutorial/bin/cause-violation" dev="vda4" ino=58946753 scontext=system_u:system_r:init_t:s0 tcontext=system_u:object_r:httpd_sys_content_t:s0 tclass=file permissive=1<br /><br /><br />type=SYSCALL msg=audit(1703120531.172:127): arch=x86_64 syscall=execve success=yes exit=0 a0=55ac11a37eb0 a1=55ac119e85a0 a2=55ac11a35920 a3=7fdb5e43e93b items=2 ppid=1 pid=1666 auid=4294967295 uid=0 gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 tty=(none) ses=4294967295 comm=cause-violation exe=/usr/bin/bash subj=system_u:system_r:init_t:s0 key=(null)<br /><br />type=CWD msg=audit(1703120531.172:127): cwd=/<br /><br />type=PATH msg=audit(1703120531.172:127): item=0 name=/bin/bash inode=8399459 dev=fc:04 mode=0100755 ouid=0 ogid=0 rdev=00:00 obj=system_u:object_r:shell_exec_t:s0 nametype=NORMAL cap_fp=0 cap_fi=0 cap_fe=0 cap_fver=0 cap_frootid=0<br /><br />type=PATH msg=audit(1703120531.172:127): item=1 name=/lib64/ld-linux-x86-64.so.2 inode=8400834 dev=fc:04 mode=0100755 ouid=0 ogid=0 rdev=00:00 obj=system_u:object_r:ld_so_t:s0 nametype=NORMAL cap_fp=0 cap_fi=0 cap_fe=0 cap_fver=0 cap_frootid=0<br /><br />Hash: cause-violation,init_t,httpd_sys_content_t,file,read,open |

### Listing SELinux labels

| Command  | Description   | Examples |
|---|---|---|
| seinfo -b | List all available SELinux booleans | |
| seinfo -r | List all available SELinux rules | |
| seinfo -t | List all available SELinux types | |
| seinfo -u | List all available SELinux users | |

### Process operations

| Command  | Description   | Examples |
|---|---|---|
| ps aux -Z | List all processes and the SELinux label that they run with | |
| ps -fZ --pid $(pgrep -f <process name>) | Show the current label of a process | # ps -fZ --pid $(pgrep -f open-messages)<br />LABEL                           UID          PID    PPID  C STIME TTY          TIME CMD<br />system_u:system_r:unconfined_service_t:s0 root 2236    1  0 08:43 ?        00:00:00 python /opt/tutorial/bin/open-messages |

### File operations

| Command  | Description   | Examples |                                                                                 
|---|---|---|  
| ls -al -Z | List all files in this directory and their SELinux label | |
| chron -R -t tmp_t /test | Change SELinux type temporarily for the directory, recursively | |
| restorecon -v -R /test | Reset labels to default recursively in this directory | |
| touch /.autorelabel | Force an auto relabel on system boot | |
| semanage fcontext -l | List default label configuration for the entire system | |
|  semanage fcontext -C -l | List all local customizations to label configuration | #  semanage fcontext -C -l<br />SELinux fcontext                                   type               Context<br /><br />/opt/tutorial/bin/cause-violation                  all files          system_u:object_r:httpd_sys_content_t:s0 |
| semanage fcontext -a -t etc_t '/test(/.*)?' | Configure default label for a directory | |
| semanage fcontext -d '/test(/.*)?' | Delete default label configuration for a directory | |

### SELinux booleans

| Command  | Description   | Examples |                                                                                 
|---|---|---| 
| getsebool -a | List all SELinux booleans (effective state) | |
| getsebool httpd_use_nfs | List a specific SELinux boolean (effective state) | |
| semanage boolean -l | List all SELinux booleans (effective and permanent) | |
| cat  /sys/fs/selinux/booleans/httpd_use_nfs | Query effective and permanent state of single variable from /sys | # cat  /sys/fs/selinux/booleans/httpd_use_nfs<br />1 1 |
| setsebool httpd_use_nfs on | Set SELinux boolean temporarily | |
| semanage boolean -m --on httpd_use_nfs | Set SELinux boolean permanently (effective on next reboot) | |

### SELinux ports

| Command  | Description   | Examples |                                                                                 
|---|---|---| 
| semanage port -l | List all port mappings (type label to allowed port) | # semanage port -l | head -n 4<br />SELinux Port Type              Proto    Port Number<br /><br />afs3_callback_port_t           tcp      7001<br />afs3_callback_port_t           udp      7001 |

### Creating custom SELinux types and policies

#### Creating a custom SELinux type

Create a policy for a binary:

```
# sepolicy generate --init /opt/tutorial/bin/open-messages
nm: /opt/tutorial/bin/open-messages: file format not recognized
Created the following files:
/root/open_messages.te # Type Enforcement file
/root/open_messages.if # Interface file
/root/open_messages.fc # File Contexts file
/root/open_messages_selinux.spec # Spec file
/root/open_messages.sh # Setup Script
```

By default, the new policy will be permissive, meaning that issues will be logged but not enforced:

```
# cat /root/open_messages.te
policy_module(open_messages, 1.0.0)

########################################
#
# Declarations
#

type open_messages_t;
type open_messages_exec_t;
init_daemon_domain(open_messages_t, open_messages_exec_t)

permissive open_messages_t;

########################################
#
# open_messages local policy
#
allow open_messages_t self:fifo_file rw_fifo_file_perms;
allow open_messages_t self:unix_stream_socket create_stream_socket_perms;

domain_use_interactive_fds(open_messages_t)

files_read_etc_files(open_messages_t)

miscfiles_read_localization(open_messages_t)
```

Remove the permissive line from `open_messages.te` to enforce rules:

```
# sed -i 's/^permissive open_messages_t;$/#permissive open_messages_t;/' open_messages.te
```

Rebuild the synstem policy with the new configuration (make sure to check the output as this may fail with missing dependencies):
```
# ./open_messages.sh
Building and Loading Policy
+ make -f /usr/share/selinux/devel/Makefile open_messages.pp
make: 'open_messages.pp' is up to date.
+ /usr/sbin/semodule -i open_messages.pp
+ sepolicy manpage -p . -d open_messages_t
./open_messages_selinux.8
+ /sbin/restorecon -F -R -v /opt/tutorial/bin/open-messages
++ pwd
+ pwd=/root
+ rpmbuild --define '_sourcedir /root' --define '_specdir /root' --define '_builddir /root' --define '_srcrpmdir /root' --define '_rpmdir /root' --define '_buildrootdir /root/.build' -ba open_messages_selinux.spec
setting SOURCE_DATE_EPOCH=1721347200
Executing(%install): /bin/sh -e /var/tmp/rpm-tmp.a13gQC
+ umask 022
+ cd /root
+ '[' /root/.build/open_messages_selinux-1.0-1.el9.x86_64 '!=' / ']'
+ rm -rf /root/.build/open_messages_selinux-1.0-1.el9.x86_64
++ dirname /root/.build/open_messages_selinux-1.0-1.el9.x86_64
+ mkdir -p /root/.build
+ mkdir /root/.build/open_messages_selinux-1.0-1.el9.x86_64
+ install -d /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/selinux/packages
+ install -m 644 /root/open_messages.pp /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/selinux/packages
+ install -d /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/selinux/devel/include/contrib
+ install -m 644 /root/open_messages.if /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/selinux/devel/include/contrib/
+ install -d /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/man/man8/
+ install -m 644 /root/open_messages_selinux.8 /root/.build/open_messages_selinux-1.0-1.el9.x86_64/usr/share/man/man8/open_messages_selinux.8
+ install -d /root/.build/open_messages_selinux-1.0-1.el9.x86_64/etc/selinux/targeted/contexts/users/
+ /usr/lib/rpm/check-buildroot
+ /usr/lib/rpm/redhat/brp-ldconfig
+ /usr/lib/rpm/brp-compress
+ /usr/lib/rpm/brp-strip /usr/bin/strip
+ /usr/lib/rpm/brp-strip-comment-note /usr/bin/strip /usr/bin/objdump
+ /usr/lib/rpm/redhat/brp-strip-lto /usr/bin/strip
+ /usr/lib/rpm/brp-strip-static-archive /usr/bin/strip
+ /usr/lib/rpm/redhat/brp-python-bytecompile '' 1 0
+ /usr/lib/rpm/brp-python-hardlink
+ /usr/lib/rpm/redhat/brp-mangle-shebangs
Processing files: open_messages_selinux-1.0-1.el9.noarch
Provides: open_messages_selinux = 1.0-1.el9
Requires(interp): /bin/sh /bin/sh
Requires(rpmlib): rpmlib(CompressedFileNames) <= 3.0.4-1 rpmlib(FileDigests) <= 4.6.0-1 rpmlib(PayloadFilesHavePrefix) <= 4.0-1
Requires(post): /bin/sh policycoreutils-python-utils selinux-policy-base >= 38.1.35-2
Requires(postun): /bin/sh policycoreutils-python-utils
Checking for unpackaged file(s): /usr/lib/rpm/check-files /root/.build/open_messages_selinux-1.0-1.el9.x86_64
Wrote: /root/open_messages_selinux-1.0-1.el9.src.rpm
Wrote: /root/noarch/open_messages_selinux-1.0-1.el9.noarch.rpm
Executing(%clean): /bin/sh -e /var/tmp/rpm-tmp.K63ODh
+ umask 022
+ cd /root
+ /usr/bin/rm -rf /root/.build/open_messages_selinux-1.0-1.el9.x86_64
+ RPM_EC=0
++ jobs -p
+ exit 0
```

Verify that the binary now is labeled with the new custom type:

```
# ls -Z /opt/tutorial/bin/open-messages
system_u:object_r:open_messages_exec_t:s0 /opt/tutorial/bin/open-messages
```

#### Creating policies for the custom type

Rerun the service, but with permissive rules:

```
# sed -i 's/^#permissive open_messages_t;$/permissive open_messages_t;/' open_messages.te
# ./open_messages.sh
```

Restart the service and make sure that it's running:
```
# systemctl restart open-messages
# systemctl status open-messages
```

Generate the list of rules needed to make the service run:
```
# ausearch -m AVC -ts recent | audit2allow -R

require {
	type open_messages_t;
}

#============= open_messages_t ==============
auth_read_passwd_file(open_messages_t)
corecmd_exec_bin(open_messages_t)
corecmd_mmap_bin_files(open_messages_t)
files_manage_generic_tmp_files(open_messages_t)
insights_client_filetrans_tmp(open_messages_t)
logging_read_generic_logs(open_messages_t)
sssd_read_public_files(open_messages_t)
sssd_search_lib(open_messages_t)
```

Copy the list of rules from the output and append them to open_messages.te:

```
cat <<'EOF' >> open_messages.te
auth_read_passwd_file(open_messages_t)
corecmd_exec_bin(open_messages_t)
corecmd_mmap_bin_files(open_messages_t)
files_manage_generic_tmp_files(open_messages_t)
insights_client_filetrans_tmp(open_messages_t)
logging_read_generic_logs(open_messages_t)
sssd_read_public_files(open_messages_t)
sssd_search_lib(open_messages_t)
EOF
```

And enforce SELinux again for the policy and rebuild the policies:

```
# sed -i 's/^permissive open_messages_t;$/#permissive open_messages_t;/' open_messages.te
# ./open_messages.sh
```

Empty /var/log/audit/audit.log:

```
# > /var/log/audit/audit.log
```

Start the open-messages service and make sure that it's running correctly:

```
# systemctl restart open-messages
# systemctl status open-messages
```

Inspect the service, you will see that it runs with the new label:

```
# ps -fZ --pid $(pgrep -f open-messages)
LABEL                           UID          PID    PPID  C STIME TTY          TIME CMD
system_u:system_r:open_messages_t:s0 root   4185       1  0 08:59 ?        00:00:00 python /opt/tutorial/bin/open-messages
```

Check that there are no SELinux denied messages for open-messages:

```
# grep denied /var/log/audit/audit.log | grep open_messages_t
```
