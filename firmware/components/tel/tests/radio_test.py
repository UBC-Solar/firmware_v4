from datetime import datetime, timezone
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
        
def send_rtc_message(bus):
    now = datetime.now(timezone.utc)

    # Simulate struct tm fields
    tm_sec   = now.second
    tm_min   = now.minute
    tm_hour  = now.hour
    tm_mday  = now.day

    # Python month is already 1-12
    # CAPL/C tm_mon is 0-11, then +1 when sent
    tm_mon = now.month - 1

    # struct tm stores years since 1900
    tm_year = now.year - 1900

    print(tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec)

    # Match CAPL byte packing exactly
    canData = [0] * 8

    canData[0] = tm_sec & 0xFF
    canData[1] = tm_min & 0xFF
    canData[2] = tm_hour & 0xFF
    canData[3] = tm_mday & 0xFF
    canData[4] = (tm_mon + 1) & 0xFF
    canData[5] = (tm_year - 100) & 0xFF
    canData[6] = 0x00
    canData[7] = 0x00

    message = can.Message(
        arbitration_id=0x300,
        data=canData,
        is_extended_id=False
    )

    try:
        bus.send(message)
        print(f"CAN Data Sent: {canData}")

    except can.CanError as e:
        print(f"RTC Sync Failed: {e}")

if(__name__ == "__main__"):
    with can.interface.Bus(channel='can0', interface='socketcan', bitrate=500000) as bus:
        # send_message(0x580, [0x04]) 
        send_rtc_message(bus)
        # drd_test.run_test()
        print("Test completed")