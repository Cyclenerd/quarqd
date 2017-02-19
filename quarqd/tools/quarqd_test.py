#!/usr/bin/python

import socket
import sys

s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)

host="localhost"
port=8168

s.connect(( host, port ))

s.send('X-set-channel: 173p\n')
s.send('X-set-channel: 19p\n')
s.send('X-set-channel: 21p\n')
s.send('X-set-channel: 0p\n')

while True:
    data=s.recv(1000000)
    print data,
    if not data:
        break

s.close()
