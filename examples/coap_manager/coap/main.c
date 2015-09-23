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
#include <stdbool.h>
#include <strings.h>
#include <string.h>

#include <sys/ioctl.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include "native_internal.h"

#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include "coap.h"
#include <unistd.h>
#include <sys/syscall.h>

#define PORT 5683

void *coap_client_start(void *arg);
void RemoveSpaces(char* source);

int main(void){

//READ IN HOW MANY TAP DEVICE TO CREATE (MAX 9999)
//File locker
    struct flock fl;
    pid_t tid = syscall(SYS_gettid);
    fl.l_type   = F_RDLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = tid; /* our PID                      */

//READ THE SIZE OF TAP_CONTROL_SIZE FILE
    char  ffile_size[2];
    int file_size,tap_num;
    int fp = open("./coap/tap_control_size.txt", O_RDONLY);
    fcntl(fp, F_SETLKW, &fl);  //Locks the file for reading
    if(0 > fp)
    {
        printf("\n tap_control_size:open() Error!!!\n");
        return 1;
    }
    read(fp,ffile_size,2);
//CLOSE FILE
    if(0 > close(fp))
    {
        printf("\n tap_control_size:close() Error!!!\n");
        return 1;
    }
    fl.l_type = F_UNLCK;  /* tell it to unlock the region */
    fcntl(fp, F_SETLK, &fl); /* set the region to unlocked */

    file_size = (int)(ffile_size[0] - '0');
//OPEN FILE TO DETERMINE WHICH CLIENT IS THIS ONE
    char  file_num[file_size];
    fl.l_type   = F_RDLCK;
    fp = open("./coap/tap_control.txt", O_RDONLY);
    fcntl(fp, F_SETLKW, &fl);  //Locks the file for reading
    if(0 > fp)
    {
        printf("\n tap_control:open() Error!!!\n");
        return 1;
    }
    read(fp,file_num,file_size);
//CLOSE FILE
    if(0 > close(fp))
    {
        printf("\n tap_control:close() Error!!!\n");
        return 1;
    }
    fl.l_type = F_UNLCK;  /* tell it to unlock the region */
    fcntl(fp, F_SETLK, &fl); /* set the region to unlocked */

//CONVERT IT TO INT
    tap_num=0;
    if(file_size == 4){
	tap_num += (int)(file_num[0] - '0')*1000;
	tap_num += (int)(file_num[1] - '0')*100;
	tap_num += (int)(file_num[2] - '0')*10;
	tap_num += (int)(file_num[3] - '0');
    }
    else if(file_size == 3){
	tap_num += (int)(file_num[0] - '0')*100;
	tap_num += (int)(file_num[1] - '0')*10;
	tap_num += (int)(file_num[2] - '0');
    }
    else if(file_size == 2){
	tap_num += (int)(file_num[0] - '0')*10;
	tap_num += (int)(file_num[1] - '0');
    }
    else 
	tap_num += (int)(file_num[0] - '0');

//CREATE TAP DEVICE
    puts("Starting the RIOT\n");
    int fd;
    char name[3+file_size];
    strcpy(name,"tap");
    int i;
    for(i=0;i<file_size;i++)
    name[3+i]=file_num[i];
    name[3+file_size]=0;

    printf("TAP DEIVCE: %s\n",name);

    struct sockaddr_in6 servaddr;
//    struct ifreq ifr;//
    struct sockaddr_in6 cliaddr;
    uint8_t buf[4096];
    uint8_t scratch_raw[4096];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
   
    fd = socket(AF_INET6,SOCK_DGRAM,0);//Socket file descriptor init

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;//inet family
    servaddr.sin6_flowinfo = 0;//??

    int ip8_1=0,ip8_2=0;
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
    if(file_size == 4){
	ip8_1 += (int)(file_num[0] - '0')*16;
	ip8_1 += (int)(file_num[1] - '0');
	ip8_2 += (int)(file_num[2] - '0')*16;
	ip8_2 += (int)(file_num[3] - '0');
    }
    else if(file_size == 3){
	ip8_1 += (int)(file_num[0] - '0');
	ip8_2 += (int)(file_num[1] - '0')*16;
	ip8_2 += (int)(file_num[2] - '0');
    }
    else if(file_size == 2){
	ip8_2 += (int)(file_num[0] - '0')*16;
	ip8_2 += (int)(file_num[1] - '0');
    }
    else 
	ip8_2 += (int)(file_num[0] - '0');

    servaddr.sin6_addr.s6_addr[14] = (uint8_t)ip8_1;//IPv6 Address 8 //TODO
    servaddr.sin6_addr.s6_addr[15] = (uint8_t)ip8_2;

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
