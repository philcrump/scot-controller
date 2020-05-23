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

    sid = int.from_bytes(data[0:4], byteorder='little', signed=False)

    if sid == 0x010:
        # Azimuth Resolver Position
        azimuth_raw = int.from_bytes(data[5:7], byteorder='little', signed=False)
        azimuth_degrees = azimuth_raw * (360.0 / 65536.0);
        az_received = az_received + 1
        if start_time == 0:
            start_time = time()

        print(f"[Resolver] Azimuth: {bcolors.BOLD}%6.2f°{bcolors.OFF} (0x%04X)" \
                     % (azimuth_degrees, azimuth_raw))
        print(f"Message rate: %.2f/s" % (az_received / (time()-start_time)))
    elif sid == 0x24:
        # Motor info Message
        speed_commanded = int.from_bytes(data[5:7], byteorder='little', signed=True)
        voltage = int.from_bytes(data[7:9], byteorder='little', signed=False)
        current = int.from_bytes(data[9:11], byteorder='little', signed=True)
        ms_since_last_command = data[11]
        limit_1 = (data[12] >> 7) & 0x01;
        limit_2 = (data[12] >> 6) & 0x01;
        pwmfault = (data[12] >> 5) & 0x01;
        control_enable = (data[12] >> 4) & 0x01;

        print("[Motor] Azimuth: %+4d, Voltage: %.3f, Current: %+.3f, L1: %s%d%s, L2: %s%d%s, PWM: %s%d%s, Control: %s%d%s, since last command: %dms" \
         % (speed_commanded, \
            (voltage*1.25) / 1000.0, \
            (current*1.25) / 1000.0, \
            bcolors.WARNING if limit_1 else bcolors.OKGREEN, limit_1, bcolors.OFF,
            bcolors.WARNING if limit_2 else bcolors.OKGREEN, limit_2, bcolors.OFF,
            bcolors.WARNING if pwmfault else bcolors.OKGREEN, pwmfault, bcolors.OFF,
            bcolors.OKGREEN if control_enable else bcolors.WARNING, control_enable, bcolors.OFF,
            ms_since_last_command))
    #else:
    #   print(hex(sid))