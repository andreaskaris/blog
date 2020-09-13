# Linux containers #

## What is a Linux container ##

Introductory read about the components that make up a container:

[https://medium.com/@nagarwal/understanding-the-docker-internals-7ccb052ce9fe](https://medium.com/@nagarwal/understanding-the-docker-internals-7ccb052ce9fe)

## Relations ship between containers, cgroups, SELinux and containers ##

`Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems` - [https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index):
~~~
Kernel namespaces ensure process isolation and cgroups are employed to control
the system resources. SELinux is used to assure separation between the host and the container and
also between the individual containers. Management interface forms a higher layer that interacts with
the aforementioned kernel components and provides tools for construction and management of
containers.
~~~

`Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems` - [https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index):
~~~
Namespaces
The kernel provides process isolation by creating separate namespaces for containers. Namespaces
enable creating an abstraction of a particular global system resource and make it appear as a
separated instance to processes within a namespace. Consequently, several containers can use the
same resource simultaneously without creating a conflict.
(...)
Control Groups (cgroups)
The kernel uses cgroups to group processes for the purpose of system resource management.
Cgroups allocate CPU time, system memory, network bandwidth, or combinations of these among user-
defined groups of tasks. In Red Hat Enterprise Linux 7, cgroups are managed with systemd slice, scope,
and service units. For more information on cgroups, see the Red Hat Enterprise Linux 7 Resource
Management Guide.
SELinux
SELinux provides secure separation of containers by applying SELinux policy and labels. It integrates
with virtual devices by using the sVirt technology. For more information see the Red Hat Enterprise
Linux 7 SELinux Users and Administrators Guide.
(...)
~~~

`Red Hat Enterprise Linux Atomis Host 7 Overview of Containers in Red Hat Systems` - [https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_atomic_host/7/html-single/overview_of_containers_in_red_hat_systems/index):
~~~
1.3. SECURE CONTAINERS WITH SELINUX
From the security point of view, there is a need to isolate the host system from a container and to
isolate containers from each other. The kernel features used by containers, namely cgroups and
namespaces, by themselves provide a certain level of security. Cgroups ensure that a single container
cannot exhaust a large amount of system resources, thus preventing some denial-of-service attacks.
By virtue of namespaces, the /dev directory created within a container is private to each container,
and therefore unaffected by the host changes. However, this can not prevent a hostile process from
breaking out of the container since the entire system is not namespaced or containerized. Another
level of separation, provided by SELinux, is therefore needed.
Security-Enhanced Linux (SELinux) is an implementation of a mandatory access control (MAC)
mechanism, multi-level security (MLS), and multi-category security (MCS) in the Linux kernel. The
sVirt project builds upon SELinux and integrates with Libvirt to provide a MAC framework for virtual
machines and containers. This architecture provides a secure separation for containers as it prevents
root processes within the container from interfering with other processes running outside this
container. The containers created with Docker are automatically assigned with an SELinux context
specified in the SELinux policy.
5Red Hat Enterprise Linux Atomic Host 7 Overview of Containers in Red Hat Systems
By default, containers created with libvirt tools are assigned with the virtd_lxc_t label (execute ps
-eZ | grep virtd_lxc_t). You can apply sVirt by setting static or dynamic labeling for processes
inside the container.
Note
You might notice that SELinux appears to be disabled inside the container even though it is running in
enforcing mode on host system – you can verify this by executing the getenforce command on host
and in the container. This is to prevent utilities that have SELinux awareness, such as setenforce, to
perform any SELinux activity inside the container.
Note that if SELinux is disabled or running in permissive mode on the host machine, containers are not
separated securely enough. For more information about SELinux, refer to Red Hat Enterprise Linux 7
SELinux Users and Administrators Guide, sVirt is described in Red Hat Enterprise Linux 7 Virtualization
Security Guide.
~~~
