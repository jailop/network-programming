# Replacing MAC address

This program is equivalent to this sequence of commands (using wlan0 as an
example):

```bash
sudo ip link set wlan0 down
sudo macchanger -r wlan0
sudo ip link set wlan0 up
```
