## IP address conversion with golang

### Finding an IP network's broadcast IP

The following function calculates the broadcast IP for both IPv4 and IPv6 networks:
~~~
func subnetBroadcastIP(ipnet net.IPNet) (net.IP, error) {
	byteIp := []byte(ipnet.IP)                // []byte representation of IP
	byteMask := []byte(ipnet.Mask)            // []byte representation of mask
	byteTargetIp := make([]byte, len(byteIp)) // []byte holding target IP
	for k, _ := range byteIp {
		// mask will give us all fixed bits of the subnet (for the given byte)
		// inverted mask will give us all moving bits of the subnet (for the given byte)
		invertedMask := byteMask[k] ^ 0xff // inverted mask byte

		// broadcastIP = networkIP added to the inverted mask
		byteTargetIp[k] = byteIp[k]&byteMask[k] | invertedMask
	}

	return net.IP(byteTargetIp), nil
}
~~~

In order to understand the aforementioned example better, here is a longer version (the if / else block is useless and hence removed):
~~~
func subnetBroadcastIP(ipnet net.IPNet) (net.IP, error) {
	// ipv4
	if ipnet.IP.To4() != nil {
		// ip address in uint32
		ipBits := binary.BigEndian.Uint32(ipnet.IP.To4())

		// mask will give us all fixed bits of the subnet
		maskBits := binary.BigEndian.Uint32(ipnet.Mask)

		// inverted mask will give us all moving bits of the subnet
		invertedMaskBits := maskBits ^ 0xffffffff // xor the mask

		// network ip
		networkIpBits := ipBits & maskBits

		// broadcastIP = networkIP added to the inverted mask
		broadcastIpBits := networkIpBits | invertedMaskBits

		broadcastIp := make(net.IP, 4)
		binary.BigEndian.PutUint32(broadcastIp, broadcastIpBits)
		return broadcastIp, nil
	}
	// ipv6
	// this conversion is actually easier, follows the same principle as above
	byteIp := []byte(ipnet.IP)                // []byte representation of IP
	byteMask := []byte(ipnet.Mask)            // []byte representation of mask
	byteTargetIp := make([]byte, len(byteIp)) // []byte holding target IP
	for k, _ := range byteIp {
		invertedMask := byteMask[k] ^ 0xff // inverted mask byte
		byteTargetIp[k] = byteIp[k]&byteMask[k] | invertedMask
	}

	return net.IP(byteTargetIp), nil
}
~~~

### Finding all IPs in a subnet

~~~
func subnetIPs(ipnet net.IPNet) ([]net.IP, error) {
        var ipList []net.IP
        ip := ipnet.IP
        for ; ipnet.Contains(ip); ip = incIP(ip) {
                ipList = append(ipList, ip) 
        }

        return ipList, nil 
}

func incIP(ip net.IP) net.IP {
        // allocate a new IP
        newIp := make(net.IP, len(ip))
        copy(newIp, ip) 

        byteIp := []byte(newIp)
        l := len(byteIp)
        var i int 
        for k, _ := range byteIp {
                // start with the rightmost index first
                // increment it
                // if the index is < 256, then no overflow happened and we increment and break
                // else, continue to the next field in the byte
                i = l - 1 - k 
                if byteIp[i] < 0xff {
                        byteIp[i]++
                        break
                } else {
                        byteIp[i] = 0 
                }
        }
        return net.IP(byteIp)
}
~~~
