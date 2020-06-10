#!/usr/bin/env python3

import sys, signal
import socket
import threading
import time

SPEED=0
SPEED_ASCENDING=True

def signal_handler(signal, frame):
    global keeprunning
    keeprunning = False
    print("\nExiting..")
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)

UDP_IP = "192.168.100.194"
UDP_PORT = 777
MESSAGE = bytearray(b"BEPISA")
MESSAGE.extend((SPEED.to_bytes(2, byteorder='little', signed=True)))

print("UDP target IP: %s" % UDP_IP)
print("UDP target port: %s" % UDP_PORT)
print("message: %s" % MESSAGE)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_message():
    global sock, keeprunning, UDP_IP, UDP_PORT, SPEED, SPEED_ASCENDING
    if SPEED_ASCENDING:
        if SPEED < 700:
            SPEED = SPEED + 1
        else:
            SPEED_ASCENDING = False
            SPEED = SPEED - 1
    else:
        if SPEED > -700:
            SPEED = SPEED - 1
        else:
            SPEED_ASCENDING = True
            SPEED = SPEED + 1
    MESSAGE = bytearray(b"BEPISA")
    MESSAGE.extend((SPEED.to_bytes(2, byteorder='little', signed=True)))
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    if keeprunning:
        threading.Timer(0.02, send_message).start()

keeprunning = True
send_message()

time.sleep(1200)
keeprunning = False