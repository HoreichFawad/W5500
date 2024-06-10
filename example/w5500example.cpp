#include "mbed.h"
#include "w5500_header.hpp"
#include "TCPSocketServer.h"
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"
#include "w5500example.hpp"

const char * IP_Addr    = "192.168.13.164";
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "192.168.11.1";
uint8_t MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};
SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
EthernetInterface net(&spi, PB_12, PA_10);
// W5500 w5500(&spi,PB_12,PA_10);

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

int serverExampleTCP() {
    
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    uint8_t buffer=0x80;
    net.spiWrite(MR,0x04,&buffer,1);
    uint32_t ip=strToIP_(IP_Addr);
    uint32_t subn=strToIP_(IP_Subnet);
    uint32_t gateway=strToIP_(IP_Gateway);
    net.reg_wr<uint32_t>(GAR, gateway);
    net.reg_wr<uint32_t>(SUBR, subn);
    for (int i = 0; i < 6; i++)
        net.reg_wr<uint8_t>(SHAR + i, MAC_Addr[i]);
    net.reg_wr<uint32_t>(SIPR, ip);
    buffer=0x01;
    net.spiWrite(IMR,0x04,&buffer,1);
    net.sreg<uint16_t>(0, Sn_PORT, 80);
    net.sreg<uint8_t>(0, Sn_MR, Sn_MR_TCP);
    net.scmd(0, W5500::Command::OPEN);
    uint16_t buf=0;
    buf=net.getSn_PORT(0);
    printf("port is %d\n",buf);
    uint8_t buf1=net.getSn_SR(0);
    if (buf1 != W5500::Status::SOCK_INIT)
        printf("socket not opened\n");
    else
        printf("Socket Opened\n");
    net.scmd(0, W5500::Command::LISTEN);
    buf1=net.getSn_SR(0);
    if(buf1==W5500::Status::SOCK_LISTEN)
        printf("Listen initiated\n");
    else
        printf("listen not initiated\n");
    printf("status register value= %d\n",net.sreg<uint8_t>(0, Sn_SR));
    printf("Connected. IP address: %s\n", net.getIPAddress());

    Timer t;
    t.reset();
    t.start();
    while(1) {
        // printf("status register value= %d\n",net.sreg<uint8_t>(0, Sn_SR));
        if (t.read_ms() > 1000000) {
            printf("exiting\n");
            return -1;
        }
        if (net.sreg<uint8_t>(0, Sn_SR) == W5500::SOCK_ESTABLISHED) {
            printf("Connection established\n");
            break;
        }
    }
    buffer=0;
    net.spiWrite(SIR,0x04,&buffer,1);
    wait_us(3000*1000);
    t.reset();
    t.start();
    int size, size2;
    while (1)
    {
        // int size = sreg<uint16_t>(socket, Sn_RX_RSR);
        
        // during the reading Sn_RX_RSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        do
        {
            size = net.sreg<uint16_t>(0, Sn_RX_RSR);
            size2 = net.sreg<uint16_t>(0, Sn_RX_RSR);
        } while (size != size2);
            printf("size is %d\n",size);
            break;
            // return size;

        // if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms)
        // {
        //     break;
        // }
    }
    uint16_t ptr = net.sreg<uint16_t>(0, Sn_RX_RD);
    uint8_t cntl_byte = (0x18 + (0 << 5));
    char buffer2[256];
    net.spiRead(ptr, cntl_byte, (uint8_t*)buffer2, size);
    net.sreg<uint16_t>(0, Sn_RX_RD, ptr + size);
    net.scmd(0, W5500::RECV);
    int len=size;
    if (len > 0) {
        buffer2[len] = '\0'; // Null-terminate the received string
        printf("Received: %s\n", buffer2);
    }
        // Echo the data back to the client
        // newConnection.send(buffer2, len);
        // int size = sreg<uint16_t>(socket, Sn_TX_FSR);
        // int size, size2;
        // during the reading Sn_TX_FSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        do
        {
            size = net.sreg<uint16_t>(0, Sn_TX_FSR);
            size2 = net.sreg<uint16_t>(0, Sn_TX_FSR);
        } while (size != size2);
        if (size > len) {
        size = len;
        }
    ptr = net.sreg<uint16_t>(0, Sn_TX_WR);
    cntl_byte = (0x14 + (0 << 5));
    net.spiWrite(ptr, cntl_byte, (uint8_t *)buffer2, len);
    net.sreg<uint16_t>(0, Sn_TX_WR, ptr + len);
    net.scmd(0, W5500::SEND);
    uint8_t tmp_Sn_IR;
    while (((tmp_Sn_IR = net.sreg<uint8_t>(0, Sn_IR)) & net.INT_SEND_OK) != W5500::INT_SEND_OK)
    {
        // @Jul.10, 2014 fix contant name, and udp sendto function.
        switch (net.sreg<uint8_t>(0, Sn_SR))
        {
        case W5500::SOCK_CLOSED:
            close(0);
            return 0;
            // break;
        case W5500::SOCK_UDP:
            // ARP timeout is possible.
            if ((tmp_Sn_IR & net.INT_TIMEOUT) == net.INT_TIMEOUT)
            {
                net.sreg<uint8_t>(0, Sn_IR, net.INT_TIMEOUT);
                return 0;
            }
            break;
        default:
            break;
        }
    }
    net.sreg<uint8_t>(0, Sn_IR, W5500::INT_SEND_OK);

    // return len;
    
    return 0;
    // }

    // // Close the socket
    // newConnection.close();
    // server.close();

    // // Bring down the network interface
    // net.disconnect();
    // return 0;
    // printf("Server done\n");
}

// int clientExampleTCP() {
//     SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
//     EthernetInterface net(&spi, PB_12, PA_10);
//     spi.format(8,0); // 8bit, mode 0
//     spi.frequency(1000000); // 1MHz
//     wait_us(1000*1000); // 1 second for stable state
//     printf("Starting client...\n");
//     // Bring up the network interface
//     int result = net.connect();
//     if (result != 0) {
//         printf("Error connecting: %d\n", result);
//         return -1;
//     }
//     printf("Connected. IP address: %s\n", net.getIPAddress());

//     // Create a TCP socket
//     TCPSocketConnection socket;
//     // socket.open(&net);

//     // Connect to the server
//     result = socket.connect("192.168.11.194",5000);
//     if (result != 0) {
//         printf("Error connecting to server: %d\n", result);
//         return -1;
//     }

//     // Send data to the server
//     char *message = "Hello, Server!";
//     socket.send(message, strlen(message));

//     // Receive data from the server
//     char buffer[256];
//     int len = socket.receive(buffer, sizeof(buffer));
//     if (len > 0) {
//         buffer[len] = '\0'; // Null-terminate the received string
//         printf("Received from server: %s\n", buffer);
//     }

//     // Close the socket
//     socket.close();

//     // Bring down the network interface
//     net.disconnect();

//     printf("Client done\n");
// }
