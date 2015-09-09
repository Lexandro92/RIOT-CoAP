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

#include "shell.h"

extern int coap_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
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
    char num[10];
    char comm[50];
    int status;
    printf("Amount of CoAP clients you want: ");
    scanf("%d",&tap_num);
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);
//Delete previous Taps
    for (i=0;i<tap_num;i++){
    sprintf(num,"%d",i);
    strcpy(comm,"ip link delete tap");
    strcat(comm,num);
    status=system(comm);
    }
//TODO
//Create Tap devices

//Start Coap Clients
    (void)status;
    return 0;
}
