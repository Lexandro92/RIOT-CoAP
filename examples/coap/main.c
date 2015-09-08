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

#include "coap.h"

#define PORT 5683

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("Starting the RIOT\n");
    int fd;
    struct sockaddr_in6 servaddr, cliaddr;
    uint8_t buf[4096];//maybe need bigger becouse IPv6
    uint8_t scratch_raw[4096];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};

    fd = socket(AF_INET6,SOCK_DGRAM,0);//Socket file descriptor init


    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;//inet family
    servaddr.sin6_flowinfo = 0;//??

    servaddr.sin6_addr.s6_addr[0] = (uint8_t)0x20;//IPv6 Address 1
    servaddr.sin6_addr.s6_addr[1] = (uint8_t)0x01;
    servaddr.sin6_addr.s6_addr[2] = (uint8_t)0x0d;//IPv6 Address 2
    servaddr.sin6_addr.s6_addr[3] = (uint8_t)0xb8;
    servaddr.sin6_addr.s6_addr[4] = (uint8_t)0x00;//IPv6 Address 3
    servaddr.sin6_addr.s6_addr[5] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[6] = (uint8_t)0x00;//IPv6 Address 4
    servaddr.sin6_addr.s6_addr[7] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[8] = (uint8_t)0x00;//IPv6 Address 5
    servaddr.sin6_addr.s6_addr[9] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[10] = (uint8_t)0x00;//IPv6 Address 6
    servaddr.sin6_addr.s6_addr[11] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[12] = (uint8_t)0x00;//IPv6 Address 7
    servaddr.sin6_addr.s6_addr[13] = (uint8_t)0x00;
    servaddr.sin6_addr.s6_addr[14] = (uint8_t)0x00;//IPv6 Address 8
    servaddr.sin6_addr.s6_addr[15] = (uint8_t)0x02;



    servaddr.sin6_port = htons(PORT);		//PORT (5683)
    bind(fd,(struct sockaddr *)&servaddr, sizeof(servaddr));

    endpoint_setup();

    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;

        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
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

                sendto(fd, buf, rsplen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
    }
}
