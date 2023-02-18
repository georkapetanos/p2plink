##LoRa Peer to Peer link for message and command delivering

### Requirements: libmosquitto-dev, uuid-dev, libssl-dev

### Examples:
Send JSON message using serial0 interface
./lrd -t /dev/serial0 -s "Test message"
Encrypt and send JSON message
./lrd -t /dev/serial0 -e -s "Test message"
Receive JSON message using ttyUSB0 interface
./lrd -t /dev/ttyUSB0 -r
Receive encrypted JSON message
./lrd -t /dev/ttyUSB0 -e -r
Receive broadcasted messages from MQTT
mosquitto_sub -h hostname -u username -P password -t "lrdlink"
