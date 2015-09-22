#include <sys/ioctl.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include "native_internal.h"
#include <strings.h>
#include <string.h>

#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>


void set_net_init(char *name){
printf("NAME: %s\n",name);
    struct ifreq ifr;
    const char *clonedev = "/dev/net/tun";
    int tap_fd;

    //Set TAP device up, give it local ipv6 address
    if ((tap_fd = open(clonedev , O_RDWR)) == -1) {
        err(EXIT_FAILURE, "open(%s)", clonedev);
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl(tap_fd, TUNSETIFF, (void *)&ifr) == -1) {
        _native_in_syscall++;
        warn("ioctl TUNSETIFF");
        warnx("probably the tap interface (%s) does not exist or is already in use", name);
        exit(EXIT_FAILURE);
    }

    if (real_close(tap_fd) == -1) {
        err(EXIT_FAILURE, "close(%s)", clonedev);
    }
}
