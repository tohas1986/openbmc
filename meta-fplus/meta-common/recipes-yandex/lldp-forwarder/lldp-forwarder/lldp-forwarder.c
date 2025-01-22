/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#define DEFAULT_IF_IN	"lan0"
#define DEFAULT_IF_OUT	"eth0"
#define BUF_SIZE		1024

bool DEBUG = false;

int send_frame(uint8_t * ptr_buf, int len, char * ifName)
{
	int sockfd, rc;
	struct ifreq if_idx;
	struct sockaddr_ll socket_address;

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("SIOCGIFINDEX");

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = 0xff;
	socket_address.sll_addr[1] = 0xff;
	socket_address.sll_addr[2] = 0xff;
	socket_address.sll_addr[3] = 0xff;
	socket_address.sll_addr[4] = 0xff;
	socket_address.sll_addr[5] = 0xff;
	/* Send packet */
	rc = sendto(sockfd, ptr_buf, len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
	if (rc < 0)
		printf("Send failed rc=%d errno=%d\n", rc, errno);

	close(sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, ret;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts;	/* set promiscuous mode */
	uint8_t buf[BUF_SIZE];
	char ifName_in[IFNAMSIZ];
	char ifName_out[IFNAMSIZ];
	
	/* Get interface name */
	if (argc > 2) {
		strcpy(ifName_in, argv[1]);
		strcpy(ifName_out, argv[2]);
	} else {
		strcpy(ifName_in, DEFAULT_IF_IN);
		strcpy(ifName_out, DEFAULT_IF_OUT);
	}

	/* Open PF_PACKET socket, listening for EtherType 0x88cc */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_LLDP))) == -1) {
		perror("listener: socket");	
		return -1;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName_in, IFNAMSIZ);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	int rc = ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	if (rc != 0)
	{
		printf("Set to promisc mode failed, rc=%d\n", rc);
	}
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		shutdown(sockfd, SHUT_RDWR);
		exit(EXIT_FAILURE);
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName_in, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		shutdown(sockfd, SHUT_RDWR);
		exit(EXIT_FAILURE);
	}

	while(1) {
		if (DEBUG) printf("listener: Waiting to recvfrom...\n");
		numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, NULL, NULL);
		if (DEBUG) printf("listener: got packet %lu bytes\n", numbytes);

		if (DEBUG) printf("Sending frame\n");
		send_frame(&buf[0], numbytes, ifName_out);
	}

	shutdown(sockfd, SHUT_RDWR);
	return ret;
}

