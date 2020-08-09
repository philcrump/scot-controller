#!/usr/bin/env python3

import socket
from time import time

UDP_IP = ''
UDP_PORT = 11234

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    OFF = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

start_time = 0
az_received = 0

while True:
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes

    if data[4] == 0xFF:
        # ADC Packet
        joystick_el_raw = int.from_bytes(data[5:9], byteorder='little', signed=False)
        joystick_az_raw = int.from_bytes(data[9:13], byteorder='little', signed=False)

        print(f"[Joystick] Elevation: 0x%08X, Azimuth: 0x%08X" % (joystick_el_raw, joystick_az_raw))

    #else:
    #   print(hex(sid))