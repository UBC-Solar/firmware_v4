#!/usr/bin/env python3
"""Interactive Digi XBee3 Cellular configurator (transparent/AT mode).

Requires: pip install pyserial
Start: python3 xbee_configurator.py

The module must be on a USB serial adapter accessible to this host. This
program does not configure Soracom Harvest or any remote cloud backend.
"""
import ipaddress
import re
import time
from contextlib import contextmanager
from serial import Serial, SerialException
from serial.tools import list_ports

BAUD_CODES = {9600: '3', 19200: '4', 38400: '5', 57600: '6',
              115200: '7', 230400: '8', 460800: '9', 921600: 'A'}
COMMON_BAUDS = (230400, 9600, 115200, 57600, 38400, 19200, 460800, 921600)
READ_FIELDS = [('VR', 'Firmware version'), ('HV', 'Hardware version'),
               ('AP', 'UART operating mode'), ('BD', 'UART baud code'),
               ('NB', 'Parity'), ('SB', 'Stop bits'), ('D7', 'CTS flow control'),
               ('D6', 'RTS flow control'), ('IP', 'IP protocol'),
               ('DL', 'Destination address'), ('DE', 'Destination port (hex)'),
               ('AN', 'Configured APN'), ('OA', 'Operating APN'),
               ('CP', 'Carrier profile'), ('AI', 'Association status'),
               ('MN', 'Mobile network'), ('MY', 'Assigned IP'),
               ('DB', 'Signal strength'), ('RJ', 'Registration reject cause')]
# Only use arbitrary writing for parameters approved here. Read-only diagnostics
# are intentionally separate. Carrier/band changes can interrupt service.
EDITABLE = {'AN', 'CP', 'IP', 'DL', 'DE', 'AP', 'BD', 'NB', 'SB', 'D6', 'D7'}

def ask(prompt, default=None):
    suffix = f' [{default}]' if default is not None else ''
    v = input(f'{prompt}{suffix}: ').strip()
    return v if v else default

def ports():
    return sorted(list(list_ports.comports()), key=lambda p: p.device)

def choose_port():
    found = ports()
    if not found:
        raise RuntimeError('No serial devices found. Connect the XBee development kit.')
    print('\nSerial ports:')
    for i, p in enumerate(found, 1):
        print(f' {i}. {p.device}  {p.description} (VID={p.vid}, PID={p.pid})')
    recommended = [i for i, p in enumerate(found, 1) if p.vid == 0x0403 and p.pid == 0x6001]
    default = recommended[0] if len(recommended) == 1 else (1 if len(found) == 1 else None)
    n = int(ask('Select port number', default))
    if not 1 <= n <= len(found):
        raise ValueError('Invalid port selection')
    return found[n - 1].device

def read_line(ser, timeout=2.0):
    old = ser.timeout
    ser.timeout = timeout
    try:
        return ser.read_until(b'\r').strip().decode('ascii', errors='replace')
    finally:
        ser.timeout = old

def enter_command(ser):
    # XBee +++ has guard times (silence before and after).
    ser.reset_input_buffer()
    time.sleep(1.2)
    ser.write(b'+++')
    ser.flush()
    time.sleep(1.2)
    result = read_line(ser, 2.0)
    if result != 'OK':
        raise RuntimeError(f'Could not enter AT command mode (response={result!r}). Check baud/AP mode.')

def at(ser, key, value=None, *, expect_ok=False):
    if not re.fullmatch(r'[A-Z0-9]{2}', key):
        raise ValueError('AT parameter must be two alphanumeric characters')
    if value is not None and ('\r' in value or '\n' in value):
        raise ValueError('Invalid AT parameter value')
    ser.reset_input_buffer()
    ser.write(('AT' + key + (value or '') + '\r').encode('ascii'))
    ser.flush()
    result = read_line(ser, 2.5)
    if not result or result == 'ERROR' or (expect_ok and result != 'OK'):
        raise RuntimeError(f'AT{key}: {result!r}')
    return result

@contextmanager
def command_session(port, baud):
    with Serial(port, baudrate=baud, timeout=0.5, write_timeout=2) as ser:
        enter_command(ser)
        try:
            yield ser
        finally:
            # CN can fail if the port disappeared or the baud was already
            # changed; callers for baud changes handle CN themselves.
            try:
                at(ser, 'CN', expect_ok=True)
            except (RuntimeError, SerialException, OSError):
                pass

