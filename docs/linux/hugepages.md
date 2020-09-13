# Hugepages #

## Requesting hugepages in C ##

### Example 1 ###

Create the code, `mmap.c`:
~~~
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE (unsigned int) 1024*1024*1024
#define NUM_PAGES 2

void main() {
	char * buf = mmap(
		NULL, 
		NUM_PAGES * PAGE_SIZE,
		PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
		-1, 
		0
	); 
  	if (buf == MAP_FAILED) {
    		perror("mmap");
    		exit(1);
  	}

	char * line = NULL;
	size_t size;

	printf("Memory address %p\n"
               "This will only reserve pages. Execute \n"
	       "grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/\n"
               "find /sys -name meminfo | xargs grep -i huge\n"
	       "to verify this.\n\n"
	       "When you are done, please hit return\n", buf);
        getline(&line,&size,stdin);
        int i;
	printf("Now, actually writing all 0s into the first hugepage\n");
        for(i = 0; i < PAGE_SIZE; i++) {
		buf[i] = '0';
	}
        printf("Now, verify again\n"
	       "grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/\n"
               "find /sys -name meminfo | xargs grep -i huge\n"
	       "to verify this.\n\n"
	       "When you are done, please hit return\n", buf);
        getline(&line,&size,stdin);

	printf("Now, actually writing one 0 into the second hugepage\n");
	for(; i < PAGE_SIZE + 1; i++) {
                buf[i] = '0';
        }
        printf("Now, verify again\n"
	       "grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/\n"
               "find /sys -name meminfo | xargs grep -i huge\n"
	       "to verify this.\n\n"
	       "When you are done, please hit return to end the program\n", buf);
        getline(&line,&size,stdin);
}
~~~

Compile this:
~~~
gcc mmap.c  -o mmap
~~~

Run the binary and follow the onscreen instructions:
~~~
[root@dell-r430-30 ~]# ./mmap 
Memory address 0x2aaac0000000
This will only reserve pages. Execute 
grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
find /sys -name meminfo | xargs grep -i huge
to verify this.

When you are done, please hit return
Now, actually writing all 0s into the first hugepage
^C
[root@dell-r430-30 ~]# ^C
[root@dell-r430-30 ~]# ^C
[root@dell-r430-30 ~]# gcc mmap.c  -o mmap
[root@dell-r430-30 ~]# ./mmap 
Memory address 0x2aaac0000000
This will only reserve pages. Execute 
grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
find /sys -name meminfo | xargs grep -i huge
to verify this.

When you are done, please hit return

Now, actually writing all 0s into the first hugepage
Now, verify again
grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
find /sys -name meminfo | xargs grep -i huge
to verify this.

When you are done, please hit return

Now, actually writing one 0 into the second hugepage
Now, verify again
grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
find /sys -name meminfo | xargs grep -i huge
to verify this.

When you are done, please hit return to end the program
~~~

Verify reserved hugepages and actually allocated hugepages. 
Also note that writing 1GB to the hugepage actually takes quite some time:
~~~
[root@dell-r430-30 ~]# grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_overcommit_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages_mempolicy:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/surplus_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/resv_hugepages:2
/sys/kernel/mm/hugepages/hugepages-1048576kB/free_hugepages:32
[root@dell-r430-30 ~]# find /sys -name meminfo | xargs grep -i huge
/sys/devices/system/node/node0/meminfo:Node 0 AnonHugePages:         0 kB
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Total:    16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Free:     16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Surp:      0
/sys/devices/system/node/node1/meminfo:Node 1 AnonHugePages:      6144 kB
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Total:    16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Free:     16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Surp:      0
[root@dell-r430-30 ~]# grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_overcommit_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages_mempolicy:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/surplus_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/resv_hugepages:1
/sys/kernel/mm/hugepages/hugepages-1048576kB/free_hugepages:31
[root@dell-r430-30 ~]# find /sys -name meminfo | xargs grep -i huge
/sys/devices/system/node/node0/meminfo:Node 0 AnonHugePages:         0 kB
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Total:    16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Free:     15
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Surp:      0
/sys/devices/system/node/node1/meminfo:Node 1 AnonHugePages:      6144 kB
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Total:    16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Free:     16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Surp:      0
[root@dell-r430-30 ~]# grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_overcommit_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages_mempolicy:32
/sys/kernel/mm/hugepages/hugepages-1048576kB/surplus_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/resv_hugepages:0
/sys/kernel/mm/hugepages/hugepages-1048576kB/free_hugepages:30
[root@dell-r430-30 ~]# find /sys -name meminfo | xargs grep -i huge
/sys/devices/system/node/node0/meminfo:Node 0 AnonHugePages:         0 kB
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Total:    16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Free:     14
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Surp:      0
/sys/devices/system/node/node1/meminfo:Node 1 AnonHugePages:      6144 kB
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Total:    16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Free:     16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Surp:      0
[root@dell-r430-30 ~]# find /sys -name meminfo | xargs grep -i huge
/sys/devices/system/node/node0/meminfo:Node 0 AnonHugePages:         0 kB
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Total:    16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Free:     16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Surp:      0
/sys/devices/system/node/node1/meminfo:Node 1 AnonHugePages:      6144 kB
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Total:    16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Free:     16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Surp:      0
~~~

