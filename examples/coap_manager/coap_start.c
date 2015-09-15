/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>

#include <sys/ioctl.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "native_internal.h"

#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include "coap.h"
#include <pthread.h>

#define PORT 5683

void *coap_client_start(void *arg);

int coap_client_setup(int argc, char **argv)
{
//READ IN HOW MANY TAP DEVICE TO CREATE (MAX 9999)
    if(argc<2)
    return -1;
    int tap_num;
    if(strlen(argv[1]) == 1){
	tap_num=(int)(argv[1][0] - '0');
    }
    else if (strlen(argv[1]) == 2){
	tap_num=10*(int)(argv[1][0] - '0')+(int)(argv[1][1] - '0');
    }
    else if (strlen(argv[1]) == 3){
	tap_num=100 * (int)(argv[1][0] - '0') + 10 * (int)(argv[1][1] - '0') + (int)(argv[1][2] - '0');
    }
    else if (strlen(argv[1]) == 4){
	tap_num= 1000 * (int)(argv[1][0] - '0') + 100 * (int)(argv[1][1] - '0') + 10*(int)(argv[1][2] - '0') + (int)(argv[1][3] - '0');
    }
    else
	return -1;
//CREATE TAP DEVICE
printf("tap_num= %d\n",tap_num);
    char num[10];
    char comm[50];
    int status;
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);
    sprintf(num,"%d",tap_num);
//Delete previous Taps
    
    strcpy(comm,"ip link delete tap");
    strcat(comm,num);
    status=system(comm);
//Create Tap devices
    strcpy(comm,"ip tuntap add tap");
    strcat(comm,num);
    strcat(comm," mode tap user ${USER}");
    status=system(comm);
    
//Set Tap up
    strcpy(comm,"ip link set tap");
    strcat(comm,num);
    strcat(comm," up");
    status=system(comm);
//Add Global IP to TAP
    strcpy(comm,"ip -6 addr add 3000::1111:2222:3333:");
    strcat(comm,argv[1]);
    strcat(comm,"/64 dev tap");
    strcat(comm,argv[1]);
    status=system(comm);
    (void)status;

    puts("Starting the RIOT\n");
    int fd,tap_fd;
    const char *clonedev = "/dev/net/tun";
    char name[] = "tap";
    strcat(name,argv[1]);
printf("%s\n",name);

    struct sockaddr_in6 servaddr;
    struct ifreq ifr;
    

    fd = socket(AF_INET6,SOCK_DGRAM,0);//Socket file descriptor init

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;//inet family
    servaddr.sin6_flowinfo = 0;//??

    servaddr.sin6_addr.s6_addr[0] = (uint8_t)0x30;//IPv6 Address 1
    servaddr.sin6_addr.s6_addr[1] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[2] = (uint8_t)0x00;//IPv6 Address 2
    servaddr.sin6_addr.s6_addr[3] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[4] = (uint8_t)0x00;//IPv6 Address 3
    servaddr.sin6_addr.s6_addr[5] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[6] = (uint8_t)0x00;//IPv6 Address 4
    servaddr.sin6_addr.s6_addr[7] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[8] = (uint8_t)0x11;//IPv6 Address 5
    servaddr.sin6_addr.s6_addr[9] = (uint8_t)0x11;
    servaddr.sin6_addr.s6_addr[10] = (uint8_t)0x22;//IPv6 Address 6
    servaddr.sin6_addr.s6_addr[11] = (uint8_t)0x22;
    servaddr.sin6_addr.s6_addr[12] = (uint8_t)0x33;//IPv6 Address 7
    servaddr.sin6_addr.s6_addr[13] = (uint8_t)0x33;
    servaddr.sin6_addr.s6_addr[14] = (uint8_t)0x00;//IPv6 Address 8 //TODO
    servaddr.sin6_addr.s6_addr[15] = (uint8_t)tap_num;
printf("inet6 = %d\n", (uint8_t)tap_num);



    servaddr.sin6_port = htons(PORT);		//PORT (5683)
    bind(fd,(struct sockaddr *)&servaddr, sizeof(servaddr));

//Set TAP device up, give it local ipv6 address
    /* implicitly create the tap interface */
    if ((tap_fd = real_open(clonedev , O_RDWR)) == -1) {
        err(EXIT_FAILURE, "open(%s)", clonedev);
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if (real_ioctl(tap_fd, TUNSETIFF, (void *)&ifr) == -1) {
        _native_in_syscall++;
        warn("ioctl TUNSETIFF");
        warnx("probably the tap interface (%s) does not exist or is already in use", name);
        real_exit(EXIT_FAILURE);
    }


    int fd_tap = fd;
    struct sockaddr_in6 cliaddr;
    uint8_t buf[4096];
    uint8_t scratch_raw[4096];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
    endpoint_setup();
    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;
        n = recvfrom(fd_tap, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
//#ifdef DEBUG
        printf("Received: ");
        coap_dump(buf, n, true);
        printf("\n");
//#endif

        if (0 != (rc = coap_parse(&pkt, buf, n)))
            printf("Bad packet rc=%d\n", rc);
        else
        {
            size_t rsplen = sizeof(buf);
            coap_packet_t rsppkt;
#ifdef DEBUG
            coap_dumpPacket(&pkt);
#endif
            coap_handle_req(&scratch_buf, &pkt, &rsppkt);

            if (0 != (rc = coap_build(buf, &rsplen, &rsppkt)))
                printf("coap_build failed rc=%d\n", rc);
            else
            {
#ifdef DEBUG
                printf("Sending: ");
                coap_dump(buf, rsplen, true);
                printf("\n");
#endif
#ifdef DEBUG
                coap_dumpPacket(&rsppkt);
#endif

                sendto(fd_tap, buf, rsplen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
    }
}
