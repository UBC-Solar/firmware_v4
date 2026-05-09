import can
import time
import threading
import struct

"""
Important notes: the python CAN library sends data in little endian meaning the least significant bit and byte is first
Ex: [1, 0, 0, 0, 0, 0, 0, 0]
1 in this case is 00000001 in binary, but due to little endianness it becomes 1000000, sending the first bit of the CAN message


"""

def send_message(can_id, data, isextended_id=False):
    message = can.Message(arbitration_id=can_id, data=data, is_extended_id=isextended_id)
    try:
        bus.send(message)
        #print(f"Message sent on CANID {can_id}:{data}")
    except can.CanError as e:
        print(f"Message NOT sent {e}")
        
if(__name__ == "__main__"):
    with can.interface.Bus(channel='can0', interface='socketcan', bitrate=500000) as bus:
        send_message(0x580, [0x04]) 
        # drd_test.run_test()
        print("Test completed")