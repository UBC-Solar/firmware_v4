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
# Syntax: cansend <interface> <ID>#<DATA>
cansend can0 403#01
```

### To test multiple CAN Messages with a python script:
