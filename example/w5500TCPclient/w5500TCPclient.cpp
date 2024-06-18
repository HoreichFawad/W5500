#include "mbed.h"
#include "w5500_header.hpp"
#include "TCPSocketServer.h"
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"
#include "w5500example.hpp"

const char * IP_Addrc    = "192.168.13.165";
const char * IP_Subnetc  = "255.255.255.0";
const char * IP_Gatewayc = "192.168.11.1";
const char * DIP_Addr    = "192.168.13.165";
uint8_t MAC_Addrc[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};

uint32_t strToIP_(const char *str)
{
    uint32_t ip = 0;
    char *p = (char *)str;
    for (int i = 0; i < 4; i++)
    {
        ip |= atoi(p);
        p = strchr(p, '.');
        if (p == NULL)
        {
            break;
        }
        ip <<= 8;
        p++;
    }
    return ip;
}

int clientExampleTCP() 
{
    SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
    EthernetInterface net(&spi, PB_12, PA_10);
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    printf("Starting client...\n");
    // printf("Ptimer is %d\n",ptimer);
    // uint8_t buffer=0x1388;
    net.reset();
    // wait_us(1000*1000);
    // ptimer=net.getPTIMER();
    // printf("Ptimer is %d\n",ptimer);
    uint16_t buf=0;
    // buf=net.getSn_DPORT(0);
    // printf("port is %d\n",buf);
    // net.spiWrite(MR,0x04,&buffer,1);
    uint32_t ip=strToIP_(IP_Addrc);
    uint32_t subn=strToIP_(IP_Subnetc);
    uint32_t gateway=strToIP_(IP_Gatewayc);
    net.reg_wr<uint32_t>(GAR, gateway);
    net.reg_wr<uint32_t>(SUBR, subn);
    for (int i = 0; i < 6; i++)
        net.reg_wr<uint8_t>(SHAR + i, MAC_Addrc[i]);
    net.reg_wr<uint32_t>(SIPR, ip);
    // buffer=0x01;
    // net.spiWrite(IMR,0x04,&buffer,1);
   
    // net.sreg<uint8_t>(0, Sn_MR, Sn_MR_CLOSE);
    net.sreg<uint8_t>(0, Sn_MR, Sn_MR_TCP);
    uint8_t mrValue= net.getSn_MR(0);
    printf("the value of MR is %d\n",mrValue);
    net.sreg<uint16_t>(0, Sn_PORT, 80);
    uint32_t dip=strToIP_(DIP_Addr);
    net.sreg<uint16_t>(0,Sn_DPORT, 5000);
    // uint8_t bufPort[2]={0x13,0x88};
    // net.spiWrite(0x0010,0x0C,bufPort,2);
    net.sreg<uint32_t>(0,Sn_DIPR, dip);
    net.scmd(0, W5500::Command::OPEN);
    while(net.getSn_SR(0)!= W5500::SOCK_INIT);
    // buf=net.getSn_PORT(0);
    buf=net.getSn_DPORT(0);
    printf("port is %d\n",buf);
    net.scmd(0, W5500::Command::CONNECT);
    // printf("status register value= %d\n",net.getSn_SR(0));
    while(!net.getSn_IR(0));
    printf("Connection established\n");
    // // Send data to the server
    char *message = "Hello, Server!";
    
    int size,size2;
     do
        {
            size = net.sreg<uint16_t>(0, Sn_TX_FSR);
            size2 = net.sreg<uint16_t>(0, Sn_TX_FSR);
        } while (size != size2);
    uint16_t ptr = net.sreg<uint16_t>(0, Sn_TX_WR);
    uint8_t cntl_byte = (0x14 + (0 << 5));
    net.spiWrite(ptr, cntl_byte, (uint8_t *)message, strlen(message));
    net.sreg<uint16_t>(0, Sn_TX_WR, ptr + (strlen(message)));
    net.scmd(0, W5500::SEND);
    uint8_t tmp_Sn_IR;
    while (((tmp_Sn_IR = net.sreg<uint8_t>(0, Sn_IR)) & W5500::INT_SEND_OK) != W5500::INT_SEND_OK)
    {
        // @Jul.10, 2014 fix contant name, and udp sendto function.
        switch (net.sreg<uint8_t>(0, Sn_SR))
        {
        case W5500::SOCK_CLOSED:
            net.close(0);
            return 0;
            // break;
        case W5500::SOCK_UDP:
            // ARP timeout is possible.
            if ((tmp_Sn_IR & W5500::INT_TIMEOUT) == W5500::INT_TIMEOUT)
            {
                net.sreg<uint8_t>(0, Sn_IR, W5500::INT_TIMEOUT);
                return 0;
            }
            break;
        default:
            break;
        }
    }
    net.sreg<uint8_t>(0, Sn_IR, W5500::INT_SEND_OK);
    uint8_t buffer=0;
    do{
        buffer=net.sreg<uint8_t>(0, Sn_IR);
    }while(buffer!=5);
    size=0;
    size2=0;
     do
        {
            size = net.sreg<uint16_t>(0, Sn_RX_RSR);
            size2 = net.sreg<uint16_t>(0, Sn_RX_RSR);
        } while (size != size2);
            // return size;

        // if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms)
        // {
        //     break;
        // }
    ptr = net.sreg<uint16_t>(0, Sn_RX_RD);
    cntl_byte = (0x18 + (0 << 5));
    char buffer2[256];
    net.spiRead(ptr, cntl_byte, (uint8_t*)buffer2, size);
    net.sreg<uint16_t>(0, Sn_RX_RD, ptr + size);
    net.scmd(0, W5500::RECV);
    int len=size;
    if (len > 0) {
        buffer2[len] = '\0'; // Null-terminate the received string
        printf("Received: %s\n", buffer2);
    }
    
    // socket.send(message, strlen(message));

    return 0;

    // // Close the socket
    // socket.close();

    // // Bring down the network interface
    // net.disconnect();

    // printf("Client done\n");
}