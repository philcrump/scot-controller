#!/usr/bin/env python3

import socket
import threading
import time

SPEED=100

UDP_IP = "192.168.100.194"
UDP_PORT = 777
MESSAGE = bytearray(b"BEPISA")
MESSAGE.extend((SPEED.to_bytes(2, byteorder='big', signed=True)))

print("UDP target IP: %s" % UDP_IP)
print("UDP target port: %s" % UDP_PORT)
print("message: %s" % MESSAGE)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_message():
    global sock, MESSAGE, keeprunning, UDP_IP, UDP_PORT
    print("t")
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    if keeprunning:
        threading.Timer(0.05, send_message).start()

keeprunning = True
send_message()

time.sleep(5)
keeprunning = False