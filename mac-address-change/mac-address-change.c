#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_arp.h>

#define NETDEV "wlan0" // Network interface to be configured

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

int main() {
    // Creating a socket to access the network interface
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct ifreq netdev; // To retrieve/set parameters
    // Assigning the network interface name
    strncpy(netdev.ifr_name, NETDEV, IFNAMSIZ);
    // Getting flags status
    if (ioctl(sockfd, SIOCGIFFLAGS, &netdev) == -1) {
        perror("Getting flags");
        exit(EXIT_FAILURE);
    }
    // Getting down the network interface
    netdev.ifr_flags &= ~IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &netdev) == -1) {
        perror("Setting flags");
        exit(EXIT_FAILURE);
    }
    // Retrieving current MAC address
    if (ioctl(sockfd, SIOCGIFHWADDR, &netdev) == -1) {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("Current MAC address: ");
    print_mac(netdev.ifr_hwaddr.sa_data);
    // Overwriting the mac address with random numbers
    for (int i = 0; i < 6; i++) {
        netdev.ifr_hwaddr.sa_data[i] = rand() % 0xff;
    }
    netdev.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    if (ioctl(sockfd, SIOCSIFHWADDR, &netdev) == -1) {
        perror("Changing mac address");
        exit(EXIT_FAILURE);
    }
    printf("MAC address was succesfully changed to: ");
    print_mac(netdev.ifr_hwaddr.sa_data);
    // Getting up the network interface
    netdev.ifr_flags &= IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &netdev) == -1) {
        perror("Getting up the network device");
        exit(EXIT_FAILURE);
    }
    return 0;
}
