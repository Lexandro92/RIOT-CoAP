/*
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gnrc_slip
 * @{
 *
 * @file
 * @brief       SLIP device implementation
 *
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kernel.h"
#include "kernel_types.h"
#include "msg.h"
#include "net/gnrc.h"
#include "periph/uart.h"
#include "od.h"
#include "ringbuffer.h"
#include "thread.h"
#include "net/ipv6/hdr.h"

#include "net/gnrc/slip.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#define _SLIP_END               ('\xc0')
#define _SLIP_ESC               ('\xdb')
#define _SLIP_END_ESC           ('\xdc')
#define _SLIP_ESC_ESC           ('\xdd')

#define _SLIP_MSG_TYPE          (0xc1dc)    /* chosen randomly */
#define _SLIP_NAME              "SLIP"
#define _SLIP_MSG_QUEUE_SIZE    (8U)

#define _SLIP_DEV(arg)    ((gnrc_slip_dev_t *)arg)

/* UART callbacks */
static void _slip_rx_cb(void *arg, char data)
{
    if (data == _SLIP_END) {
        msg_t msg;

        msg.type = _SLIP_MSG_TYPE;
        msg.content.value = _SLIP_DEV(arg)->in_bytes;

        msg_send_int(&msg, _SLIP_DEV(arg)->slip_pid);

        _SLIP_DEV(arg)->in_bytes = 0;
    }
    else if (_SLIP_DEV(arg)->in_esc) {
        _SLIP_DEV(arg)->in_esc = 0;

        switch (data) {
            case (_SLIP_END_ESC):
                if (ringbuffer_add_one(&_SLIP_DEV(arg)->in_buf, _SLIP_END) < 0) {
                    _SLIP_DEV(arg)->in_bytes++;
                }

                break;

            case (_SLIP_ESC_ESC):
                if (ringbuffer_add_one(&_SLIP_DEV(arg)->in_buf, _SLIP_ESC) < 0) {
                    _SLIP_DEV(arg)->in_bytes++;
                }

                break;

            default:
                break;
        }
    }
    else if (data == _SLIP_ESC) {
        _SLIP_DEV(arg)->in_esc = 1;
    }
    else {
        if (ringbuffer_add_one(&_SLIP_DEV(arg)->in_buf, data) < 0) {
            _SLIP_DEV(arg)->in_bytes++;
        }
    }
}

int _slip_tx_cb(void *arg)
{
    if (_SLIP_DEV(arg)->out_buf.avail > 0) {
        char c = (char)ringbuffer_get_one(&_SLIP_DEV(arg)->out_buf);
        uart_write((uart_t)(_SLIP_DEV(arg)->uart), c);
        return 1;
    }

    return 0;
}

/* SLIP receive handler */
static void _slip_receive(gnrc_slip_dev_t *dev, size_t bytes)
{
    gnrc_netif_hdr_t *hdr;
    gnrc_pktsnip_t *pkt, *netif_hdr;

    pkt = gnrc_pktbuf_add(NULL, NULL, bytes, GNRC_NETTYPE_UNDEF);

    if (pkt == NULL) {
        DEBUG("slip: no space left in packet buffer\n");
        return;
    }

    netif_hdr = gnrc_pktbuf_add(pkt, NULL, sizeof(gnrc_netif_hdr_t),
                                GNRC_NETTYPE_NETIF);

    if (netif_hdr == NULL) {
        DEBUG("slip: no space left in packet buffer\n");
        gnrc_pktbuf_release(pkt);
        return;
    }

    hdr = netif_hdr->data;
    gnrc_netif_hdr_init(hdr, 0, 0);
    hdr->if_pid = thread_getpid();

    if (ringbuffer_get(&dev->in_buf, pkt->data, bytes) != bytes) {
        DEBUG("slip: could not read %u bytes from ringbuffer\n", (unsigned)bytes);
        gnrc_pktbuf_release(pkt);
        return;
    }
#if ENABLE_DEBUG && defined(MODULE_OD)
    else {
        DEBUG("slip: received data\n");
        od_hex_dump(pkt->data, bytes, OD_WIDTH_DEFAULT);
    }
#endif

#ifdef MODULE_GNRC_IPV6
    if ((pkt->size >= sizeof(ipv6_hdr_t)) && ipv6_hdr_is(pkt->data)) {
        pkt->type = GNRC_NETTYPE_IPV6;
    }
#endif

    if (gnrc_netapi_dispatch_receive(pkt->type, GNRC_NETREG_DEMUX_CTX_ALL, pkt) == 0) {
        DEBUG("slip: unable to forward packet of type %i\n", pkt->type);
        gnrc_pktbuf_release(pkt);
    }
}

