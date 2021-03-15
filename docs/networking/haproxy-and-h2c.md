## haproxy and HTTP2 with a focus on h2c

This blog article focuses on haproxy's capabilities when it comes to protocol upgrades from HTTP/1.1 to HTTP/2 - all in the context of connection termination on the haproxy side. 
All tests will create one TCP connection from the client to the haproxy frontend and another TCP connection from the haproxy backend to the backend server.

haproxy can support HTTP/2 via encrypted as well as via clear text connections. For encrypted connections, haproxy allows protocol negotiation with ALPN (Application-Layer Protocol Negotiation).

* https://en.wikipedia.org/wiki/Application-Layer_Protocol_Negotiation

ALPN is a TLS protocol extension. Thus, this feature requires TLS connections from the client to the haproxy frontend and from the haproxy backend to the backend server.

haproxy does not support the `Upgrade: h2c` statement. Hence, it is possible to establish either HTTP/1.1 or HTTP/2 connections, but clear text connections cannot be upgraded from HTTP/1.1 to HTTP/2.

### Some theory - h2 vs h2c

For the differences between HTTP/2 and h2c, see RFC 7540:

* https://tools.ietf.org/html/rfc7540#section-3.1
* https://tools.ietf.org/html/rfc7540#section-3.2
~~~
3.1.  HTTP/2 Version Identification

   The protocol defined in this document has two identifiers.

   o  The string "h2" identifies the protocol where HTTP/2 uses
      Transport Layer Security (TLS) [TLS12].  This identifier is used
      in the TLS application-layer protocol negotiation (ALPN) extension
      [TLS-ALPN] field and in any place where HTTP/2 over TLS is
      identified.

      The "h2" string is serialized into an ALPN protocol identifier as
      the two-octet sequence: 0x68, 0x32.

   o  The string "h2c" identifies the protocol where HTTP/2 is run over
      cleartext TCP.  This identifier is used in the HTTP/1.1 Upgrade
      header field and in any place where HTTP/2 over TCP is identified.

      The "h2c" string is reserved from the ALPN identifier space but
      describes a protocol that does not use TLS.

   Negotiating "h2" or "h2c" implies the use of the transport, security,
   framing, and message semantics described in this document.

3.2.  Starting HTTP/2 for "http" URIs

   A client that makes a request for an "http" URI without prior
   knowledge about support for HTTP/2 on the next hop uses the HTTP
   Upgrade mechanism (Section 6.7 of [RFC7230]).  The client does so by
   making an HTTP/1.1 request that includes an Upgrade header field with
   the "h2c" token.  Such an HTTP/1.1 request MUST include exactly one
   HTTP2-Settings (Section 3.2.1) header field.

   For example:

     GET / HTTP/1.1
     Host: server.example.com
     Connection: Upgrade, HTTP2-Settings
     Upgrade: h2c
     HTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>

   Requests that contain a payload body MUST be sent in their entirety
   before the client can send HTTP/2 frames.  This means that a large
   request can block the use of the connection until it is completely
   sent.

   If concurrency of an initial request with subsequent requests is
   important, an OPTIONS request can be used to perform the upgrade to
   HTTP/2, at the cost of an additional round trip.



