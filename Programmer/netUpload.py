#!/usr/bin/env python3

# Uploads file to BDOS over network.
# File is written to current path.

import socket
import sys
from time import sleep

# read file to send
filename = "code.bin" # default file to send
if len(sys.argv) >= 2:
    filename = sys.argv[1]

outFilename = filename # name of the file on FPGC

# TODO: check on valid 8.3 filename

if len(sys.argv) >= 3:
    outFilename = sys.argv[2]

with open(filename, "rb") as f:
    binfile = f.read()

downloadToFile = True

# init connection
s = socket.socket()
port = 3220

try:
    s.connect(("192.168.0.213", port))


    bdata = ""
    if downloadToFile:
        bdata += "DOWN "
    else:
        bdata += "EXEC "

    bdata += str(len(binfile)) + ":"

    if downloadToFile:
        bdata += outFilename

    bdata += "\n"

    bdata = bdata.encode()
    bdata = bdata + binfile
    s.send(bdata)
    rcv = s.recv(1024)

    if rcv != b'THX!':
        print("Got wrong response:")
        print(rcv)

except:
    print("Could not connect")


if downloadToFile:
    print("File sent")
else:
    print("Program sent")

# close socket when done
s.close()
exit(0)