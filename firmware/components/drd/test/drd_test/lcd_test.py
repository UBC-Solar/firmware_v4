import can
import time
import yaml
import threading

bus = can.interface.Bus(channel='can0', interface='socketcan', bitrate=500000)
print("CAN bus initialized")

canid = '0x100'
data = [0x00]