### Example 2 - Allocating hugepages right away ###

Instead of having to write to the hugepage to allocate it, we can populate the page right away and it will show up in used pages right away:
~~~
man mmap
(...)
       MAP_POPULATE (since Linux 2.5.46)
              Populate (prefault) page tables for a mapping.  For a file mapping, this  causes  read-ahead  on  the
              file.   Later  accesses to the mapping will not be blocked by page faults.  MAP_POPULATE is supported
              for private mappings only since Linux 2.6.23.
(...)
~~~

The application is `mmap2.c`:
~~~
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE (unsigned int) 1024*1024*1024
#define NUM_PAGES 2

void main() {
	char * buf = mmap(
		NULL, 
		NUM_PAGES * PAGE_SIZE,
		PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE,
		-1, 
		0
	); 
  	if (buf == MAP_FAILED) {
    		perror("mmap");
    		exit(1);
  	}

	char * line = NULL;
	size_t size;

	printf("Memory address %p\n"
               "This will only reserve and populate pages. Execute \n"
	       "grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/\n"
               "find /sys -name meminfo | xargs grep -i huge\n"
	       "to verify this.\n\n"
	       "When you are done, please hit return\n", buf);
        getline(&line,&size,stdin);
}
~~~

Testing:
~~~
[root@dell-r430-30 ~]# gcc mmap2.c -o mmap2
[root@dell-r430-30 ~]# ./mmap2 
Memory address 0x2aaac0000000
This will only reserve and populate pages. Execute 
grep -R '' /sys/kernel/mm/hugepages/hugepages-1048576kB/
find /sys -name meminfo | xargs grep -i huge
to verify this.

When you are done, please hit return

~~~

~~~
[root@dell-r430-30 ~]# find /sys -name meminfo | xargs grep -i huge
/sys/devices/system/node/node0/meminfo:Node 0 AnonHugePages:         0 kB
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Total:    16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Free:     16
/sys/devices/system/node/node0/meminfo:Node 0 HugePages_Surp:      0
/sys/devices/system/node/node1/meminfo:Node 1 AnonHugePages:      6144 kB
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Total:    16
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Free:     14
/sys/devices/system/node/node1/meminfo:Node 1 HugePages_Surp:      0
[root@dell-r430-30 ~]# 
~~~

## Sharing hugepages between processes ##

