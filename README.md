##LoRa Peer to Peer link for message and command delivering

### Requirements: libmosquitto-dev, uuid-dev

### Examples:
Send JSON message usding serial0 interface
./lrd -s /dev/serial0 -d boat,2,cat,1
Receive JSON message usding ttyUSB0 interface
./lrd -s /dev/ttyUSB0 -r
