import can
import time
import threading

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
        #  Test drive page soc and speed
        # print("\nDrive Page: SOC and Speed incrementing")
        # self.test_drive_page()
        
        # Trigger Fault page by sending a fault message
        print("Fault Page: Triggering faults")
        self.test_fault_page()
        # Test warning page
        self.switch_page()
        print("Warning Page: Triggering warnings")
        self.test_warning_page()
        #Test Temperature page
        self.switch_page()

    def switch_page(self):
        if self.prevswitch == 0:
            send_message(0x580, [0x04])  # Command to switch to drive page
            self.prevswitch = 1
        else:
            send_message(0x580, [0x00])  # Command to switch to drive page
            self.prevswitch = 0
        time.sleep(1)  # Wait for the page to switch

    def test_drive_page(self):
        time.sleep(1)  # Wait for the LCD to initialize
        t1 = threading.Thread(target=self.script_send_soc, args=(self.bus,))
        t2 = threading.Thread(target=self.script_send_speed_kmh, args=(self.bus,))
        t1.start()
        t2.start()
        t1.join()
        t2.join()

    def test_fault_page(self):
        #NOT DONE: Motor_Comm Fault, Throttle Out of Range, Throttle Mismatch, Not sent over CAN
        #Need to Test DCOC and COC with negative ECU pack current

        print("Testing Slave Board Comm Fault and BMS Self Test Fault")
        send_message(0x622, [(1<<0)|(1<<1),0,0,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Battery Overtemperature, Battery Undervoltage")
        send_message(0x622, [(1<<2)|(1<<3),0,0,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        #TODO: check DCOC and COC works with correct CAN
        print("Testing Battery Overvoltage, Charge OC/DOC")
        send_message(0x622, [(1<<4)|(1<<6),0,0,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing VOLT_HI from battery")
        safe_value = int(134.4 * 468)
        value = int(136 * 468)
        send_message(0x623, [value & 0xFF,(value >> 8) & 0xFF,0,0,0,0,0,0])
        time.sleep(2)
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
        time.sleep(2)
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
        time.sleep(2)
        send_message(0x450, [0,0,0,0,0,0,0,0])
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        time.sleep(1)

        print("Testing Motor Overcurrent, Motor Overvoltage, FET Thermistor Error")
        send_message(0x08A50225,[(1<<3),0,(1<<1)|(1<<3),0,0,0,0,0], isextended_id=True)
        time.sleep(2)
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        time.sleep(1)

        print("Testing 7 faults on page")
        send_message(0x622, [(1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<6),0,0,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing 8 faults on page")
        send_message(0x450, [0,0,0,0,0,(1<<4),0,0])
        send_message(0x08A50225,[(1<<3),0,(1<<1)|(1<<3),(1<<0),0,0,0,0], isextended_id=True)
        time.sleep(2)
        send_message(0x450, [0,0,0,0,0,0,0,0])
        send_message(0x08A50225, [0,0,0,0,0,0,0,0], isextended_id=True)
        send_message(0x403, [0,0,0,0,0,0,0,0])
        time.sleep(1)
        

    def test_warning_page(self):
        print("Testing Low Voltage Warning, High Voltage Warning")
        send_message(0x622,[0,(1<<5)|(1<<6),0,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(2)

        print("Testing Battery Low Temperature Warning, Battery High Temperature Warning")
        send_message(0x622,[0,(1<<7),1,0,0,0,0,0])
        time.sleep(2)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(2)

        print("Testing No ECU Current Message Received Warning")
        send_message(0x622,[0,0,(1<<2),0,0,0,0,0])
        time.sleep(2)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        time.sleep(2)

        print("Testing Pack Overdischarge, Pack Overcharge")
        send_message(0x450,[0,0,0,0,0,(1<<0)|(1<<1),0,0])
        time.sleep(2)
        send_message(0x450,[0,0,0,0,0,0,0,0])
        time.sleep(2)

        print("Testing all 7 warnings")
        send_message(0x622,[0,(1<<5)|(1<<6)|(1<<7),1|(1<<2),0,0,0,0,0])
        send_message(0x450,[0,0,0,0,0,(1<<0)|(1<<1),0,0])
        time.sleep(2)
        send_message(0x622,[0,0,0,0,0,0,0,0])
        send_message(0x450,[0,0,0,0,0,0,0,0])
        time.sleep(2)

    def test_debug_page(self):
        time.sleep(1)  # Wait for the LCD to initialize
        t1 = threading.Thread(target=self.script_send_pack_current_and_voltage, args=(self.bus,))
        t1.start()
        t1.join()

    # SETS SPEED TO 0 SO DRIVE STATE CAN BE SWITCHED
    def script_test_drive_state(self, can_bus):
        send_message(0x08850225, [00], isextended_id=True)

    def script_send_soc(self, can_bus):
        # SOC 0-100% voltage range
        voltage_start = 94.0   # 0% SOC
        voltage_end = 129.0    # 100% SOC
        steps = 36             # step every ~1 V

        for i in range(steps + 1):
            voltage = voltage_start + (voltage_end - voltage_start) * i / steps
            current = 0.0  # A, no discharge/charge

            # Encode current (int16_t)
            current_can = int(current * 65.535)
            # Encode voltage (uint16_t)
            voltage_can = int(voltage * 1000)

            # Send current over CAN 0x450
            send_message(0x450, [
                current_can & 0xFF,
                (current_can >> 8) & 0xFF,
                0,0,0,0,0,0
            ])

            # Send voltage over CAN 0x623
            send_message(0x623, [
                voltage_can & 0xFF,
                (voltage_can >> 8) & 0xFF,
                0,0,0,0,0,0
            ])

            print(f"Sent Voltage: {voltage:.1f} V, Current: {current:.1f} A")
            time.sleep(0.5)  # wait 0.5s between steps

    def script_send_speed_kmh(self, can_bus):
        """Send speed kmh from 1 to 100 over 10 seconds."""
        wheel_radius = 0.283  # meters
        for speed in range(1, 101):
            # Reverse engineer RPM from speed (v = w*r)
            rpm_val = int((speed * 60) / (2 * 3.14159265 * wheel_radius * 3.6)) + 1

            # Pack RPM into data[4] and data[5]
            data4 = (rpm_val & 0x1F) << 3        # bits 0-4 of rpm into bits 3-7
            data5 = (rpm_val >> 5) & 0x7F        # bits 5-11 of rpm into bits 0-6

            # Create 8-byte payload (other bytes can be 0)
            payload = [0x00, 0x00, 0x00, 0x00, data4, data5, 0x00, 0x00]

            send_message(0x08850225, payload, isextended_id=True)
            time.sleep(0.5)

    def script_send_temperature(self, can_bus):
        for temp in range(0, 99):
            send_message(0x650, [temp])
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

if(__name__ == "__main__"):
    with can.interface.Bus(channel='can0', interface='socketcan', bitrate=500000) as bus:
        print("CAN bus initialized")
        send_message(0x623, [0xFF,0xFF,0,0,0,0,0,0])
        send_message(0x450, [0xFF,0xFF,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x623, [0,0,0,0,0,0,0,0])
        send_message(0x450, [0,0,0,0,0,0,0,0])
        # drd_test = DRDTest(bus)
        # drd_test.script_send_soc(drd_test.bus)
        print("Test completed")