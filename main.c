#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <netinet/ether.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#define zero(ifreq_a) memset(&ifreq_a, 0, sizeof(ifreq_a))

#define test "test"

//char POLY1 = 0x04C11DB7;
//char POLY2 = 0xEDB88320;
//char POLY3 = 0xDB710641;
//char POLY4 = 0x82608EDB;
char *if_name = "ens38";
char DESTMAC0 = 0x44;
char DESTMAC1 = 0x8A;
char DESTMAC2 = 0x5B;
char DESTMAC3 = 0x6E;
char DESTMAC4 = 0xEE;
char DESTMAC5 = 0x89;
int total_length = 0;

#define ETH_SIZE 1512

#define CRC_POLY 0xEDB88320
//#define CRC_POLY 0xBA0DC66B

unsigned int crc32c(unsigned char message[]) {
	int i, j;
	unsigned int byte, temp, crc, mask;
	i = 0;
	crc = 0xFFFFFFFF;
	while (message[i] != 0) {
		int resu = strtoul(message[i], NULL, 16);
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0x8F6E37A0 & mask);
		}
		i = i + 1;
	}
	return (~crc);
}

uint32_t crc32_calc(uint8_t *data, int len) {
	int i, j;
	uint32_t crc;
	int newlen = len - 4;
	crc = 0xFFFFFFFF;
	for (j = 0; j < newlen; j++) {
		crc ^= data[j];
		for (i = 0; i < 8; i++) {
			crc = (crc & 1) ? ((crc >> 1) ^ CRC_POLY) : (crc >> 1);
		}
	}
	return (crc ^ 0xFFFFFFFF);
}

uint32_t crc32b(uint8_t *message, int len) {
	int i, j;
	uint32_t crc, mask;
	uint8_t byte;
	crc = 0xFFFFFFFF;
	int newlen = len - 4;
	for (j = 0; j < newlen; j++) {
		byte = message[i];
		crc = crc ^ byte;
		for (i = 7; i >= 0; i--) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}

	}
	return (crc ^ 0xFFFFFFFF);
}

int main() {

	int sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if (sockfd == -1)
		puts("Failed to create socket");
	puts("Created socket");
	struct ifreq ifreq_ip, ifreq_index, ifreq_mac;
	zero(ifreq_ip);
	strncpy(ifreq_ip.ifr_name, if_name, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFADDR, &ifreq_ip) < 0) {
		puts("Error in getting IP address");
	}

// getting index of the interface to send a packet
	zero(ifreq_index);
	strncpy(ifreq_index.ifr_name, if_name, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFINDEX, &ifreq_index) < 0)
		puts("Failed to get index of interface");

	printf("Source interface: %s\n", ifreq_index.ifr_name);

// getting MAC address

	zero(ifreq_mac);
	strncpy(ifreq_mac.ifr_name, if_name, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifreq_mac) < 0)
		puts("Failed to get MAC address");
	unsigned char *sendbuff = (unsigned char*) malloc(1518);
	memset(sendbuff, 0, ETH_SIZE);
	uint8_t bytearr[ETH_SIZE];

// Constructing the Eth header

	struct ethhdr *eth = (struct ethhdr*) (sendbuff);
	eth->h_source[0] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[0]);
	eth->h_source[1] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[1]);
	eth->h_source[2] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[2]);
	eth->h_source[3] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[3]);
	eth->h_source[4] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[4]);
	eth->h_source[5] = (unsigned char) (ifreq_mac.ifr_hwaddr.sa_data[5]);
	printf("Source MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", eth->h_source[0],
			eth->h_source[1], eth->h_source[2], eth->h_source[3],
			eth->h_source[4], eth->h_source[5]);
	eth->h_dest[0] = DESTMAC0;
	eth->h_dest[1] = DESTMAC1;
	eth->h_dest[2] = DESTMAC2;
	eth->h_dest[3] = DESTMAC3;
	eth->h_dest[4] = DESTMAC4;
	eth->h_dest[5] = DESTMAC5;
	eth->h_proto = htons(ETH_P_IP);
	total_length += sizeof(struct ethhdr);
	struct sockaddr_ll saddr_ll1;
	saddr_ll1.sll_ifindex = ifreq_index.ifr_ifindex;
	saddr_ll1.sll_halen = ETH_ALEN;
	saddr_ll1.sll_addr[0] = DESTMAC0;
	saddr_ll1.sll_addr[1] = DESTMAC1;
	saddr_ll1.sll_addr[2] = DESTMAC2;
	saddr_ll1.sll_addr[3] = DESTMAC3;
	saddr_ll1.sll_addr[4] = DESTMAC4;
	saddr_ll1.sll_addr[5] = DESTMAC5;

	struct iphdr *iph = (struct iphdr*) (sendbuff + sizeof(struct ethhdr));
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 16;
	iph->id = htons(10201);
	iph->ttl = 64;
	iph->protocol = 17;
	iph->saddr = inet_addr(
			inet_ntoa(
					(((struct sockaddr_in*) &(ifreq_ip.ifr_addr))->sin_addr)));
	iph->daddr = inet_addr("33.22.11.20");
	total_length += sizeof(struct iphdr);

	struct udphdr *uh = (struct udphdr*) (sendbuff + sizeof(struct iphdr)
			+ sizeof(struct ethhdr));
	uh->source = htons(23451);
	uh->dest = htons(23452);
	total_length += sizeof(struct udphdr);

	sendbuff[total_length++] = 0xEE;

	uh->len = htons(
			(total_length - sizeof(struct iphdr) - sizeof(struct ethhdr)));
	iph->tot_len = htons(total_length - sizeof(struct ethhdr));

	unsigned short checksum(unsigned short *buff, int _16bitword) {
		unsigned long sum;
		for (sum = 0; _16bitword > 0; _16bitword--) {
			sum += htons(*(buff)++);
		}
		do {
			sum = ((sum >> 16) + (sum & 0xFFFF));
		} while (sum & 0xFFFF0000);
		return htons((unsigned short) (~sum));
	}

	iph->check = checksum((unsigned short*) (sendbuff + sizeof(struct ethhdr)),
			(sizeof(struct iphdr) / 2));
	for (int i = 0; i < ETH_SIZE; i++) {
		bytearr[i] = sendbuff[i];
	}
	puts("dd");
	printf("%i\n", sizeof(bytearr));
	uint32_t res = crc32b(bytearr, sizeof(bytearr));
	uint32_t res1 = crc32_calc(bytearr, sizeof(bytearr));
	printf("0x%.2x\n", res);
	printf("0x%.2x\n", res1);
	unsigned int first_two = (res1 >> 24) & 0xff;
	unsigned int second_two = (res1 >> 16) & 0xff;
	unsigned int third_two = (res1 >> 8) & 0xff;
	unsigned int final_two = res1 & 0xff;
	int frame_check_start_index = sizeof(bytearr) - 4;
	int bitvalue = 0;
	for (frame_check_start_index; frame_check_start_index < sizeof(bytearr);
			frame_check_start_index++) {
		printf("%i\n", bitvalue);
		unsigned int hexa = ((res1 >> bitvalue) & 0xff);
		sendbuff[frame_check_start_index] = hexa;
		bitvalue += 8;
	}

	int send_len = sendto(sockfd, sendbuff, ETH_SIZE, 0,
			(const struct sockaddr*) &saddr_ll1, sizeof(struct sockaddr_ll));
	printf("%i\n", send_len);

	return (0);

}