def detect_baud(port):
    print('Detecting UART speed (do not send data from another program)...')
    for baud in COMMON_BAUDS:
        try:
            with command_session(port, baud) as ser:
                mode = at(ser, 'AP')
                if mode != '0':
                    print(f'Warning: AP={mode}; transparent-mode features may not work.')
                print(f'Connected at {baud} baud.')
                return baud
        except (RuntimeError, SerialException, OSError):
            continue
    raise RuntimeError('No response at common baud rates. Close other serial apps and check XBee power.')

def status(port, baud):
    with command_session(port, baud) as ser:
        print('\n--- XBee status ---')
        for key, label in READ_FIELDS:
            try:
                val = at(ser, key)
                if key == 'DE':
                    try:
                        val += f' (decimal {int(val, 16)})'
                    except ValueError:
                        pass
                print(f'{label:25s} AT{key}: {val}')
            except RuntimeError as exc:
                print(f'{label:25s} AT{key}: unavailable ({exc})')
    print('\nNote: AI=0 normally indicates successful cellular association;')
    print('a configured endpoint does not guarantee that a TCP session is connected.')

def apply_settings(port, baud, settings):
    # Validate all values before writing anything; write to flash only after
    # all modem commands succeed. If one fails, prior values may be staged.
    with command_session(port, baud) as ser:
        before = {}
        for key, value in settings.items():
            before[key] = at(ser, key)
            print(f'AT{key}: {before[key]} -> {value}')
        if ask('Apply and save these changes? (yes/no)', 'no').lower() != 'yes':
            print('Cancelled (no changes written).')
            return False
        updated = []
        try:
            for key, value in settings.items():
                at(ser, key, value, expect_ok=True)
                updated.append(key)
            for key, value in settings.items():
                readback = at(ser, key)
                print(f'  Readback AT{key}: {readback}')
            at(ser, 'WR', expect_ok=True)
            print('Saved to XBee flash (ATWR).')
            return True
        except Exception:
            print('A command failed. Attempting to restore prior values (not guaranteed).')
            for key in updated:
                try:
                    at(ser, key, before[key], expect_ok=True)
                except Exception:
                    pass
            raise

def configure_tcp(port, baud, echo=False):
    if echo:
        host, p = '45.79.112.203', 4242
        print('Selecting public TCP echo test destination (service availability not guaranteed).')
    else:
        host = ask('Destination IPv4 address or hostname')
        p = int(ask('TCP destination port (decimal)'))
    if not host or not 1 <= p <= 65535:
        raise ValueError('Enter a valid host and port 1..65535')
    try:
        ipaddress.ip_address(host)
    except ValueError:
        if len(host) > 253 or not re.fullmatch(r'[a-zA-Z0-9][a-zA-Z0-9.-]*', host):
            raise ValueError('Invalid hostname')
    # IP=1 is TCP for this XBee3 firmware; DL can contain dotted IPv4 / hostname.
    apply_settings(port, baud, {'IP': '1', 'DL': host, 'DE': f'{p:X}'})

def configure_cellular(port, baud):
    print('Changing APN or carrier profile may disconnect cellular service.')
    print('Leave a field blank to keep its existing value.')
    with command_session(port, baud) as ser:
        current_an = at(ser, 'AN')
        current_cp = at(ser, 'CP')
    print(f'Current APN: {current_an}; carrier profile: {current_cp}')
    apn = ask('New APN (blank = unchanged)', '')
    cp = ask('New carrier profile code (blank = unchanged)', '')
    changes = {}
    if apn:
        if len(apn) > 100 or not re.fullmatch(r'[A-Za-z0-9._-]+', apn):
            raise ValueError('Invalid APN')
        changes['AN'] = apn
    if cp:
        if not re.fullmatch(r'[0-9A-Fa-f]{1,2}', cp):
            raise ValueError('Carrier profile must be a hexadecimal code')
        changes['CP'] = cp.upper()
    if changes and apply_settings(port, baud, changes):
        print('Allow time for network reconnection. A carrier-profile change may require a reboot.')
    elif not changes:
        print('No changes requested.')