In vhost_user, OVS-DPDK and qemu-kvm instances share the same hugepages for DMA copies. [https://access.redhat.com/solutions/3394851](https://access.redhat.com/solutions/3394851). Let's emulate this with 2 sample applications.

[http://nuncaalaprimera.com/2014/using-hugepage-backed-buffers-in-linux-kernel-driver](http://nuncaalaprimera.com/2014/using-hugepage-backed-buffers-in-linux-kernel-driver)

#### IPC (Inter Process Communication) ###

Let's start with IPC (Inter Process Communication) via Linux sockets as we'll have to communicate the location of the shared memory from the server process to the client process.

`ipc.c`:
~~~
/*
 *
 * This heavily borrows from examples in 
 * "The Linux Programming Interface"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SOCKNAME "/tmp/unix.socket"
#define HUGEPAGE_FILE_NAME "/dev/hugepages/server"
#define NUM_PAGES 2
#define PAGE_SIZE (unsigned long) 1024 * 1024 * 1024

int bind_socket() {
	int sfd;
	struct sockaddr_un addr;
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1)
		return -1;
	/* Create socket */
	memset(&addr, 0, sizeof(struct sockaddr_un));
	/* Clear structure */
	addr.sun_family = AF_UNIX;
	/* UNIX domain address */
	strncpy(addr.sun_path, SOCKNAME, sizeof(addr.sun_path) - 1);
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
		return -1;
	return sfd;
}

int connect_socket() {
	int sfd;
	struct sockaddr_un addr;
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1)
		return -1;
	/* Create socket */
	memset(&addr, 0, sizeof(struct sockaddr_un));
	/* Clear structure */
	addr.sun_family = AF_UNIX;
	/* UNIX domain address */
	strncpy(addr.sun_path, SOCKNAME, sizeof(addr.sun_path) - 1);
	if (connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
		return -1;
	return sfd;
}

int share_memory_location(int fd, char * address) {
	if (listen(fd, 1) == -1)
		return -1;
	int cfd;

	// while(!sigint_received) {
		cfd = accept(fd, NULL, NULL);
		printf("Client connected\n");
		if(cfd == -1)
			return -1;
		printf("Sending '%s' to client\n", address);
		dprintf(cfd, address);
		close(cfd);
	// }

	return 0;
}

int close_socket(int fd) {
	close(fd);
	unlink(SOCKNAME);
	return 0;
}

int read_memory_location(int fd, void * memory_location, int buf_size) {
	int bytes_read;
  	bytes_read = read(fd, memory_location, 100);

	return 0;
}

int create_shared_hugepage() {
	int fd = open(HUGEPAGE_FILE_NAME, O_CREAT | O_RDWR, 0755);
	if (fd < 0)
		return -1;

        char * buf = mmap(
                (void *)(0x0UL),
                NUM_PAGES * PAGE_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE,
                fd,
                0
        );
        if (buf == MAP_FAILED) {
                return -1;
        }
	buf[0] = 'y';

	return fd;
}

int read_from_shared_hugepage(char * page_location, void * return_buf) {
	int fd = open(page_location, O_RDWR, 0755);
	if (fd < 0)
		return -1;

        char * buf = mmap(
                (void *)(0x0UL),
                NUM_PAGES * PAGE_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0
        );
        if (buf == MAP_FAILED) {
                return -1;
        }

	((char *) return_buf)[0] = buf[0];

	return fd;
}

int delete_shared_hugepage(int fd) {
	close(fd);
	unlink(HUGEPAGE_FILE_NAME);
	return 0;
}

int main(int argc , char ** argv) {
	if(argc < 2) {
		printf("Please provide either server or client as an argument\n");
		exit(1);
	}

	bool server_mode = false;

	if(strcmp(argv[1], "server") == 0) {
		printf("Server mode ...\n");
		server_mode = true;
	} else {
		printf("Client mode ...\n");
	}

	int fd;
	if(server_mode) {
		char * memory_location = HUGEPAGE_FILE_NAME;
		int hp_fd = create_shared_hugepage();
		if(hp_fd == -1) {
			perror("Couldn't allocate hugepage");
			exit(1);
		}

		fd = bind_socket();
		if(fd == -1) {
			printf("Error binding socket (does %s exist? Delete it!)\n", SOCKNAME);
			perror("");
			exit(1);
		}
		if(share_memory_location(fd, memory_location) == -1) {
			perror("Error sharing memory location");
			exit(1);
		}
		sleep(5);
		delete_shared_hugepage(hp_fd);
		close_socket(fd);
	} else {
		char mem_loc[100];
		fd = connect_socket();
		if(read_memory_location(fd, &mem_loc, 100) == -1) {
			perror("Error reading memory location");
			exit(1);
		}
		printf("Memory location is '%s'\n", mem_loc);

		char hp_content[100];
		read_from_shared_hugepage(mem_loc, hp_content);
	        printf("Content of first byte at page '%s' is '%s'\n", 
		mem_loc,
		hp_content[0]);
	}
}
~~~


## Resources ##
[https://lwn.net/Articles/374424/](https://lwn.net/Articles/374424/)

[https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt](https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt)

