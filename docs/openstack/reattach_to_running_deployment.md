# Reattaching to a running overcloud deployment #

How to re-attach to a running `openstack overcloud deploy` deployment in tripleo / Red Hat OpenStack

One may inadvertently cancel ...
~~~
openstack overcloud deploy (...)
~~~
... with CTRL-C. In order to reattach to the live event list, one can use this handy command.
~~~
openstack stack event list overcloud --follow
~~~
