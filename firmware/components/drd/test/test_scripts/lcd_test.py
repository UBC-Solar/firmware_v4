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

class DRDTest:
    def __init__(self, bus):
        self.bus = bus
        self.prevswitch = 0

    def run_test(self):
        self.clearbus(self.bus)
        #  Test drive page speed and drive_state
        print("\nDrive Page Test and Speed")
        self.test_drive_page()
        
        # Trigger Fault page by sending a fault message
        print("\nFault Page: Triggering faults")
        self.test_fault_page()

        # Test warning page
        self.switch_page()
        print("\nWarning Page: Triggering warnings")
        self.test_warning_page()

        # Test Temperature page
        self.switch_page()
        print("\nTemperature Page: test temperature")
        self.script_send_temperature(self.bus)

        #Test Temperature page
        self.switch_page()

    def switch_page(self):
        if self.prevswitch == 0:
            send_message(0x580, [0x04])  # Command to switch to drive page
            self.prevswitch = 1
        else:
            send_message(0x580, [0x00])  # Command to switch to drive page
            self.prevswitch = 0

    def test_change_page(self):
        print("Testing 100 fast page changes, shouldn't flash because of debouncing")
        for i in range(100):
            self.switch_page()
            time.sleep(0.00025)
        time.sleep(1)
        print("Testing 100 fast page changes with fault flags, should stay on fault page")
        for i in range(100):
            send_message(0x622, [(1<<0),0,0,0,0,0,0,0])
            self.switch_page()
            time.sleep(0.00025)

    def test_drive_page(self):
        print("\n\nTesting Drive State")
        print("\n5 seconds to Switch Drive States with switch")
        for _ in range(10):
            self.test_drive_state()
            time.sleep(0.5)
        t2 = threading.Thread(target=self.script_send_speed_kmh, args=(self.bus,))
        t2.start()
        t2.join()

    def test_fault_page(self):
        #NOT DONE: Motor_Comm Fault, Throttle Out of Range, Throttle Mismatch, Not sent over CAN
        #Need to Test DCOC and COC with negative ECU pack current
        print("Test Estop LED")
        send_message(0x450, [0,0,0,0,0,(1 << 5),0,0])
        time.sleep(1)
        send_message(0x450, [0,0,0,0,0,0,0,0])

        print("Testing Slave Board Comm Fault and BMS Self Test Fault")
        send_message(0x622, [(1<<0)|(1<<1),0,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Battery Overtemperature, Battery Undervoltage")
        send_message(0x622, [(1<<2)|(1<<3),0,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        #TODO: check DCOC and COC works with correct CAN
        print("Testing Battery Overvoltage, Charge OC/DOC")
        send_message(0x622, [(1<<4)|(1<<6),0,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing VOLT_HI from battery")
        safe_value = int(134.4 * 468)
        value = int(136 * 468)
        send_message(0x623, [value & 0xFF,(value >> 8) & 0xFF,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x623, [safe_value & 0xFF,(safe_value >> 8) & 0xFF,0,0,0,0,0,0]) # clear
        time.sleep(1)
        # print("Testing VOLT_HI increment by 1 from battery")
        # value = int(136 * 468 + 1)
        # send_message(0x623, [value & 0xFF,(value >> 8) & 0xFF,0,0,0,0,0,0])
        # time.sleep(2)
        # send_message(0x623, [safe_value & 0xFF,(safe_value >> 8) & 0xFF,0,0,0,0,0,0])
        # time.sleep(1)
        print("Testing VOLT_LO from battery")
        value = int(86.72 * 468)
        send_message(0x623, [value & 0xFF,(value >> 8) & 0xFF,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x623, [safe_value & 0xFF,(safe_value >> 8) & 0xFF,0,0,0,0,0,0])
        time.sleep(1)
        # print("Testing VOLT_LO increment by 1 from battery")
        # value = int(86.72 * 468 - 1)
        # send_message(0x623, [value & 0xFF,(value >> 8) & 0xFF,0,0,0,0,0,0])
        # time.sleep(2)
        # send_message(0x623, [safe_value & 0xFF,(safe_value >> 8) & 0xFF,0,0,0,0,0,0])
        # time.sleep(1)

        print("Testing ECU RFW, and Motor System Error")
        # cansend can0 450#0000000000100000
        send_message(0x450, [0,0,0,0,0,(1<<4),0,0])
        send_message(0x08A50225, [0,0,0,(1<<0),0,0,0,0], isextended_id=True)
        time.sleep(1)
        send_message(0x450, [0,0,0,0,0,0,0,0])
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        time.sleep(1)

        print("Testing Motor Overcurrent, Motor Overvoltage, FET Thermistor Error")
        send_message(0x08A50225,[(1<<3),0,(1<<1)|(1<<3),0,0,0,0,0], isextended_id=True)
        time.sleep(1)
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        time.sleep(1)

        print("Testing 7 faults on page")
        send_message(0x622, [(1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<6),0,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing 5 faults on page")
        send_message(0x450, [0,0,0,0,0,(1<<4),0,0])
        send_message(0x08A50225,[(1<<3),0,(1<<1)|(1<<3),(1<<0),0,0,0,0], isextended_id=True)
        time.sleep(1)
        send_message(0x450, [0,0,0,0,0,0,0,0])
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        time.sleep(1)
        

    def test_warning_page(self):
        print("Testing Low Voltage Warning, High Voltage Warning")
        send_message(0x622,[0,(1<<5)|(1<<6),0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Battery Low Temperature Warning, Battery High Temperature Warning")
        send_message(0x622,[0,(1<<7),1,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing No ECU Current Message Received Warning")
        send_message(0x622,[0,0,(1<<2),0,0,0,0,0])
        time.sleep(1)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Pack Overdischarge, Pack Overcharge")
        send_message(0x450,[0,0,0,0,0,(1<<0)|(1<<1),0,0])
        time.sleep(1)
        send_message(0x450,[0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing all 7 warnings")
        send_message(0x622,[0,(1<<5)|(1<<6)|(1<<7),1|(1<<2),0,0,0,0,0])
        send_message(0x450,[0,0,0,0,0,(1<<0)|(1<<1),0,0])
        time.sleep(1)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        send_message(0x450,[0,0,0,0,0,0,0,0])
        time.sleep(1)

    def test_debug_page(self):
        time.sleep(1)  # Wait for the LCD to initialize
        t1 = threading.Thread(target=self.script_send_pack_current_and_voltage, args=(self.bus,))
        t1.start()
        t1.join()

    def test_drive_state(self):
        rpm_val = int((0 * 60) / (2 * 3.14159265 * 0.283 * 3.6)) + 1

        # Pack RPM into data[4] and data[5]
        data4 = (rpm_val & 0x1F) << 3        # bits 0-4 of rpm into bits 3-7
        data5 = (rpm_val >> 5) & 0x7F        # bits 5-11 of rpm into bits 0-6

        # Create 8-byte payload (other bytes can be 0)
        payload = [0x00, 0x00, 0x00, 0x00, data4, data5, 0x00, 0x00]
        send_message(0x08850225, payload, isextended_id=True)

    def script_send_speed_kmh(self, can_bus):
        """Send speed kmh from 1 to 100 over 10 seconds."""
        wheel_radius = 0.283  # meters
        for speed in range(0, 100):
            # Reverse engineer RPM from speed (v = w*r)
            rpm_val = int((speed * 60) / (2 * 3.14159265 * wheel_radius * 3.6)) + 1

            # Pack RPM into data[4] and data[5]
            data4 = (rpm_val & 0x1F) << 3        # bits 0-4 of rpm into bits 3-7
            data5 = (rpm_val >> 5) & 0x7F        # bits 5-11 of rpm into bits 0-6

            # Create 8-byte payload (other bytes can be 0)
            payload = [0x00, 0x00, 0x00, 0x00, data4, data5, 0x00, 0x00]

            send_message(0x08850225, payload, isextended_id=True)
            time.sleep(0.1)

    def script_send_temperature(self, can_bus):
        for temp in range(0, 101):
            # Battery Min and Max
            send_message(0x625, [0,temp,0,temp,0,0,0,0])

            float_bytes = struct.pack('<f', float(temp))
            
            # Extract the individual bytes (This replaces your bit-shifting)
            byte_0 = float_bytes[0]
            byte_1 = float_bytes[1]
            byte_2 = float_bytes[2]
            byte_3 = float_bytes[3]

            # Create the payload with the float repeated twice
            payload = [byte_0, byte_1, byte_2, byte_3, byte_0, byte_1, byte_2, byte_3]
            send_message(0x6A2, payload)
            send_message(0x6B2, payload)
            send_message(0x6C2, payload)

            # Motor controller temp
            raw_temp = int(temp/5)
            temp_1 = (raw_temp & 0x03) << 6
            temp_2 = (raw_temp >> 2) & 0x07
            send_message(0x08850225, [0,0,0,temp_1,temp_2,0,0,0], isextended_id=True)
            time.sleep(0.1)

    def script_send_pack_current_and_voltage(self, can_bus):
        """Send PackCurrent and PackVoltage signals over 30 seconds."""
        current = 40
        voltage = 134
        current_step = (current - (-20)) /120
        voltage_step = (voltage - 89) / 120

        for _ in range(120):
            send_message(0x623, [int(current)])
            send_message(0x450, [int(voltage)])
            current -= current_step
            voltage -= voltage_step
            time.sleep(0.250)

    def clearbus(self, can_bus):
        safe_value = int(134.4 * 468)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        send_message(0x450,[0,0,0,0,0,0,0,0])
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        send_message(0x623, [safe_value & 0xFF,(safe_value >> 8) & 0xFF,0,0,0,0,0,0]) # clear

if(__name__ == "__main__"):
    with can.interface.Bus(channel='PCAN_USBBUS1', interface='pcan', bitrate=500000) as bus:
        print("CAN bus initialized")
        drd_test = DRDTest(bus)
        drd_test.clearbus(bus)
        
        drd_test.script_send_speed_kmh(bus)
        print("Test completed")