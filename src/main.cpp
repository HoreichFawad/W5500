
#include "mbed.h"
#include "w5500_header.hpp"
#include "EthernetInterface.h"


int main() {
    EthernetInterface eth(MBED_CONF_W5500_MOSI_PIN,MBED_CONF_W5500_MISO_PIN,MBED_CONF_W5500_CLK_PIN,MBED_CONF_W5500_CS_PIN,MBED_CONF_W5500_RST_PIN);

    // SPI spi(...);
    // W5500 chip(&spi)
    // chip.Supsend(
    
    eth.init(); //Use DHCP
    printf("init\r\n");
    eth.connect();
    printf("IP address: %s\r\n", eth.getIPAddress());
}          