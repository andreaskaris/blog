## IP address conversion with golang

### Finding an IP network's broadcast IP

The following function calculates the broadcast IP for both IPv4 and IPv6 networks:
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