def change_baud(port, baud):
    print('Available speeds:', ', '.join(map(str, BAUD_CODES)))
    new = int(ask('New UART baud rate (decimal)', 230400))
    if new not in BAUD_CODES:
        raise ValueError('Unsupported choice in this tool')
    if new == baud:
        print('Already at that speed.')
        return baud
    with Serial(port, baudrate=baud, timeout=0.5, write_timeout=2) as ser:
        enter_command(ser)
        print(f'Changing from {baud} to {new}; connection will be reopened.')
        if ask('Proceed? (yes/no)', 'no').lower() != 'yes':
            at(ser, 'CN', expect_ok=True)
            return baud
        at(ser, 'BD', BAUD_CODES[new], expect_ok=True)
        at(ser, 'WR', expect_ok=True)
        at(ser, 'CN', expect_ok=True)
    time.sleep(1)
    with command_session(port, new) as ser:
        code = at(ser, 'BD')
        if code.upper() != BAUD_CODES[new]:
            raise RuntimeError(f'Baud verification failed: expected {BAUD_CODES[new]}, got {code}')
    print(f'UART speed verified: {new} baud.')
    return new

def test_echo(port, baud):
    payload = b'{"can_id":1878,"data":"0102030405060708"}\n'
    with command_session(port, baud) as ser:
        mode = at(ser, 'AP')
        dest = at(ser, 'DL')
        port_hex = at(ser, 'DE')
        protocol = at(ser, 'IP')
        if mode != '0' or protocol != '1':
            raise RuntimeError(f'Requires transparent TCP: AP={mode}, IP={protocol}')
        if dest != '45.79.112.203' or int(port_hex, 16) != 4242:
            print(f'Current endpoint is {dest}:{int(port_hex,16)} (not known echo server).')
            if ask('Test anyway? (yes/no)', 'no').lower() != 'yes':
                return
    with Serial(port, baudrate=baud, timeout=0.4, write_timeout=2) as ser:
        time.sleep(0.3)
        ser.reset_input_buffer()
        ser.write(payload)
        ser.flush()
        received = bytearray()
        deadline = time.monotonic() + 18
        while time.monotonic() < deadline:
            received.extend(ser.read(256))
            if b'\n' in received:
                break
        print('Sent:    ', repr(payload))
        print('Received:', repr(bytes(received)))
        print('Echo match:', bytes(received) == payload)
        if bytes(received) != payload:
            print('A mismatch/timeout does not by itself prove modem failure; check TCP endpoint.')

def main():
    print('=== UBC Solar | Digi XBee3 Cellular Configurator ===')
    port = choose_port()
    baud = detect_baud(port)
    while True:
        print(f'\nDevice: {port} @ {baud} baud')
        print(' 1) Show modem configuration and diagnostics')
        print(' 2) Configure TCP destination')
        print(' 3) Set public TCP echo test profile')
        print(' 4) Configure APN / carrier profile')
        print(' 5) Change UART baud rate')
        print(' 6) Run TCP echo test')
        print(' 7) Read an AT parameter')
        print(' 8) Set an advanced AT parameter (restricted)')
        print(' 9) Re-detect baud rate')
        print(' 0) Exit')
        selection = ask('Selection')
        try:
            if selection == '1':
                status(port, baud)
            elif selection == '2':
                configure_tcp(port, baud)
            elif selection == '3':
                configure_tcp(port, baud, echo=True)
            elif selection == '4':
                configure_cellular(port, baud)
            elif selection == '5':
                baud = change_baud(port, baud)
            elif selection == '6':
                test_echo(port, baud)
            elif selection == '7':
                key = ask('AT parameter (2 characters)').upper()
                with command_session(port, baud) as ser:
                    print(f'AT{key}: {at(ser,key)}')
            elif selection == '8':
                key = ask('AT parameter (2 characters)').upper()
                if key not in EDITABLE or key in {'BD', 'AP'}:
                    raise ValueError('Use the dedicated menu for baud or mode, or unsupported parameter')
                value = ask('Value (AT format; hex for numeric parameters)')
                if not value or not re.fullmatch(r'[A-Za-z0-9.\-_:]+', value):
                    raise ValueError('Invalid value')
                apply_settings(port, baud, {key: value})
            elif selection == '9':
                baud = detect_baud(port)
            elif selection == '0':
                print('Goodbye.')
                return
            else:
                print('Invalid menu option.')
        except (RuntimeError, SerialException, OSError, ValueError, UnicodeEncodeError) as exc:
            print('ERROR:', exc)

if __name__ == '__main__':
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print('\nExited.')
    except (RuntimeError, SerialException, OSError, ValueError) as exc:
        print('ERROR:', exc)
        raise SystemExit(1)
