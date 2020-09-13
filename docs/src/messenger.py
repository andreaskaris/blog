#!/usr/bin/env python3

import socket
import sys

BUF_SIZE = 10
SERVER_PORT = 12345
SERVER_ADDRESS = "127.0.0.1"
MSG = "MSG"

def receiver():
    print("Starting receiver")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((SERVER_ADDRESS, SERVER_PORT))
                         
    while True:
        data, addr = sock.recvfrom(BUF_SIZE)
        print("Received datagram from ", addr)
        print("Datagram content: ", data)

def sender():
    print("Starting sender")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(MSG.encode('utf-8'), (SERVER_ADDRESS, SERVER_PORT))

if(sys.argv[1] == "sender"):
    print("This node is a sender")
    sender()
else:
    print("This node is a receiver")
    receiver()
