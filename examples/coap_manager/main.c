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
#include <inttypes.h>
#include <stdint.h>
#include "native_internal.h"
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>


int coap_cmd(int argc, char **argv);
void end (void) __attribute__((destructor));

static const shell_command_t shell_commands[] = {
    { "coap", "Starts X amount of coap clients", coap_cmd },
    { NULL, NULL, NULL }
};

int i,tap_num,status;
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
//READ THE SIZE OF TAP_AMOUNT FILE
    char  ffile_size[2];
    int file_size;
    int fp = open("./set_tap_up/tap_char_size.txt", O_RDONLY);
    if(0 > fp)
    {
        printf("\n open() Error!!!\n");
        return 1;
    }
    read(fp,ffile_size,2);
//CLOSE FILE
    if(0 > close(fp))
    {
        printf("\n close() Error!!!\n");
        return 1;
    }
    file_size = (int)(ffile_size[0] - '0');
//READ THE AMOUNT OF COAP CLIENTS TO BE CREATED
    char  file_num[file_size];
    fp = open("./set_tap_up/tap_amount.txt", O_RDONLY);
    if(0 > fp)
    {
        printf("\n open() Error!!!\n");
        return 1;
    }
    read(fp,file_num,file_size);
//CLOSE FILE
    if(0 > close(fp))
    {
        printf("\n close() Error!!!\n");
        return 1;
    }
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

    printf("Amount of CoAP clients will be created: %d\n",tap_num);
    strcpy(comm,"sysctl net.ipv6.conf.all.forwarding=1");
    status=system(comm);

//Initalize our file lock
    struct flock fl;
    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

    int num_size=0;
    char num_size2char[2];
//Start Coap Clients
    for (i=0;i<tap_num;i++){
	    fl.l_type = F_WRLCK;  // tell it to lock the region 
	    fp = open("./coap/tap_control.txt", O_WRONLY | O_TRUNC);
	    fcntl(fp, F_SETLKW, &fl);  //Locks the file for writing
	    if(i/1000 >= 1.0)
		num_size=4;
    	    else if(i/100 >= 1.0)
		num_size=3;
     	    else if(i/10 >= 1.0)
		num_size=2;
    	    else 
	        num_size=1;
    	    char num_w[num_size];
       	    sprintf(num_w,"%d",i);
    	    write(fp,num_w,num_size);
	    fl.l_type = F_UNLCK;  // tell it to unlock the region 
	    fcntl(fp, F_SETLK, &fl); // set the region to unlocked 
	    if(0 > close(fp))
	    {
		printf("\n close() Error!!!\n");
		return 1;
	    }

//SET FILE SIZE
	    fp = open("./coap/tap_control_size.txt", O_WRONLY | O_TRUNC);
	    fl.l_type = F_WRLCK;  // tell it to lock the region 
	    fcntl(fp, F_SETLKW, &fl);  //Locks the file for writing
	    strcpy(num_size2char,"");
    	    sprintf(num_size2char,"%d",num_size);
    	    write(fp,num_size2char,2);
	    fl.l_type = F_UNLCK;  // tell it to unlock the region 
	    fcntl(fp, F_SETLK, &fl); // set the region to unlocked 
	    if(0 > close(fp))
	    {
		printf("\n close() Error!!!\n");
		return 1;
	    }

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
    char num[4];
    for (i=0;i<tap_num;i++){
    	sprintf(num,"%d",i);
	//Delete previous Taps
    	strcpy(comm,"ip link delete tap");
  	strcat(comm,num);
   	status=system(comm);
    }
//Kill CoAP clients
  	strcpy(comm,"pkill -9 coap.elf");
   	status=system(comm);
}
