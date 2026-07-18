# LCD Testing
### To set up CAN Testing use the following commands:
```
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

### To dump/print out CAN Messages:
```
candump -td -H -x -c can0
```

### To inject a single CAN Message:
```
# Syntax: cansend <interface> <ID>#<DATA in HEX>
# Changes page
cansend can0 580#04
# Changes page
cansend can0 580#00
```

### To test multiple CAN Messages with a python script:
```
# Setup the venv
chmod +x setup.sh
rm -rf environment
./setup.sh
source environment/bin/activate

# run the script
python3 lcd_test.py
python3 simulate_mc.py
```

