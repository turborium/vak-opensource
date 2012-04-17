#include "w5100.h"
#include "ethernet.h"

uint8_t _ethernet_state [MAX_SOCK_NUM] = { 0 };

uint16_t _ethernet_server_port [MAX_SOCK_NUM] = { 0 };

void ethernet_begin (uint8_t *mac, uint8_t *ip, uint8_t *gateway, uint8_t *subnet)
{
    if (! gateway) {
        uint8_t default_gateway[4];

        default_gateway[0] = ip[0];
        default_gateway[1] = ip[1];
        default_gateway[2] = ip[2];
        default_gateway[3] = 1;
        gateway = default_gateway;
    }
    if (! subnet) {
        static uint8_t default_subnet[] = { 255, 255, 255, 0 };

        subnet = default_subnet;
    }
    w5100_init();
    w5100_setMACAddress (mac);
    w5100_setIPAddress (ip);
    w5100_setGatewayIp (gateway);
    w5100_setSubnetMask (subnet);
}
