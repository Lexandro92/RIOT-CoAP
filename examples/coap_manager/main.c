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
#include "coap.h"
#include <time.h>


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


extern int coap_client_setup(int argc, char **argv);
int coap_cmd(int argc, char **argv);
void end (void) __attribute__((destructor));


static const shell_command_t shell_commands[] = {
    { "coap2", "Starts X amount of coap clients", coap_client_setup },
    { "coap", "Starts X amount of coap clients", coap_cmd },
    { NULL, NULL, NULL }
};

int i,tap_num;
int status;
char num[4];
char comm[50];

int main(void)
{
    puts("RIOT CoAP Client Manager");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}

int coap_cmd(int argc, char **argv){
    (void)argc;
    (void)argv;

    printf("Amount of CoAP clients you want: ");
    scanf("%d",&tap_num);
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);
//File for communication between the 2 source codes.
    int fp = open("./coap/tap_control.txt", O_WRONLY | O_TRUNC);
    if(0 > fp)
    {
        printf("\n open() Error!!!\n");
        return 1;
    }
//Initalize our file lock
    struct flock fl;
    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

//DEMO VARS
struct ifreq ifr;
const char *clonedev = "/dev/net/tun";
char name[7];
strcpy(name,"tap");
strcat(name,num);
int tap_fd;

//Start Coap Clients
    for (i=0;i<tap_num;i++){

    sprintf(num,"%d",i);
    //num[1]=0;
    fp = open("./coap/tap_control.txt", O_WRONLY | O_TRUNC);
    fcntl(fp, F_SETLKW, &fl);  //Locks the file for writing
    write(fp,num,4);
    fl.l_type = F_UNLCK;  /* tell it to unlock the region */
    fcntl(fp, F_SETLK, &fl); /* set the region to unlocked */
    if(0 > close(fp))
    {
        printf("\n close() Error!!!\n");
        return 1;
    }
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

//DEMO END

    strcpy(comm,"xterm -hold ./coap/bin/native/coap.elf &");
    status=system(comm);

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 250000000;
    nanosleep(&tim , &tim2);   
    }

    (void)status;
    return 0;
}

//Destructor
__attribute__((destructor)) void end (void)
{
    for (i=0;i<tap_num;i++){
    	sprintf(num,"%d",i);
	//Delete previous Taps
    	strcpy(comm,"ip link delete tap");
  	strcat(comm,num);
   	status=system(comm);
    }
}
