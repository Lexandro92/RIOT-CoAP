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

#include "shell.h"
#include "coap.h"
#include <time.h>

extern int coap_client_setup(int argc, char **argv);
int coap_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "coap2", "Starts X amount of coap clients", coap_client_setup },
    { "coap", "Starts X amount of coap clients", coap_cmd },
    { NULL, NULL, NULL }
};

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
    int i,tap_num;
    char comm[50];
    char num[10];
    int status;

    printf("Amount of CoAP clients you want: ");
    scanf("%d",&tap_num);
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);
//File for communication between the 2 source codes.
    //FILE *fp;
    //fp = fopen("./coap/tap_control.txt","w");
    int fp = open("./coap/tap_control.txt", O_WRONLY | O_TRUNC);
    if(0 > fp)
    {
        printf("\n fopen() Error!!!\n");
        return 1;
    }
//Initalize our file lock
    struct flock fl;
    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

//Start Coap Clients
    for (i=0;i<tap_num;i++){
    sprintf(num,"%d",i);
    fp = open("./coap/tap_control.txt", O_WRONLY | O_TRUNC);
    fcntl(fp, F_SETLKW, &fl);  //Locks the file for writing
    write(fp,num,1);
    fl.l_type = F_UNLCK;  /* tell it to unlock the region */
    fcntl(fp, F_SETLK, &fl); /* set the region to unlocked */
    close(fp);
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
