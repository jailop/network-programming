# Replacing MAC address with random values

To use this program:

```bash
make
sudo ./macrandchg wlan0
```

This program is equivalent to this sequence of commands (using wlan0 as an
example):

```bash
sudo ip link set wlan0 down
sudo macchanger -r wlan0
sudo ip link set wlan0 up
```

References:

* macchanger <https://github.com/alobbs/macchanger>
* netdevice <https://www.man7.org/linux/man-pages/man7/netdevice.7.html>
