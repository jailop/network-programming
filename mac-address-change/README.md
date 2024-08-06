# Replacing MAC address with random values

To use this program (replace `wlan0` with the appropiate device name):

```bash
make
sudo ./macrandchg wlan0
```

A possible output is:

```
Current MAC address: 68:8D:BA:F6:51:57
MAC address was succesfully changed to: B4:A0:13:58:E8:F3
```

This program is equivalent to this sequence of commands using `macchanger` tool:

```bash
sudo ip link set wlan0 down
sudo macchanger -r wlan0
sudo ip link set wlan0 up
```

References:

* macchanger <https://github.com/alobbs/macchanger>
* netdevice <https://www.man7.org/linux/man-pages/man7/netdevice.7.html>
