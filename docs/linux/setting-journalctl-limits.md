## Changing the size of data that journald retains

The systemd journal by default retains 4GB of data. In order to increase or decrease that value, set `SystemMaxUse` and
if needed set `SystemKeepFree` which will be the upper bound of storage that will be kept free on the drive. You set
these values in file `/etc/systemd/journald.conf` under the `[Journal]` section.

For example, to increase the journal size to 40GB but to make sure that the system has 100GB of free disk space:
~~~
# /etc/systemd/journald.conf
[Journal]
SystemMaxUse=40G 
SystemKeepFree=100G
~~~

After changing the settings, restart the journal service:
~~~
systemctl restart systemd-journald.service
~~~

That means that logs will grow to 40G in size, but only if at least 100GB of disk space are free on the target file system.

~~~
man journald.conf
(...)
       SystemMaxUse=, SystemKeepFree=, SystemMaxFileSize=, SystemMaxFiles=, RuntimeMaxUse=, RuntimeKeepFree=,
       RuntimeMaxFileSize=, RuntimeMaxFiles=
           Enforce size limits on the journal files stored. The options prefixed with "System" apply to the
           journal files when stored on a persistent file system, more specifically /var/log/journal. The options
           prefixed with "Runtime" apply to the journal files when stored on a volatile in-memory file system,
           more specifically /run/log/journal. The former is used only when /var is mounted, writable, and the
           directory /var/log/journal exists. Otherwise, only the latter applies. Note that this means that during
           early boot and if the administrator disabled persistent logging, only the latter options apply, while
           the former apply if persistent logging is enabled and the system is fully booted up.  journalctl and
           systemd-journald ignore all files with names not ending with ".journal" or ".journal~", so only such
           files, located in the appropriate directories, are taken into account when calculating current disk
           usage.

           SystemMaxUse= and RuntimeMaxUse= control how much disk space the journal may use up at most.
           SystemKeepFree= and RuntimeKeepFree= control how much disk space systemd-journald shall leave free for
           other uses.  systemd-journald will respect both limits and use the smaller of the two values.

           The first pair defaults to 10% and the second to 15% of the size of the respective file system, but
           each value is capped to 4G. If the file system is nearly full and either SystemKeepFree= or
           RuntimeKeepFree= are violated when systemd-journald is started, the limit will be raised to the
           percentage that is actually free. This means that if there was enough free space before and journal
           files were created, and subsequently something else causes the file system to fill up, journald will
           stop using more space, but it will not be removing existing files to reduce the footprint again,
           either. Also note that only archived files are deleted to reduce the space occupied by journal files.
           This means that, in effect, there might still be more space used than SystemMaxUse= or RuntimeMaxUse=
           limit after a vacuuming operation is complete.
(...)
~~~

### Test case

In order to test these settings, one can disable the journal's rate limiting feature with `RateLimitIntervalSec` and `RateLimitBurst` and set a low `SystemMaxUse`:
~~~
# egrep -v '^#' /etc/systemd/journald.conf 

[Journal]
RateLimitIntervalSec=0
RateLimitBurst=0
SystemMaxUse=2G
SystemKeepFree=100G
~~~

Restart the service:
~~~
systemctl restart systemd-journald.service
~~~


Generate lots of logging quickly, for example with:
~~~
while true; do dd if=/dev/urandom bs=3 count=10000 | base64 | logger; done
~~~

Now, verify while running the above command. You will see that the start of the log moves, so earlier data will be list:
~~~
# journalctl | head -n 3
-- Logs begin at Sat 2021-04-10 13:00:43 UTC, end at Thu 2021-04-22 15:14:54 UTC. --
Apr 10 13:00:43 node-0 NetworkManager[1962]: <info>  [1618059643.6773] dhcp4 (enp4s0f1): canceled DHCP transaction
Apr 10 13:00:43 node-0 NetworkManager[1962]: <info>  [1618059643.6774] dhcp4 (enp4s0f1): state changed timeout -> done
~~~

And the size of `/var/log/journal` will fluctuate as new data is written to the logs and old data is erased:
~~~
[root@node-0 ~]# while true; do du -s /var/log/journal/ ; sleep 5 ;done
2064492	/var/log/journal/
2113640	/var/log/journal/
2097252	/var/log/journal/
2138212	/var/log/journal/
^C
[root@node-0 ~]# journalctl | head -n 3
-- Logs begin at Tue 2021-04-13 17:31:58 UTC, end at Thu 2021-04-22 15:16:04 UTC. --
Apr 13 17:31:58 node-0 NetworkManager[1962]: <info>  [1618335118.6725] policy: auto-activating connection 'Wired connection 7' (d434db97-6ad2-3255-adfd-b2826bcb31a9)
Apr 13 17:31:58 node-0 NetworkManager[1962]: <info>  [1618335118.6735] device (enp4s0f0): Activation: starting connection 'Wired connection 4' (f0a93efd-0bb7-36f6-9630-b7eb53a4f591)
~~~


