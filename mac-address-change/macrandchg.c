#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_arp.h>
#include <time.h>

struct netdev {
    struct ifreq ifdev;
    int sock;
};

/**
 * A utility function to print MAC addresses
 */
void print_mac(char *addr) {
    for (int i = 0; i < 6; i++) {
        printf("%X", addr[i] & 0xff);
        if (i < 5) printf(":");
    }
    printf("\n");
}

int netdev_init(struct netdev *dev, const char *name) {
    dev->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (dev->sock == -1) {
        perror("socket");
        return -1;
    }
    strncpy(dev->ifdev.ifr_name, name, IFNAMSIZ);
    return 0;
}

int netdev_deinit(struct netdev *dev) {
    return close(dev->sock);
}

int netdev_down(struct netdev *dev) {
    // Getting flags status
    if (ioctl(dev->sock, SIOCGIFFLAGS, &(dev->ifdev)) == -1) {
        perror("Getting flags");
        return -1;
    }
    // Getting down the network interface
    dev->ifdev.ifr_flags &= ~IFF_UP;
    if (ioctl(dev->sock, SIOCSIFFLAGS, &(dev->ifdev)) == -1) {
        perror("Setting flags");
        return -1;
    }
    return 0;
}

int netdev_up(struct netdev *dev) {
    // Getting flags status
    if (ioctl(dev->sock, SIOCGIFFLAGS, &(dev->ifdev)) == -1) {
        perror("Getting flags");
        return -1;
    }
    // Getting down the network interface
    dev->ifdev.ifr_flags |= IFF_UP;
    if (ioctl(dev->sock, SIOCSIFFLAGS, &(dev->ifdev)) == -1) {
        perror("Setting flags");
        return -1;
    }
    return 0;
}

int netdev_randomize_mac(struct netdev *dev) {
    dev->ifdev.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    if (ioctl(dev->sock, SIOCGIFHWADDR, &(dev->ifdev)) == -1) {
        perror("ioctl");
        return -1;
    }
    printf("Current MAC address: ");
    print_mac(dev->ifdev.ifr_hwaddr.sa_data);
    // Overwriting the mac address with random numbers
    srand(time(NULL));
    for (int i = 0; i < 6; i++) {
        dev->ifdev.ifr_hwaddr.sa_data[i] = (random() % 0xff) & (i == 0 ? 0xFC : 0xFF);
    }
    if (ioctl(dev->sock, SIOCSIFHWADDR, &(dev->ifdev)) == -1) {
        perror("Changing mac address");
        return -1;
    }
    printf("MAC address was succesfully changed to: ");
    print_mac(dev->ifdev.ifr_hwaddr.sa_data);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2)  {
        printf("Usage: %s IFNAME\n", argv[0]);
        return EXIT_FAILURE;
    }
    struct netdev dev;
    int ret = 0;
    ret |= netdev_init(&dev, argv[1]);
    ret |= netdev_down(&dev);
    ret |= netdev_randomize_mac(&dev);
    ret |= netdev_up(&dev);
    ret |= netdev_deinit(&dev);
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
