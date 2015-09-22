/*
 * Copyright (C) 2008, 2009, 2010  Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2013 INRIA
 * Copyright (C) 2013 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
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
 * @brief       Default application that shows a lot of functionality of RIOT
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "shell.h"

#include <sys/ioctl.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include "native_internal.h"
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

int coap_cmd(int argc, char **argv);
extern void set_net_init(char *name);

int i,tap_num,num_size;
int status;
char comm[50];

int main(void)
{
    char name[7];
    puts("RIOT Tap Setup");
    /* start shell */
    puts("All up, running the shell now");
    printf("Amount of CoAP clients you want: ");
    scanf("%d",&tap_num);
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);
//File for communication between the 2 source codes.
    int fp = open("./tap_amount.txt", O_WRONLY | O_TRUNC);
    if(0 > fp)
    {
        printf("\n open() Error!!!\n");
        return 1;
    }
    if(tap_num/1000 >= 1.0)
	num_size=4;
    else if(tap_num/100 >= 1.0)
	num_size=3;
    else if(tap_num/10 >= 1.0)
	num_size=2;
    else 
	num_size=1;
    char num[num_size];
    sprintf(num,"%d",tap_num);
    write(fp,num,num_size);
//CLOSE FILE
    if(0 > close(fp))
    {
	printf("\n close() Error!!!\n");
	return 1;
    }

//WRITE THE STRING SIZE INTO FILE
    fp = open("./tap_char_size.txt", O_WRONLY | O_TRUNC);
    if(0 > fp)
    {
        printf("\n open() Error!!!\n");
        return 1;
    }
    char *num_size2char;
    sprintf(num_size2char,"%d",num_size);
    write(fp,&num_size2char,2);
//CLOSE FILE
    if(0 > close(fp))
    {
	printf("\n close() Error!!!\n");
	return 1;
    }

//Start Coap Clients
    for (i=0;i<tap_num;i++){
	    sprintf(num,"%d",i);

	//DEMO
	    strcpy(name,"tap");
	    strcat(name,num);
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
	    strcat(comm,num);
	    strcat(comm,"/64 dev tap");
	    strcat(comm,num);
	    status=system(comm);
	    (void)status;
	//Set TAP device up, give it local ipv6 address
	    set_net_init(name);
    }
    (void)status;
    /* should be never reached */
    exit(0);
    return 0;
}

