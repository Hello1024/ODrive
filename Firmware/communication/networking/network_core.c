// based on LRNDIS lib demonstration project

#include "network_core.h"
#include "lrndis/dhcp-server/dhserver.h"
#include "lrndis/dns-server/dnserver.h"
#include <stdlib.h>
#include <stdio.h>
#include "lrndis/rndis-stm32/usbd_rndis_core.h"
#include "netif/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "lwip/tcp_impl.h"
#include "lwip/tcp.h"
#include "time.h"
#include "httpd.h"

#include <cmsis_os.h>

static uint8_t hwaddr[6]  = {0x20,0x89,0x84,0x6A,0x96,00};
static uint8_t ipaddr[4]  = {192, 168, 7, 1};
static uint8_t netmask[4] = {255, 255, 255, 0};
static uint8_t gateway[4] = {0, 0, 0, 0};

#define NUM_DHCP_ENTRY 3

static dhcp_entry_t entries[NUM_DHCP_ENTRY] =
{
    /* mac    ip address        subnet mask        lease time */
    { {0}, {192, 168, 7, 2}, {255, 255, 255, 0}, 24 * 60 * 60 },
    { {0}, {192, 168, 7, 3}, {255, 255, 255, 0}, 24 * 60 * 60 },
    { {0}, {192, 168, 7, 4}, {255, 255, 255, 0}, 24 * 60 * 60 }
};

static dhcp_config_t dhcp_config =
{
    {192, 168, 7, 1}, 67, /* server address, port */
    {192, 168, 7, 1},     /* dns server */
    "odrive",                /* dns suffix */
    NUM_DHCP_ENTRY,       /* num entry */
    entries               /* entries */
};


void timer_handler();


osThreadId network_thread;

struct netif netif_data;

const char *network_rx_data = NULL;
int network_rx_size;

osMessageQDef(net_message_q, 5, struct pbuf* ); // Declare a message queue
osMessageQId (net_message_q_id);           // Declare an ID for the message queue

osSemaphoreId sem_network_tx;
osSemaphoreDef(sem_network_tx);


osTimerDef(tmr_network, timer_handler);


// Received in USB interrupt
void rndis_handler(const char *data, int size)
{
    if (data) {
        // We don't want to run
        // the whole IP stack in the interrupt, so make a copy
        // and use a semaphore.
        
        struct pbuf* frame = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
        if (!frame)  // Out of memory drop
            return; 

        memcpy(frame->payload, data, size);
        frame->len = size;

        if (osMessagePut(net_message_q_id, (uint32_t)frame, 0) != osOK) {
            pbuf_free(frame);
        }
    } else {
        // NULL data means rndis is good to TX another packet
        osSemaphoreRelease(sem_network_tx);
    }
}

// Run in timer thread.  Wake up the main thread.
void timer_handler()
{
    // Ignore error if queue is full.
    osMessagePut(net_message_q_id, 0, 0);
}

// Used inside lwip
u32_t sys_now(void) {
    return (u32_t)((uint64_t)osKernelSysTick() * 1000 / osKernelSysTickFrequency);
}



err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    static char data[RNDIS_MTU + 14 + 4];
    int size = 0;

    // Timeout eventually in case link goes down.
    osSemaphoreWait(sem_network_tx, 100);
    if (!rndis_can_send())
        return ERR_USE;

    for(q = p; q != NULL; q = q->next)
    {
        if (size + q->len > RNDIS_MTU + 14)
            return ERR_ARG;
        memcpy(data + size, (char *)q->payload, q->len);
        size += q->len;
    }

    rndis_send(data, size);
    return ERR_OK;
}

err_t netif_init_cb(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = RNDIS_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = etharp_output;
    return ERR_OK;
}

#define PADDR(ptr) ((ip_addr_t *)ptr)

static void init_lwip()
{
    net_message_q_id = osMessageCreate(osMessageQ(net_message_q), NULL);
    sem_network_tx = osSemaphoreCreate(osSemaphore(sem_network_tx), 1);
    rndis_net_handler = rndis_handler;

    osTimerId timer = osTimerCreate(osTimer(tmr_network), osTimerPeriodic, (void *)0);
    osTimerStart(timer, TCP_TMR_INTERVAL);
    struct netif  *netif = &netif_data;

    lwip_init();
    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, hwaddr, 6);

    netif = netif_add(netif, PADDR(ipaddr), PADDR(netmask), PADDR(gateway), NULL, netif_init_cb, ip_input);
    netif_set_default(netif);
}

static bool dns_query_proc(const char *name, ip_addr_t *addr)
{
    if (strcmp(name, "control.odrive") == 0 || strcmp(name, "www.control.odrive") == 0)
    {
        addr->addr = *(uint32_t *)ipaddr;
        return true;
    }
    return false;
}

static void network_thread_main()
{
    
    init_lwip();

    while (!netif_is_up(&netif_data)) ;

    while (dhserv_init(&dhcp_config) != ERR_OK) ;

    while (dnserv_init(PADDR(ipaddr), 53, dns_query_proc) != ERR_OK) ;

    httpd_init();

    while(true) {
        osEvent event = osMessageGet(net_message_q_id, osWaitForever);
        if (event.status == osEventMessage) {

            if (event.value.p) {
                ethernet_input(event.value.p, &netif_data);
                pbuf_free(event.value.p);
            } else {
                tcp_tmr();  
            }
        }
    }
}


void start_networking(void) {
    // Start USB communication thread
    osThreadDef(network_thread_def, network_thread_main, osPriorityNormal, 0, 512);
    network_thread = osThreadCreate(osThread(network_thread_def), NULL);
}