Belshe, et al.               Standards Track                    [Page 8]

 
RFC 7540                         HTTP/2                         May 2015


   A server that does not support HTTP/2 can respond to the request as
   though the Upgrade header field were absent:

     HTTP/1.1 200 OK
     Content-Length: 243
     Content-Type: text/html

     ...

   A server MUST ignore an "h2" token in an Upgrade header field.
   Presence of a token with "h2" implies HTTP/2 over TLS, which is
   instead negotiated as described in Section 3.3.

   A server that supports HTTP/2 accepts the upgrade with a 101
   (Switching Protocols) response.  After the empty line that terminates
   the 101 response, the server can begin sending HTTP/2 frames.  These
   frames MUST include a response to the request that initiated the
   upgrade.

   For example:

     HTTP/1.1 101 Switching Protocols
     Connection: Upgrade
     Upgrade: h2c

     [ HTTP/2 connection ...

   The first HTTP/2 frame sent by the server MUST be a server connection
   preface (Section 3.5) consisting of a SETTINGS frame (Section 6.5).
   Upon receiving the 101 response, the client MUST send a connection
   preface (Section 3.5), which includes a SETTINGS frame.

   The HTTP/1.1 request that is sent prior to upgrade is assigned a
   stream identifier of 1 (see Section 5.1.1) with default priority
   values (Section 5.3.5).  Stream 1 is implicitly "half-closed" from
   the client toward the server (see Section 5.1), since the request is
   completed as an HTTP/1.1 request.  After commencing the HTTP/2
   connection, stream 1 is used for the response.
~~~

### Upgrade h2c with haproxy

Haproxy - protocol upgrades from HTTP/1.1 to HTTP/2 via `Upgrade: h2c` is not possible with haproxy. For negotiation of the protocol, haproxy only supports ALPN. 

Otherwise the protocol must be forced to either HTTP/1.1 or HTTP/2. For example, on the frontend, it is possible to not select a protocol. A client can then either initiate an HTTP/1.1 connection or an HTTP/2 connection, but it cannot upgrade from HTTP/1.1 to HTTP/2. (see `POC` for further details)

On the backend side, it is the same. You can either force the backend proto via `proto h2` to be HTTP/2. Or you can use HTTP/1.1. For negotiation of the protocol, you must use ALPN on the backend.

Refer to the documentation: http://cbonte.github.io/haproxy-dconv/2.4/configuration.html
~~~
alpn <alpn>  defines which protocols to advertise with ALPN. The protocol
             list consists in a comma-delimited list of protocol names,
             for instance: "http/1.1,http/1.0" (without quotes).
             If it is not set, the server ALPN is used.
~~~

~~~
alpn <protocols>

This enables the TLS ALPN extension and advertises the specified protocol
list as supported on top of ALPN. The protocol list consists in a comma-
delimited list of protocol names, for instance: "http/1.1,http/1.0" (without
quotes). This requires that the SSL library is built with support for TLS
extensions enabled (check with haproxy -vv). The ALPN extension replaces the
initial NPN extension. ALPN is required to enable HTTP/2 on an HTTP frontend.
Versions of OpenSSL prior to 1.0.2 didn't support ALPN and only supposed the
now obsolete NPN extension. At the time of writing this, most browsers still
support both ALPN and NPN for HTTP/2 so a fallback to NPN may still work for
a while. But ALPN must be used whenever possible. If both HTTP/2 and HTTP/1.1
are expected to be supported, both versions can be advertised, in order of
preference, like below :

     bind :443 ssl crt pub.pem alpn h2,http/1.1
~~~

~~~
proto <name>

Forces the multiplexer's protocol to use for the incoming connections. It
must be compatible with the mode of the frontend (TCP or HTTP). It must also
be usable on the frontend side. The list of available protocols is reported
in haproxy -vv.
Idea behind this option is to bypass the selection of the best multiplexer's
protocol for all connections instantiated from this listening socket. For
instance, it is possible to force the http/2 on clear TCP by specifying "proto
h2" on the bind line.
~~~

~~~
[root@26b317c91a20 /]# haproxy -vv | grep proto -A5
Available multiplexer protocols :
(protocols marked as <default> cannot be specified using 'proto' keyword)
            fcgi : mode=HTTP       side=BE        mux=FCGI
       <default> : mode=HTTP       side=FE|BE     mux=H1
              h2 : mode=HTTP       side=FE|BE     mux=H2
       <default> : mode=TCP        side=FE|BE     mux=PASS
~~~

Version for tests in POC:
~~~
[root@ef82916a0181 /]# haproxy -v
HA-Proxy version 2.2.9-a947cc2 2021/02/06 - https://haproxy.org/
Status: long-term supported branch - will stop receiving fixes around Q2 2025.
Known bugs: http://www.haproxy.org/bugs/bugs-2.2.9.html
Running on: Linux 4.18.0-240.1.1.el8_3.x86_64 #1 SMP Fri Oct 16 13:36:46 EDT 2020 x86_64
~~~

The tested haproxy version [0] can only negotiate the HTTP version via ALPN and this requires the use of TLS and thus of encrypted connections. This applies to both frontend and backend connections. 
Thus, if haproxy uses ALPN on both the frontend and the backend, any higher level tool that configures haproxy (such as the `openshift-router` binary inside the OpenShift router pods) does not have to worry and can simply rely on ALPN protocol negotiation to find the correct protocol for the client and for the backend server.
The `Upgrade: h2c` statement does not seem to be supported with haproxy. I could not find official sources for this, but my tests and research on the web point towards this.
On unencrypted frontends, haproxy can be configured to accept HTTP/1.1 and HTTP/2 connections by omitting the `proto` parameter, but this will work only if the client directly chooses either of the 2 protocols. On the frontend, upgrade attempts from HTTP/1.1 to HTTP/2 will fail. HTTP/2 can be forced via `proto h2`. I did not verify if it is possible to force HTTP/1.1 on the frontend.
On the backend, haproxy does not offer any way to negotiate the protocol. Administrators or tools must choose either HTTP/1.1 by omitting `proto` or HTTP/2 by setting `proto h2`.

### POC

#### POC reproducer

I used https://github.com/andreaskaris/http2-poc for the POC setup and my testings

#### HTTPD basaeline

With the correct configuration, httpd supports the `Upgrade: h2c` statement:
~~~
<VirtualHost *:$H2C_PORT>
DocumentRoot /var/www/html2
ServerName www.h2c.example.org
Protocols h2c
</VirtualHost>
~~~

In that case, either of the 3 following tests works with httpd:
~~~
[root@kind haproxy]# curl http://localhost:8081 -v
* Rebuilt URL to: http://localhost:8081/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8081 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8081 (#0)
> GET / HTTP/1.1
> Host: localhost:8081
> User-Agent: curl/7.61.1
> Accept: */*
> 
< HTTP/1.1 200 OK
< Date: Mon, 08 Mar 2021 19:58:09 GMT
< Server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< Upgrade: h2c
< Connection: Upgrade
< Last-Modified: Mon, 08 Mar 2021 18:34:18 GMT
< ETag: "16-5bd0aae785e80"
< Accept-Ranges: bytes
< Content-Length: 22
< Content-Type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact

---

[root@kind http2-haproxy]# curl --http2 http://localhost:8081 -v
* Rebuilt URL to: http://localhost:8081/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8081 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8081 (#0)
> GET / HTTP/1.1
> Host: localhost:8081
> User-Agent: curl/7.61.1
> Accept: */*
> Connection: Upgrade, HTTP2-Settings
> Upgrade: h2c
> HTTP2-Settings: AAMAAABkAARAAAAAAAIAAAAA
> 
< HTTP/1.1 101 Switching Protocols
< Upgrade: h2c
< Connection: Upgrade
* Received 101
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< date: Sun, 00 Jan 1900 00:00:00 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: W/"16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact

---

[root@kind http2-haproxy]# curl --http2-prior-knowledge http://localhost:8081 -v
* Rebuilt URL to: http://localhost:8081/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8081 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8081 (#0)
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Using Stream ID: 1 (easy handle 0x55d307eec480)
> GET / HTTP/2
> Host: localhost:8081
> User-Agent: curl/7.61.1
> Accept: */*
> 
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< date: Mon, 08 Mar 2021 19:18:38 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact
~~~

#### Testing the haproxy frontend side 

The problem with haproxy is that it cannot upgrade an unencrypted frontend HTTP/1.1 connection to HTTP/2 --- the Upgrade: h2c statement does not work.

According to https://discourse.haproxy.org/t/converting-http-2-h2c-without-tls-to-http-1-1/2176/10 , upgrading from HTTP/1.1 to HTTP/2 on the frontend is only possible in haproxy with ALPN.

Also see https://www.haproxy.com/blog/haproxy-1-9-has-arrived/ and https://www.haproxy.com/blog/haproxy-2-0-and-beyond/#end-to-end-http-2

##### Testing to force h2,http/1.1 in that order

Something like this does not work:
~~~
frontend fe_h2c
    mode http
    bind *:8084 proto h2,http/1.1
    default_backend be_h2c

backend be_h2c
    mode http
    server server1 10.88.0.123:8081 proto h2
~~~

~~~
kill -USR2 8
~~~

~~~
[WARNING] 066/191543 (8) : Reexecuting Master process
[NOTICE] 066/191543 (8) : haproxy version is 2.2.9-a947cc2
[NOTICE] 066/191543 (8) : path to executable is /usr/sbin/haproxy
[ALERT] 066/191543 (8) : parsing [/etc/haproxy/haproxy.cfg:47] : 'bind *:8084' : 'proto' :  unknown MUX protocol 'h2,http/1.1'
[ALERT] 066/191543 (8) : Error(s) found in configuration file : /etc/haproxy/haproxy.cfg
[ALERT] 066/191543 (8) : Fatal errors found in configuration.
[WARNING] 066/191543 (8) : Reexecuting Master process in waitpid mode
[WARNING] 066/191543 (8) : Reexecuting Master process
~~~

The documentation also clarifies this in the `proto` section:
~~~
[root@26b317c91a20 /]# haproxy -vv | grep proto -A5
Available multiplexer protocols :
(protocols marked as <default> cannot be specified using 'proto' keyword)
            fcgi : mode=HTTP       side=BE        mux=FCGI
       <default> : mode=HTTP       side=FE|BE     mux=H1
              h2 : mode=HTTP       side=FE|BE     mux=H2
       <default> : mode=TCP        side=FE|BE     mux=PASS
~~~

##### Forcing only HTTP/2

Whereas the following works but will only allow HTTP/2:
~~~
frontend fe_h2c
    mode http
    bind *:8084 proto h2
    default_backend be_h2c

backend be_h2c
    mode http
    server server1 10.88.0.123:8081 proto h2
~~~

But a connection upgrade is not possible as HTTP/1.1 is disabled:
~~~
[root@kind haproxy]# curl http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
> GET / HTTP/1.1
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> 
* Empty reply from server
* Connection #0 to host localhost left intact
curl: (52) Empty reply from server

---

[root@kind http2-haproxy]# curl --http2 http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
> GET / HTTP/1.1
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> Connection: Upgrade, HTTP2-Settings
> Upgrade: h2c
> HTTP2-Settings: AAMAAABkAARAAAAAAAIAAAAA
> 
* Empty reply from server
* Connection #0 to host localhost left intact
curl: (52) Empty reply from server

---

[root@kind http2-haproxy]# curl --http2-prior-knowledge http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Using Stream ID: 1 (easy handle 0x563681d13480)
> GET / HTTP/2
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> 
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< date: Mon, 08 Mar 2021 19:17:15 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact
~~~

##### Permitting either HTTP/1.1 or HTTP/2 on the frontend, no upgrade

On the other hand, the following works for HTTP/1.1 and HTTP/2, but not for upgrades from 1.1 to HTTP/2 via upgrade h2c:
~~~
frontend fe_h2c 
    mode http 
    bind *:8084 # proto h2
    default_backend be_h2c 
 
backend be_h2c 
    mode http 
    server server1 10.88.0.123:8081 proto h2 
~~~

~~~
kill -USR2 8
~~~

~~~
[WARNING] 066/192956 (8) : Reexecuting Master process
[WARNING] 066/192956 (138) : Stopping frontend GLOBAL in 0 ms.
[WARNING] 066/192956 (138) : Stopping frontend fe_http1 in 0 ms.
[WARNING] 066/192956 (138) : Stopping backend be_http1 in 0 ms.
[WARNING] 066/192956 (138) : Stopping frontend fe_h2c in 0 ms.
[WARNING] 066/192956 (138) : Stopping backend be_h2c in 0 ms.
[WARNING] 066/192956 (138) : Stopping frontend fe_h2 in 0 ms.
[WARNING] 066/192956 (138) : Stopping backend be_h2 in 0 ms.
[NOTICE] 066/192956 (8) : New worker #1 (143) forked
[WARNING] 066/192956 (138) : Proxy GLOBAL stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy fe_http1 stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy be_http1 stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy fe_h2c stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy be_h2c stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy fe_h2 stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (138) : Proxy be_h2 stopped (cumulated conns: FE: 0, BE: 0).
[WARNING] 066/192956 (8) : Former worker #1 (138) exited with code 0 (Exit)
~~~


~~~
[root@kind http2-haproxy]# curl  http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
> GET / HTTP/1.1
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> 
< HTTP/1.1 200 
< date: Mon, 08 Mar 2021 19:28:19 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact

---

[root@kind http2-haproxy]# curl --http2  http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
> GET / HTTP/1.1
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> Connection: Upgrade, HTTP2-Settings
> Upgrade: h2c
> HTTP2-Settings: AAMAAABkAARAAAAAAAIAAAAA
> 
< HTTP/1.1 200 
< date: Mon, 08 Mar 2021 19:28:24 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact

---

[root@kind http2-haproxy]# curl --http2-prior-knowledge http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Using Stream ID: 1 (easy handle 0x55899a161480)
> GET / HTTP/2
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> 
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< date: Mon, 08 Mar 2021 19:28:28 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact
~~~

#### Testing the haproxy backend side ###

On the backend side, it is the same. You can either force the backend proto via `proto h2` to be HTTP/2. Or you can use HTTP/1.1. For negotiation of the protocol, you must use ALPN on the backend.

haproxy cannot upgrade an unencrypted backend HTTP/1.1 connection to HTTP/2 --- the Upgrade: h2c statement is not a functionality that haproxy provides.

According to https://discourse.haproxy.org/t/converting-http-2-h2c-without-tls-to-http-1-1/2176/10 , upgrading from HTTP/1.1 to HTTP/2 on the frontend is only possible in haproxy with ALPN.

Also see https://www.haproxy.com/blog/haproxy-1-9-has-arrived/ and https://www.haproxy.com/blog/haproxy-2-0-and-beyond/#end-to-end-http-2

In order to verify this, I installed wireshark inside the haproxy pod, ran a live capture of the backend traffic with:
~~~
 tshark -i eth0 host 10.88.0.123 -O http
~~~

And then, I ran curl against the frontend.

#### Baseline, HTTP only virtual server

This is the frontend and backend configuration:
~~~
frontend fe_http1 
    mode http 
    bind *:8083
    default_backend be_http1 
 
backend be_http1 
    mode http 
    server server1 10.88.0.123:8080
~~~

~~~
curl http://localhost:8083 -v
~~~

~~~
(...)
Hypertext Transfer Protocol
    GET / HTTP/1.1\r\n
        [Expert Info (Chat/Sequence): GET / HTTP/1.1\r\n]
            [GET / HTTP/1.1\r\n]
            [Severity level: Chat]
            [Group: Sequence]
        Request Method: GET
        Request URI: /
        Request Version: HTTP/1.1
    host: localhost:8083\r\n
    user-agent: curl/7.61.1\r\n
    accept: */*\r\n
    x-forwarded-for: 10.88.0.1\r\n
    connection: close\r\n
    \r\n
    [Full request URI: http://localhost:8083/]
    [HTTP request 1/1]
(...)
Hypertext Transfer Protocol
    HTTP/1.1 200 OK\r\n
        [Expert Info (Chat/Sequence): HTTP/1.1 200 OK\r\n]
            [HTTP/1.1 200 OK\r\n]
            [Severity level: Chat]
            [Group: Sequence]
        Response Version: HTTP/1.1
        Status Code: 200
        [Status Code Description: OK]
        Response Phrase: OK
    Date: Tue, 09 Mar 2021 10:17:50 GMT\r\n
    Server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j\r\n
    Last-Modified: Mon, 08 Mar 2021 18:34:18 GMT\r\n
    ETag: "c-5bd0aae785e80"\r\n
    Accept-Ranges: bytes\r\n
    Content-Length: 12\r\n
        [Content length: 12]
    Connection: close\r\n
    Content-Type: text/html; charset=UTF-8\r\n
    \r\n
    [HTTP response 1/1]
    [Time since request: 0.001789907 seconds]
    [Request in frame: 7]
    [Request URI: http://localhost:8083/]
    File Data: 12 bytes
Line-based text data: text/html (1 lines)
(...)
~~~

#### Baseline, HTTP/2 virtual server, unencrypted

We have 2 scenarios here. Both force clear text HTTP/2 on the backend.

In order to capture traffic, we need to change the tshark command to:
~~~
tshark -i eth0 -d tcp.port==8081,http -O http -O http2 host 10.88.0.123
~~~
> It's important to tell Wireshark that we expect to see http over port 8081 (`-d tcp.port==8081,http`). This will tell tshark to interpret everything via port 8081 as HTTP(2).

#### Forcing HTTP/2 on the frontend

~~~
frontend fe_h2c 
    mode http 
    bind *:8084 proto h2 
    default_backend be_h2c 
 
backend be_h2c 
    mode http 
    server server1 10.88.0.123:8081 proto h2
~~~

In this case, no backend connection is initiatied if one runs:
~~~
curl http://localhost:8084 -v
~~~

Neither with:
~~~
curl --http2 http://localhost:8084 -v
~~~

However, when one forces http2 from the start with ...
~~~
curl --http2-prior-knowledge http://localhost:8084 -v
~~~

... then one can see HTTP/2 on the backend:
~~~
(...)
Transmission Control Protocol, Src Port: 47652, Dst Port: 8081, Seq: 1, Ack: 1, Len: 45
HyperText Transfer Protocol 2
    Stream: Magic
        Magic: PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n
    Stream: SETTINGS, Stream ID: 0, Length 12
        Length: 12
        Type: SETTINGS (4)
        Flags: 0x00
            0000 000. = Unused: 0x00
            .... ...0 = ACK: False
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
        Settings - Enable PUSH : 0
            Settings Identifier: Enable PUSH (2)
            Enable PUSH: 0
        Settings - Max concurrent streams : 100
            Settings Identifier: Max concurrent streams (3)
            Max concurrent streams: 100
(...)
HyperText Transfer Protocol 2
    Stream: SETTINGS, Stream ID: 0, Length 6
        Length: 6
        Type: SETTINGS (4)
        Flags: 0x00
            0000 000. = Unused: 0x00
            .... ...0 = ACK: False
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
        Settings - Max concurrent streams : 100
            Settings Identifier: Max concurrent streams (3)
            Max concurrent streams: 100
    Stream: SETTINGS, Stream ID: 0, Length 0
        Length: 0
        Type: SETTINGS (4)
        Flags: 0x01, ACK
            0000 000. = Unused: 0x00
            .... ...1 = ACK: True
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
    Stream: WINDOW_UPDATE, Stream ID: 0, Length 4
        Length: 4
        Type: WINDOW_UPDATE (8)
        Flags: 0x00
            0000 0000 = Unused: 0x00
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .111 1111 1111 1111 0000 0000 0000 0000 = Window Size Increment: 2147418112
(...)
HyperText Transfer Protocol 2
    Stream: SETTINGS, Stream ID: 0, Length 0
        Length: 0
        Type: SETTINGS (4)
        Flags: 0x01, ACK
            0000 000. = Unused: 0x00
            .... ...1 = ACK: True
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
(...)
HyperText Transfer Protocol 2
    Stream: HEADERS, Stream ID: 1, Length 64, GET /
        Length: 64
        Type: HEADERS (1)
        Flags: 0x05, End Headers, End Stream
            00.0 ..0. = Unused: 0x00
            ..0. .... = Priority: False
            .... 0... = Padded: False
            .... .1.. = End Headers: True
            .... ...1 = End Stream: True
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0001 = Stream Identifier: 1
        [Pad Length: 0]
        Header Block Fragment: 8286410e6c6f63616c686f73743a38303834847a0b6375726c2f372e36312e3153032a2f…
        [Header Length: 161]
        [Header Count: 7]
        Header: :method: GET
            Name Length: 7
            Name: :method
            Value Length: 3
            Value: GET
            :method: GET
            [Unescaped: GET]
            Representation: Indexed Header Field
            Index: 2
        Header: :scheme: http
            Name Length: 7
            Name: :scheme
            Value Length: 4
            Value: http
            :scheme: http
            [Unescaped: http]
            Representation: Indexed Header Field
            Index: 6
        Header: :authority: localhost:8084
            Name Length: 10
            Name: :authority
            Value Length: 14
            Value: localhost:8084
            :authority: localhost:8084
            [Unescaped: localhost:8084]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 1
        Header: :path: /
            Name Length: 5
            Name: :path
            Value Length: 1
            Value: /
            :path: /
            [Unescaped: /]
            Representation: Indexed Header Field
            Index: 4
        Header: user-agent: curl/7.61.1
            Name Length: 10
            Name: user-agent
            Value Length: 11
            Value: curl/7.61.1
            user-agent: curl/7.61.1
            [Unescaped: curl/7.61.1]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 58
        Header: accept: */*
            Name Length: 6
            Name: accept
            Value Length: 3
            Value: */*
            accept: */*
            [Unescaped: */*]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 19
        Header: x-forwarded-for: 10.88.0.1
            Name Length: 15
            Name: x-forwarded-for
            Value Length: 9
            Value: 10.88.0.1
            [Unescaped: 10.88.0.1]
            Representation: Literal Header Field without Indexing - New Name
(...)
HyperText Transfer Protocol 2
    Stream: HEADERS, Stream ID: 1, Length 128, 200 OK
        Length: 128
        Type: HEADERS (1)
        Flags: 0x04, End Headers
            00.0 ..0. = Unused: 0x00
            ..0. .... = Priority: False
            .... 0... = Padded: False
            .... .1.. = End Headers: True
            .... ...0 = End Stream: False
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0001 = Stream Identifier: 1
        [Pad Length: 0]
        Header Block Fragment: 886196df697e9403ea681d8a08020a810dc035700253168dff769d86b19272b025da5da7…
        [Header Length: 284]
        [Header Count: 8]
        Header: :status: 200 OK
            Name Length: 7
            Name: :status
            Value Length: 3
            Value: 200
            :status: 200
            [Unescaped: 200]
            Representation: Indexed Header Field
            Index: 8
        Header: date: Tue, 09 Mar 2021 11:04:02 GMT
            Name Length: 4
            Name: date
            Value Length: 29
            Value: Tue, 09 Mar 2021 11:04:02 GMT
            date: Tue, 09 Mar 2021 11:04:02 GMT
            [Unescaped: Tue, 09 Mar 2021 11:04:02 GMT]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 33
        Header: server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
            Name Length: 6
            Name: server
            Value Length: 37
            Value: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
            server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
            [Unescaped: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 54
        Header: last-modified: Mon, 08 Mar 2021 18:34:18 GMT
            Name Length: 13
            Name: last-modified
            Value Length: 29
            Value: Mon, 08 Mar 2021 18:34:18 GMT
            last-modified: Mon, 08 Mar 2021 18:34:18 GMT
            [Unescaped: Mon, 08 Mar 2021 18:34:18 GMT]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 44
        Header: etag: "16-5bd0aae785e80"
            Name Length: 4
            Name: etag
            Value Length: 18
            Value: "16-5bd0aae785e80"
            etag: "16-5bd0aae785e80"
            [Unescaped: "16-5bd0aae785e80"]
            Representation: Literal Header Field without Indexing - Indexed Name
            Index: 34
        Header: accept-ranges: bytes
            Name Length: 13
            Name: accept-ranges
            Value Length: 5
            Value: bytes
            accept-ranges: bytes
            [Unescaped: bytes]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 18
        Header: content-length: 22
            Name Length: 14
            Name: content-length
            Value Length: 2
            Value: 22
            content-length: 22
            [Unescaped: 22]
            Representation: Literal Header Field without Indexing - Indexed Name
            Index: 28
        Header: content-type: text/html; charset=UTF-8
            Name Length: 12
            Name: content-type
            Value Length: 24
            Value: text/html; charset=UTF-8
            content-type: text/html; charset=UTF-8
            [Unescaped: text/html; charset=UTF-8]
            Representation: Literal Header Field with Incremental Indexing - Indexed Name
            Index: 31
    Stream: DATA, Stream ID: 1, Length 22
        Length: 22
        Type: DATA (0)
        Flags: 0x01, End Stream
            0000 .00. = Unused: 0x00
            .... 0... = Padded: False
            .... ...1 = End Stream: True
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0001 = Stream Identifier: 1
        [Pad Length: 0]
        Data: 485454502f3220696e7365637572652028683263290a
    Line-based text data: text/html (1 lines)
        HTTP/2 insecure (h2c)\n
(...)
HyperText Transfer Protocol 2
    Stream: WINDOW_UPDATE, Stream ID: 1, Length 4
        Length: 4
        Type: WINDOW_UPDATE (8)
        Flags: 0x00
            0000 0000 = Unused: 0x00
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0001 = Stream Identifier: 1
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0001 0110 = Window Size Increment: 22
    Stream: WINDOW_UPDATE, Stream ID: 0, Length 4
        Length: 4
        Type: WINDOW_UPDATE (8)
        Flags: 0x00
            0000 0000 = Unused: 0x00
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .111 1111 1111 1111 0000 0000 0001 0110 = Window Size Increment: 2147418134
(...)
HyperText Transfer Protocol 2
    Stream: GOAWAY, Stream ID: 0, Length 15
        Length: 15
        Type: GOAWAY (7)
        Flags: 0x00
            0000 0000 = Unused: 0x00
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0000 = Stream Identifier: 0
        0... .... .... .... .... .... .... .... = Reserved: 0x0
        .000 0000 0000 0000 0000 0000 0000 0001 = Promised-Stream-ID: 1
        Error: NO_ERROR (0)
        Additional Debug Data: 74696d656f7574
(...)
~~~

##### Not forcing HTTP/2 on the frontend

~~~
frontend fe_h2c
    mode http
    bind *:8084 # proto h2
    default_backend be_h2c

backend be_h2c
    mode http
    server server1 10.88.0.123:8081 proto h2
~~~

In that case, the following will yield an HTTP/1.1 answer on the frontend:
~~~
curl http://localhost:8084 -v
~~~

On the backend, however, we see the same HTTP/2 traffic as before:
~~~
[root@ef82916a0181 /]# tshark -i eth0 -d tcp.port==8081,http  host 10.88.0.123
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
    1 0.000000000  10.88.0.128 → 10.88.0.123  TCP 74 47690 → 8081 [SYN] Seq=0 Win=29200 Len=0 MSS=1460 SACK_PERM=1 TSval=281077415 TSecr=0 WS=128
    2 0.000016099  10.88.0.123 → 10.88.0.128  TCP 74 8081 → 47690 [SYN, ACK] Seq=0 Ack=1 Win=28960 Len=0 MSS=1460 SACK_PERM=1 TSval=3828088613 TSecr=281077415 WS=128
    3 0.000021473  10.88.0.128 → 10.88.0.123  TCP 66 47690 → 8081 [ACK] Seq=1 Ack=1 Win=29312 Len=0 TSval=281077415 TSecr=3828088613
    4 0.000064928  10.88.0.128 → 10.88.0.123  HTTP2 111 Magic, SETTINGS[0]
    5 0.000092463  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47690 [ACK] Seq=1 Ack=46 Win=29056 Len=0 TSval=3828088613 TSecr=281077415
    6 0.000330539  10.88.0.123 → 10.88.0.128  HTTP2 103 SETTINGS[0], SETTINGS[0], WINDOW_UPDATE[0]
    7 0.000335106  10.88.0.128 → 10.88.0.123  TCP 66 47690 → 8081 [ACK] Seq=46 Ack=38 Win=29312 Len=0 TSval=281077416 TSecr=3828088614
    8 0.000423649  10.88.0.128 → 10.88.0.123  HTTP2 75 SETTINGS[0]
    9 0.000446667  10.88.0.128 → 10.88.0.123  HTTP2 139 HEADERS[1]: GET /
   10 0.000451488  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47690 [ACK] Seq=38 Ack=128 Win=29056 Len=0 TSval=3828088614 TSecr=281077416
   11 0.000743338  10.88.0.123 → 10.88.0.128  HTTP2 203 HEADERS[1]: 200 OK
   12 0.000764739  10.88.0.123 → 10.88.0.128  HTTP2 97 DATA[1] (text/html)
   13 0.000785449  10.88.0.128 → 10.88.0.123  TCP 66 47690 → 8081 [ACK] Seq=128 Ack=206 Win=30336 Len=0 TSval=281077416 TSecr=3828088614
   14 0.000794674  10.88.0.128 → 10.88.0.123  HTTP2 92 WINDOW_UPDATE[1], WINDOW_UPDATE[0]
   15 0.041548863  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47690 [ACK] Seq=206 Ack=154 Win=29056 Len=0 TSval=3828088655 TSecr=281077416
~~~

And the following yields HTTP/2 on the frontend:
~~~
curl --http2-prior-knowledge http://localhost:8084 -v
~~~

And HTTP/2 on the backend:
~~~
[root@ef82916a0181 /]# tshark -i eth0 -d tcp.port==8081,http  host 10.88.0.123
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
    1 0.000000000  10.88.0.128 → 10.88.0.123  TCP 74 47742 → 8081 [SYN] Seq=0 Win=29200 Len=0 MSS=1460 SACK_PERM=1 TSval=281141371 TSecr=0 WS=128
    2 0.000061147  10.88.0.123 → 10.88.0.128  TCP 74 8081 → 47742 [SYN, ACK] Seq=0 Ack=1 Win=28960 Len=0 MSS=1460 SACK_PERM=1 TSval=3828152569 TSecr=281141371 WS=128
    3 0.000093249  10.88.0.128 → 10.88.0.123  TCP 66 47742 → 8081 [ACK] Seq=1 Ack=1 Win=29312 Len=0 TSval=281141371 TSecr=3828152569
    4 0.000237410  10.88.0.128 → 10.88.0.123  HTTP2 111 Magic, SETTINGS[0]
    5 0.000387862  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47742 [ACK] Seq=1 Ack=46 Win=29056 Len=0 TSval=3828152569 TSecr=281141371
    6 0.000951782  10.88.0.123 → 10.88.0.128  HTTP2 103 SETTINGS[0], SETTINGS[0], WINDOW_UPDATE[0]
    7 0.000967453  10.88.0.128 → 10.88.0.123  TCP 66 47742 → 8081 [ACK] Seq=46 Ack=38 Win=29312 Len=0 TSval=281141372 TSecr=3828152570
    8 0.001132627  10.88.0.128 → 10.88.0.123  HTTP2 75 SETTINGS[0]
    9 0.001202587  10.88.0.128 → 10.88.0.123  HTTP2 139 HEADERS[1]: GET /
   10 0.001231726  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47742 [ACK] Seq=38 Ack=128 Win=29056 Len=0 TSval=3828152570 TSecr=281141372
   11 0.002061044  10.88.0.123 → 10.88.0.128  HTTP2 203 HEADERS[1]: 200 OK
   12 0.002183092  10.88.0.123 → 10.88.0.128  HTTP2 97 DATA[1] (text/html)
   13 0.002425192  10.88.0.128 → 10.88.0.123  TCP 66 47742 → 8081 [ACK] Seq=128 Ack=206 Win=30336 Len=0 TSval=281141374 TSecr=3828152571
   14 0.002485282  10.88.0.128 → 10.88.0.123  HTTP2 92 WINDOW_UPDATE[1], WINDOW_UPDATE[0]
   15 0.043436470  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 47742 [ACK] Seq=206 Ack=154 Win=29056 Len=0 TSval=3828152613 TSecr=281141374
~~~

##### Forcing HTTP/2 on the frontend, HTTP/1.1 on the backend

It's also possible to force HTTP/2 on the frontend and then terminate the connection with HTTP/1.1 on the backend.

The following is also possible:
~~~
frontend fe_h2c
    mode http
    bind *:8084 proto h2
    default_backend be_h2c

backend be_h2c
    mode http
    server server1 10.88.0.123:8081 # proto h2
~~~

~~~
[root@kind ~]# curl --http2-prior-knowledge http://localhost:8084 -v
* Rebuilt URL to: http://localhost:8084/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8084 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8084 (#0)
* Using HTTP2, server supports multi-use
* Connection state changed (HTTP/2 confirmed)
* Copying HTTP/2 data in stream buffer to connection buffer after upgrade: len=0
* Using Stream ID: 1 (easy handle 0x557fb6d1e480)
> GET / HTTP/2
> Host: localhost:8084
> User-Agent: curl/7.61.1
> Accept: */*
> 
* Connection state changed (MAX_CONCURRENT_STREAMS == 100)!
< HTTP/2 200 
< date: Tue, 09 Mar 2021 11:27:25 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "16-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 22
< content-type: text/html; charset=UTF-8
< 
HTTP/2 insecure (h2c)
* Connection #0 to host localhost left intact
[root@kind ~]# 
~~~

~~~
[root@ef82916a0181 /]# tshark -i eth0 -d tcp.port==8081,http  host 10.88.0.123
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
    1 0.000000000  10.88.0.128 → 10.88.0.123  TCP 74 48014 → 8081 [SYN] Seq=0 Win=29200 Len=0 MSS=1460 SACK_PERM=1 TSval=282131294 TSecr=0 WS=128
    2 0.000019888  10.88.0.123 → 10.88.0.128  TCP 74 8081 → 48014 [SYN, ACK] Seq=0 Ack=1 Win=28960 Len=0 MSS=1460 SACK_PERM=1 TSval=3829142492 TSecr=282131294 WS=128
    3 0.000024442  10.88.0.128 → 10.88.0.123  TCP 66 48014 → 8081 [ACK] Seq=1 Ack=1 Win=29312 Len=0 TSval=282131294 TSecr=3829142492
    4 0.000063274  10.88.0.128 → 10.88.0.123  HTTP 191 GET / HTTP/1.1 
    5 0.000084897  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 48014 [ACK] Seq=1 Ack=126 Win=29056 Len=0 TSval=3829142493 TSecr=282131295
    6 0.000532732  10.88.0.123 → 10.88.0.128  HTTP 387 HTTP/1.1 200 OK  (text/html)
    7 0.000536368  10.88.0.128 → 10.88.0.123  TCP 66 48014 → 8081 [ACK] Seq=126 Ack=322 Win=30336 Len=0 TSval=282131295 TSecr=3829142493
    8 0.000611168  10.88.0.123 → 10.88.0.128  TCP 66 8081 → 48014 [FIN, ACK] Seq=322 Ack=126 Win=29056 Len=0 TSval=3829142493 TSecr=282131295
    9 0.000697672  10.88.0.128 → 10.88.0.123  TCP 66 48014 → 8081 [RST, ACK] Seq=126 Ack=323 Win=30336 Len=0 TSval=282131295 TSecr=3829142493
~~~


##### TLS/ALPN protocol detection for HTTP/2

Looking at the h2 virtual servers with TLS/ALPN in place:
~~~
frontend fe_h2 
    mode http 
    bind *:8085 ssl crt /etc/haproxy/certs/pem alpn h2,http/1.1 
    default_backend be_h2 
 
backend be_h2 
    mode http 
    server server1 10.88.0.123:8082 ssl verify none alpn h2 
~~~

On the frontend, we can force the protocol to use, just as an example, to be HTTP/1.1
~~~
[root@kind ~]# curl --http1.1 https://localhost:8085 -v -k
* Rebuilt URL to: https://localhost:8085/
*   Trying ::1...
* TCP_NODELAY set
* connect to ::1 port 8085 failed: Connection refused
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to localhost (127.0.0.1) port 8085 (#0)
* ALPN, offering http/1.1
* successfully set certificate verify locations:
*   CAfile: /etc/pki/tls/certs/ca-bundle.crt
  CApath: none
* TLSv1.3 (OUT), TLS handshake, Client hello (1):
* TLSv1.3 (IN), TLS handshake, Server hello (2):
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, Encrypted Extensions (8):
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, Certificate (11):
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, CERT verify (15):
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, Finished (20):
* TLSv1.3 (OUT), TLS change cipher, Change cipher spec (1):
* TLSv1.3 (OUT), TLS handshake, [no content] (0):
* TLSv1.3 (OUT), TLS handshake, Finished (20):
* SSL connection using TLSv1.3 / TLS_AES_256_GCM_SHA384
* ALPN, server accepted to use http/1.1
* Server certificate:
*  subject: C=CA; ST=Arctica; L=Northpole; O=Accme Inc; OU=DevOps; CN=www.company.org
*  start date: Mar  8 16:56:03 2021 GMT
*  expire date: Mar 21 16:56:03 2049 GMT
*  issuer: C=CA; ST=Arctica; L=Northpole; O=Accme Inc; OU=DevOps; CN=www.company.org
*  SSL certificate verify result: unable to get local issuer certificate (20), continuing anyway.
* TLSv1.3 (OUT), TLS app data, [no content] (0):
> GET / HTTP/1.1
> Host: localhost:8085
> User-Agent: curl/7.61.1
> Accept: */*
> 
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, Newsession Ticket (4):
* TLSv1.3 (IN), TLS handshake, [no content] (0):
* TLSv1.3 (IN), TLS handshake, Newsession Ticket (4):
* TLSv1.3 (IN), TLS app data, [no content] (0):
< HTTP/1.1 200 
< date: Tue, 09 Mar 2021 11:13:07 GMT
< server: Apache/2.4.46 (Fedora) OpenSSL/1.1.1j
< last-modified: Mon, 08 Mar 2021 18:34:18 GMT
< etag: "13-5bd0aae785e80"
< accept-ranges: bytes
< content-length: 19
< content-type: text/html; charset=UTF-8
< 
HTTP/2 secure (h2)
* Connection #0 to host localhost left intact
~~~

On the backend, we see that haproxy will negotiate and offer HTTP/2 to the server via ALPN:
~~~
[root@ef82916a0181 /]# tshark -i eth0 -O tls  host 10.88.0.123  | egrep -i 'application_layer_protocol_negotiation|alpn|Transmission Control Protocol'
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 0, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 0, Ack: 1, Len: 0
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 1, Ack: 1, Len: 0
18 Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 1, Ack: 1, Len: 601
            Extension: application_layer_protocol_negotiation (len=5)
                Type: application_layer_protocol_negotiation (16)
                ALPN Extension Length: 3
                ALPN Protocol
                    ALPN string length: 2
                    ALPN Next Protocol: h2
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 1, Ack: 602, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 1, Ack: 602, Len: 250
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 602, Ack: 251, Len: 0
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 602, Ack: 251, Len: 80
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 682, Ack: 251, Len: 67
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 251, Ack: 749, Len: 287
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 538, Ack: 749, Len: 59
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 749, Ack: 597, Len: 0
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 749, Ack: 597, Len: 31
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 780, Ack: 597, Len: 95
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 597, Ack: 875, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 597, Ack: 875, Len: 187
27 Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 875, Ack: 784, Len: 48
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 784, Ack: 923, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 784, Ack: 923, Len: 46
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 830, Ack: 923, Len: 24
Transmission Control Protocol, Src Port: 8082, Dst Port: 34156, Seq: 854, Ack: 923, Len: 0
Transmission Control Protocol, Src Port: 34156, Dst Port: 8082, Seq: 923, Ack: 855, Len: 0
~~~

When changing to the following:
~~~
frontend fe_h2 
    mode http 
    bind *:8085 ssl crt /etc/haproxy/certs/pem alpn h2,http/1.1 
    default_backend be_h2 
 
backend be_h2 
    mode http 
    server server1 10.88.0.123:8082 ssl verify none alpn h2,http/1.1
~~~

We can see that haproxy will send both options to the server in its offer:
~~~
[root@ef82916a0181 /]# tshark -i eth0 -O tls  host 10.88.0.123  | egrep -i 'application_layer_protocol_negotiation|alpn|Transmission Control Protocol'
Running as user "root" and group "root". This could be dangerous.
Capturing on 'eth0'
22 Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 0, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 0, Ack: 1, Len: 0
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 1, Ack: 1, Len: 0
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 1, Ack: 1, Len: 517
            Extension: application_layer_protocol_negotiation (len=14)
                Type: application_layer_protocol_negotiation (16)
                ALPN Extension Length: 12
                ALPN Protocol
                    ALPN string length: 2
                    ALPN Next Protocol: h2
                    ALPN string length: 8
                    ALPN Next Protocol: http/1.1
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 1, Ack: 518, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 1, Ack: 518, Len: 1521
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 518, Ack: 1522, Len: 0
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 518, Ack: 1522, Len: 80
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 598, Ack: 1522, Len: 67
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 1522, Ack: 665, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 1522, Ack: 665, Len: 287
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 1809, Ack: 665, Len: 287
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 2096, Ack: 665, Len: 59
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 665, Ack: 2155, Len: 0
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 665, Ack: 2155, Len: 31
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 696, Ack: 2155, Len: 95
30 Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 2155, Ack: 791, Len: 0
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 2155, Ack: 791, Len: 159
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 2314, Ack: 791, Len: 50
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 791, Ack: 2364, Len: 0
Transmission Control Protocol, Src Port: 34182, Dst Port: 8082, Seq: 791, Ack: 2364, Len: 48
Transmission Control Protocol, Src Port: 8082, Dst Port: 34182, Seq: 2364, Ack: 839, Len: 0
~~~

