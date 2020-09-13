# How to run old Java with xorgs in a container 

Also see reference: [https://adam.younglogic.com/2017/01/gui-applications-container/](https://adam.younglogic.com/2017/01/gui-applications-container/)

Make sure that you're running xorgs and not wayland:
[https://docs.fedoraproject.org/en-US/quick-docs/configuring-xorg-as-default-gnome-session/](https://docs.fedoraproject.org/en-US/quick-docs/configuring-xorg-as-default-gnome-session/)
~~~
Procedure

    Open /etc/gdm/custom.conf and uncomment WaylandEnable=false.

    Add the following line to the [daemon] section:

    DefaultSession=gnome-xorg.desktop

    Save the custom.conf file.
~~~

Build container with buildah:
~~~
buildah from --name java-container fedora:26
buildah run java-container -- yum install xclock icedtea-web -y
buildah commit java-container java-image
~~~

Disable selinux:
~~~
sudo setenforce 0
~~~

Test container:
~~~
podman run -ti -e DISPLAY --rm -v /run/user/1000/gdm/Xauthority:/run/user/0/gdm/Xauthority:Z --net=host localhost/java-image javaws xclock
~~~

If this does not work, check `journalctl -f`. I got:
~~~
May 19 13:48:48 linux audit[104503]: AVC avc:  denied  { connectto } for  pid=104503 comm="xclock" path=002F746D702F2E5831312D756E69782F5831 scontext=system_u:system_r:container_t:s0:c616,c783 tcontext=unconfined_u:unconfined_r:xserver_t:s0-s0:c0.c1023 tclass=unix_stream_socket permissive=
~~~

In order to work around this:
~~~
echo "(allow container_t xserver_t (unix_stream_socket (connectto)))" > mycontainer.cil
sudo semodule -i mycontainer.ci
~~~

Once xclock works, save viewer.jnlp (from the iDrac) to /tmp/viewer.jnlp

Exit container and run jviewer:
~~~
podman run -ti -e DISPLAY --rm -v /run/user/1000/gdm/Xauthority:/run/user/0/gdm/Xauthority:Z --net=host -v /tmp/viewer.jnlp:/root/viewer.jnlp localhost/java-image javaws /root/viewer.jnlp
~~~
