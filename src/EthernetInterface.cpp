
#include "EthernetInterface.h"
#include "DHCPClient.h"
//static char ip_string[17];
//static uint32_t ipaddr ;
EthernetInterface::EthernetInterface(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset) :
        W5500(mosi, miso, sclk, cs, reset)
{
    ip_set = false;
}

EthernetInterface::EthernetInterface(SPI* spi, PinName cs, PinName reset) :
        W5500(spi, cs, reset)
{
    ip_set = false;
}


int EthernetInterface::init()
{
    dhcp = true;
    //
    //for (int i =0; i < 6; i++) this->mac[i] = mac[i];
    //
    reset();
    return 0;
}

int EthernetInterface::init(uint8_t * mac)
{
    dhcp = true;
    //
    for (int i =0; i < 6; i++) this->mac[i] = mac[i];
    //
    reset();
    setMac();    
    return 0;
}

// add this function, because sometimes no needed MAC address in init calling.
int EthernetInterface::init(const char* ip, const char* mask, const char* gateway)
{
    dhcp = false;
    //
    //for (int i =0; i < 6; i++) this->mac[i] = mac[i];
    //
    this->ip = strToIp(ip);
    strcpy(ip_string, ip);
    ip_set = true;
    this->netmask = strToIp(mask);
    this->gateway = strToIp(gateway);
    reset();
    
    // @Jul. 8. 2014 add code. should be called to write chip.
    setIP(); 
    
    return 0;
}

int EthernetInterface::init(uint8_t * mac, const char* ip, const char* mask, const char* gateway)
{
    dhcp = false;
    //
    for (int i =0; i < 6; i++) this->mac[i] = mac[i];
    //
    this->ip = strToIp(ip);
    strcpy(ip_string, ip);
    ip_set = true;
    this->netmask = strToIp(mask);
    this->gateway = strToIp(gateway);
    reset();

    // @Jul. 8. 2014 add code. should be called to write chip.
    setMac();
    setIP();
    
    return 0;
}

// Connect Bring the interface up, start DHCP if needed.
int EthernetInterface::connect()
{
    if (dhcp) {
        int r = IPrenew();
        if (r < 0) {
            return r;
        }
    }
    
    if (W5500::setIP() == false) return -1;
    return 0;
}

// Disconnect Bring the interface down.
int EthernetInterface::disconnect()
{
    if (W5500::disconnect() == false) return -1;
    return 0;
}

/*
void EthernetInterface::getip(void)
{
    //ip = 0x12345678;
    ipaddr = reg_rd<uint32_t>(SIPR);
    //snprintf(ip_string, sizeof(ip_string), "%d.%d.%d.%d", (ipaddr>>24)&0xff, (ipaddr>>16)&0xff, (ipaddr>>8)&0xff, ipaddr&0xff);   
}
*/
char* EthernetInterface::getIPAddress()
{
    //Suint32_t ip = inet_getip(SIPR);//reg_rd<uint32_t>(SIPR);
    //static uint32_t ip = getip();
    //uint32_t ip = getip();
    
    uint32_t ip = reg_rd<uint32_t>(SIPR);
    snprintf(ip_string, sizeof(ip_string), "%d.%d.%d.%d", (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);   
    return ip_string;
}

char* EthernetInterface::getNetworkMask()
{
    uint32_t ip = reg_rd<uint32_t>(SUBR);
    snprintf(mask_string, sizeof(mask_string), "%d.%d.%d.%d", (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
    return mask_string;
}

char* EthernetInterface::getGateway()
{
    uint32_t ip = reg_rd<uint32_t>(GAR);
    snprintf(gw_string, sizeof(gw_string), "%d.%d.%d.%d", (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
    return gw_string;
}

char* EthernetInterface::getMACAddress()
{
    uint8_t mac[6];
    reg_rd_mac(SHAR, mac);
    snprintf(mac_string, sizeof(mac_string), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return mac_string; 
}

int EthernetInterface::IPrenew(int timeout_ms)
{
//    printf("DHCP Started, waiting for IP...\n");  
    DHCPClient dhcp;
    int err = dhcp.setup(timeout_ms);
    if (err == (-1)) {
//        printf("Timeout.\n");
        return -1;
    }
//    printf("Connected, IP: %d.%d.%d.%d\n", dhcp.yiaddr[0], dhcp.yiaddr[1], dhcp.yiaddr[2], dhcp.yiaddr[3]);
    ip      = (dhcp.yiaddr[0] <<24) | (dhcp.yiaddr[1] <<16) | (dhcp.yiaddr[2] <<8) | dhcp.yiaddr[3];
    gateway = (dhcp.gateway[0]<<24) | (dhcp.gateway[1]<<16) | (dhcp.gateway[2]<<8) | dhcp.gateway[3];
    netmask = (dhcp.netmask[0]<<24) | (dhcp.netmask[1]<<16) | (dhcp.netmask[2]<<8) | dhcp.netmask[3];
    dnsaddr = (dhcp.dnsaddr[0]<<24) | (dhcp.dnsaddr[1]<<16) | (dhcp.dnsaddr[2]<<8) | dhcp.dnsaddr[3];
    return 0;
}