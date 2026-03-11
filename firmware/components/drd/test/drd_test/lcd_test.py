import can
import time
import threading

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
        send_message(0x580, [0x04])  # Command to switch to drive page
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

        print("Testing Battery Overvoltage, Charge OC")
        send_message(0x622, [(1<<4)|(1<<6),0,0,0,0,0,0,0])
        time.sleep(1)
        send_message(0x622, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        # Not working
        print("Testing ECU RFW")
        send_message(0x450, [0,0,0,0,0,(1<<4),0,0])
        time.sleep(1)
        send_message(0x450, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Motor System Error")
        send_message(0x08A50225, [0,0,0,1,0,0,0,0])
        time.sleep(1)
        send_message(0x08A50225, [0,0,0,0,0,0,0,0])
        time.sleep(1)

        print("Testing Motor Comm Fault and throttle_ADC_out_of_range and throttle_ADC_mismatch")
        send_message(0x403, [1 << 37 | 1 << 34 | 1 << 35]) # Motor Comm Fault, throttle_ADC_out_of_range, throttle_ADC_mismatch
        time.sleep(1)  # Wait for the LCD to update
        send_message(0x403, [0 << 37 | 0 << 34 | 0 << 35]) # Motor Comm Fault, throttle_ADC_out_of_range, throttle_ADC_mismatch
        time.sleep(1)  # Wait for the LCD to update

        print("Testing 8 faults on page")
        send_message(0x622, [0xFF]) # All battery faults
        time.sleep(1)  # Wait for the LCD to update
        send_message(0x450, [1 << 44]) # ECU RFW
        send_message(0x623, [135]) # VOLT_HI
        send_message(0x623, [85]) # VOLT_LO
        send_message(0x08A50225, [1 << 24 | 1 << 17 | 1 << 19 | 1 << 3]) # All motor faults
        time.sleep(1)  # Wait for the LCD to update
        send_message(0x08A50225, [1 << 24 | 1 << 17 | 1 << 19 | 1 << 3]) # All motor faults
        send_message(0x403, [1 << 37 | 1 << 34 | 1 << 35]) # Motor Comm Fault, throttle_ADC_out_of_range, throttle_ADC_mismatch
        time.sleep(1)  # Wait for the LCD to update

    def test_warning_page(self):
        print("Testing 3 warnings on page")
        send_message(0x622, [1 << 13 | 1 << 14 | 1 << 15]) # LOW_VOLT, HIGH_VOLT, LOW_TEMP (Low Voltage, High Voltage, Low Temperature Warning)
        time.sleep(1)  # Wait for the LCD to update
        send_message(0x622, [1 << 16 | 1 << 18]) # HIGH_TEMP, NO_MSG (High Temperature Warning, No ECU Current Message Received Warning)
        time.sleep(1)  # Wait for the LCD to update
        send_message(0x450, [1 << 40 | 1 << 41]) # PCK_OD, PCK_OC (Pack Overdischarge, Pack Overcharge)
        time.sleep(1)  # Wait for the LCD to update

        print("Testing all warnings on page")
        send_message(0x622, [1 << 13 | 1 << 14 | 1 << 15 | 1 << 16 | 1 << 18]) # All warnings
        send_message(0x450, [1 << 40 | 1 << 41]) # PCK_OD, PCK_OC (Pack Overdischarge, Pack Overcharge)
        time.sleep(1)  # Wait for the LCD to update

    def test_debug_page(self):
        time.sleep(1)  # Wait for the LCD to initialize
        t1 = threading.Thread(target=self.script_send_pack_current_and_voltage, args=(self.bus,))
        t1.start()
        t1.join()

    # SETS SPEED TO 0 SO DRIVE STATE CAN BE SWITCHED
    def script_test_drive_state(self, can_bus):
        send_message(0x08850225, [00], isextended_id=True)

    def script_send_soc(self, can_bus):
        """Send SoC message from 100% to 0% over 30 seconds."""
        for soc in range(100, -1, -1):
            send_message(0x624, [soc])
            time.sleep(0.3)
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
        drd_test = DRDTest(bus)
        while True:
            drd_test.run_test()
        drd_test.run_test()
        print("Test completed")