static inline void _slip_send_char(gnrc_slip_dev_t *dev, char c)
{
    ringbuffer_add_one(&dev->out_buf, c);
    uart_tx_begin(dev->uart);
}

/* SLIP send handler */
static void _slip_send(gnrc_slip_dev_t *dev, gnrc_pktsnip_t *pkt)
{
    gnrc_pktsnip_t *ptr;

    ptr = pkt->next;    /* ignore gnrc_netif_hdr_t, we don't need it */

    while (ptr != NULL) {
        DEBUG("slip: send pktsnip of length %u over UART_%d\n", (unsigned)ptr->size, dev->uart);
        char *data = ptr->data;

        for (size_t i = 0; i < ptr->size; i++) {
            switch (data[i]) {
                case _SLIP_END:
                    DEBUG("slip: encountered END byte on send: stuff with ESC\n");
                    _slip_send_char(dev, _SLIP_ESC);
                    _slip_send_char(dev, _SLIP_END_ESC);
                    break;

                case _SLIP_ESC:
                    DEBUG("slip: encountered ESC byte on send: stuff with ESC\n");
                    _slip_send_char(dev, _SLIP_ESC);
                    _slip_send_char(dev, _SLIP_ESC_ESC);
                    break;

                default:
                    _slip_send_char(dev, data[i]);

                    break;
            }
        }

        ptr = ptr->next;
    }

    _slip_send_char(dev, _SLIP_END);

    gnrc_pktbuf_release(pkt);
}

static void *_slip(void *args)
{
    gnrc_slip_dev_t *dev = _SLIP_DEV(args);
    msg_t msg, reply, msg_q[_SLIP_MSG_QUEUE_SIZE];

    msg_init_queue(msg_q, _SLIP_MSG_QUEUE_SIZE);
    dev->slip_pid = thread_getpid();
    gnrc_netif_add(dev->slip_pid);

    DEBUG("slip: SLIP runs on UART_%d\n", dev->uart);

    while (1) {
        DEBUG("slip: waiting for incoming messages\n");
        msg_receive(&msg);

        switch (msg.type) {
            case _SLIP_MSG_TYPE:
                DEBUG("slip: incoming message of size %" PRIu32 " from UART_%d in buffer\n",
                      msg.content.value, dev->uart);
                _slip_receive(dev, (size_t)msg.content.value);
                break;

            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUG("slip: GNRC_NETAPI_MSG_TYPE_SND received\n");
                _slip_send(dev, (gnrc_pktsnip_t *)msg.content.ptr);
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                DEBUG("slip: GNRC_NETAPI_MSG_TYPE_GET or GNRC_NETAPI_MSG_TYPE_SET received\n");
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t)(-ENOTSUP);
                DEBUG("slip: I don't support these but have to reply.\n");
                msg_reply(&msg, &reply);
                break;
        }
    }

    /* should be never reached */
    return NULL;
}

kernel_pid_t gnrc_slip_init(gnrc_slip_dev_t *dev, uart_t uart, uint32_t baudrate,
                            char *stack, size_t stack_size, char priority)
{
    kernel_pid_t pid;

    /* reset device descriptor fields */
    dev->uart = uart;
    dev->in_bytes = 0;
    dev->in_esc = 0;
    dev->slip_pid = KERNEL_PID_UNDEF;

    /* initialize buffers */
    ringbuffer_init(&dev->in_buf, dev->rx_mem, sizeof(dev->rx_mem));
    ringbuffer_init(&dev->out_buf, dev->tx_mem, sizeof(dev->tx_mem));

    /* initialize UART */
    DEBUG("slip: initialize UART_%d with baudrate %" PRIu32 "\n", uart,
          baudrate);
    if (uart_init(uart, baudrate, _slip_rx_cb, _slip_tx_cb, dev) < 0) {
        DEBUG("slip: error initializing UART_%i with baudrate %" PRIu32 "\n",
              uart, baudrate);
        return -ENODEV;
    }

    /* start SLIP thread */
    DEBUG("slip: starting SLIP thread\n");
    pid = thread_create(stack, stack_size, priority, CREATE_STACKTEST,
                        _slip, dev, _SLIP_NAME);
    if (pid < 0) {
        DEBUG("slip: unable to create SLIP thread\n");
        return -EFAULT;
    }
    return pid;
}
