/* example.cpp
 *
 * Copyright (C) 20017-2021 Fanzhe Lyu <lvfanzhe@hotmail.com>, all rights reserved.
 *
 * modbuspp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// #include "modbus.h"
#include "w5500example.hpp"
#include "w5500_header.hpp"
#include "testw5500.hpp"
#include "TCPSocketServer.h"
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"

using SOCKADDR = struct sockaddr;
using SOCKADDR_IN = struct sockaddr_in;

#define MAX_MSG_LENGTH 260

///Function Code
#define READ_COILS 0x01
#define READ_INPUT_BITS 0x02
#define READ_REGS 0x03
#define READ_INPUT_REGS 0x04
#define WRITE_COIL 0x05
#define WRITE_REG 0x06
#define WRITE_COILS 0x0F
#define WRITE_REGS 0x10

///Exception Codes

#define EX_ILLEGAL_FUNCTION 0x01 // Function Code not Supported
#define EX_ILLEGAL_ADDRESS 0x02  // Output Address not exists
#define EX_ILLEGAL_VALUE 0x03    // Output Value not in Range
#define EX_SERVER_FAILURE 0x04   // Slave Deive Fails to process request
#define EX_ACKNOWLEDGE 0x05      // Service Need Long Time to Execute
#define EX_SERVER_BUSY 0x06      // Server Was Unable to Accept MB Request PDU
#define EX_NEGATIVE_ACK 0x07
#define EX_MEM_PARITY_PROB 0x08
#define EX_GATEWAY_PROBLEMP 0x0A // Gateway Path not Available
#define EX_GATEWAY_PROBLEMF 0x0B // Target Device Failed to Response
#define EX_BAD_DATA 0XFF         // Bad Data lenght or Address

bool _connected{};
uint16_t PORT{};
uint32_t _msg_id{};
int _slaveid{};
std::string HOST;

// X_SOCKET _socket{};
// SOCKADDR_IN _server{};
uint8_t MAC_Addrc[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};
SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
EthernetInterface net(&spi, PB_12, PA_10);

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

bool init()
{
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    return true;
}

bool setConfigurations(const char* IP_Addrc, const char* IP_Subnetc, const char* IP_Gatewayc, const char* DIP_Addr)
{
    uint32_t ip=strToIP_(IP_Addrc);
    uint32_t subn=strToIP_(IP_Subnetc);
    uint32_t gateway=strToIP_(IP_Gatewayc);
    uint32_t dip=strToIP_(DIP_Addr);
    net.reg_wr<uint32_t>(GAR, gateway);
    net.reg_wr<uint32_t>(SUBR, subn);
    for (int i = 0; i < 6; i++)
        net.reg_wr<uint8_t>(SHAR + i, MAC_Addrc[i]);
    net.reg_wr<uint32_t>(SIPR, ip);
    net.sreg<uint8_t>(0, Sn_MR, Sn_MR_TCP);
    net.sreg<uint16_t>(0, Sn_PORT, 502);
    net.sreg<uint16_t>(0,Sn_DPORT, 502);
    net.sreg<uint32_t>(0,Sn_DIPR, dip);
    return true;
}

bool socketConfiguration()
{
    net.scmd(0, W5500::Command::OPEN);
    while(net.getSn_SR(0)!= W5500::SOCK_INIT);
    // buf=net.getSn_PORT(0);
    net.scmd(0, W5500::Command::CONNECT);
    // printf("status register value= %d\n",net.getSn_SR(0));
    while(!net.getSn_IR(0));
    printf("Connection established\n");
}

void modbus_set_slave_id(int id)
{
     _slaveid=id;
}

void modbus_build_request(uint8_t *to_send, uint16_t address, int func) 
{
    to_send[0] = (uint8_t)(_msg_id >> 8u);
    to_send[1] = (uint8_t)(_msg_id & 0x00FFu);
    to_send[2] = 0;
    to_send[3] = 0;
    to_send[4] = 0;
    to_send[6] = (uint8_t)_slaveid;
    to_send[7] = (uint8_t)func;
    to_send[8] = (uint8_t)(address >> 8u);
    to_send[9] = (uint8_t)(address & 0x00FFu);
}

ssize_t modbus_send(uint8_t *to_send, size_t length)
{
    _msg_id++;
    uint16_t ptr = net.sreg<uint16_t>(0, Sn_TX_WR);
    uint8_t cntl_byte = (0x14 + (0 << 5));
    net.spiWrite(ptr, cntl_byte, to_send, length);
    net.sreg<uint16_t>(0, Sn_TX_WR, ptr + length);
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
}

int modbus_write(uint16_t address, uint16_t amount, int func, const uint16_t *value)
{
    int status = 0;
    uint8_t *to_send;
    if (func == WRITE_COIL || func == WRITE_REG)
    {
        to_send = new uint8_t[12];
        modbus_build_request(to_send, address, func);
        to_send[5] = 6;
        to_send[10] = (uint8_t)(value[0] >> 8u);
        to_send[11] = (uint8_t)(value[0] & 0x00FFu);
        status = modbus_send(to_send, 12);
    }
    else if (func == WRITE_REGS)
    {
        to_send = new uint8_t[13 + 2 * amount];
        modbus_build_request(to_send, address, func);
        to_send[5] = (uint8_t)(7 + 2 * amount);
        to_send[10] = (uint8_t)(amount >> 8u);
        to_send[11] = (uint8_t)(amount & 0x00FFu);
        to_send[12] = (uint8_t)(2 * amount);
        for (int i = 0; i < amount; i++)
        {
            to_send[13 + 2 * i] = (uint8_t)(value[i] >> 8u);
            to_send[14 + 2 * i] = (uint8_t)(value[i] & 0x00FFu);
        }
        status = modbus_send(to_send, 13 + 2 * amount);
    }
    else if (func == WRITE_COILS)
    {
        to_send = new uint8_t[14 + (amount - 1) / 8];
        modbus_build_request(to_send, address, func);
        to_send[5] = (uint8_t)(7 + (amount + 7) / 8);
        to_send[10] = (uint8_t)(amount >> 8u);
        to_send[11] = (uint8_t)(amount & 0x00FFu);
        to_send[12] = (uint8_t)((amount + 7) / 8);
        for (int i = 0; i < (amount + 7) / 8; i++)
            to_send[13 + i] = 0; // init needed before summing!
        for (int i = 0; i < amount; i++)
        {
            to_send[13 + i / 8] += (uint8_t)(value[i] << (i % 8u));
        }
        status = modbus_send(to_send, 14 + (amount - 1) / 8);
    }
    delete[] to_send;
    return status;
}

ssize_t modbus_receive(uint8_t* buf)
{
    uint8_t buffer=0;
    do{
        buffer=net.sreg<uint8_t>(0, Sn_IR);
    }while(buffer!=5);
    int size=0;
    int size2=0;
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
    uint16_t ptr = net.sreg<uint16_t>(0, Sn_RX_RD);
    uint8_t cntl_byte = (0x18 + (0 << 5));
    net.spiRead(ptr, cntl_byte, buf, size);
    net.sreg<uint16_t>(0, Sn_RX_RD, ptr + size);
    net.scmd(0, W5500::RECV);
}

void modbuserror_handle(const uint8_t *msg, int func)
{
    if (msg[7] == func + 0x80)
    {
        switch (msg[8])
        {
        case EX_ILLEGAL_FUNCTION:
            printf("1 Illegal Function\n");
            break;
        case EX_ILLEGAL_ADDRESS:
            printf("2 Illegal Address\n");
            break;
        case EX_ILLEGAL_VALUE:
            printf("3 Illegal Value\n");
            break;
        case EX_SERVER_FAILURE:
            printf("4 Server Failure\n");
            break;
        case EX_ACKNOWLEDGE:
            printf("5 Acknowledge\n");
            break;
        case EX_SERVER_BUSY:
            printf("6 Server Busy\n");
            break;
        case EX_NEGATIVE_ACK:
            printf("7 Negative Acknowledge\n");
            break;
        case EX_MEM_PARITY_PROB:
            printf("8 Memory Parity Problem\n");
            break;
        case EX_GATEWAY_PROBLEMP:
            printf("10 Gateway Path Unavailable\n");
            break;
        case EX_GATEWAY_PROBLEMF:
            printf("11 Gateway Target Device Failed to Respond\n");
            break;
        default:
            printf("UNK\n");
            break;
        }
    }
}

int modbus_write_register(uint16_t address, const uint16_t &value)
{
        modbus_write(address, 1, WRITE_REG, &value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbus_receive(to_rec);
        if (k == -1)
        {
            printf("bad connection\n");
            return -1;
        }
        modbuserror_handle(to_rec, WRITE_COIL);
        return 0;
}

int modbusExample()
{   
    init();
    setConfigurations("192.168.13.165","255.255.255.0","192.168.11.1","192.168.13.165");
    socketConfiguration();
    modbus_set_slave_id(1);
    modbus_write_register(0, 123);
    // create a modbus object
    // modbus mb = modbus("127.0.0.1", 502);

    // // set slave id
    // mb.modbus_set_slave_id(1);

    // // connect with the server
    // mb.modbus_connect();

    // // read coil                        function 0x01
    // bool read_coil;
    // mb.modbus_read_coils(0, 1, &read_coil);

    // // read input bits(discrete input)  function 0x02
    // bool read_bits;
    // mb.modbus_read_input_bits(0, 1, &read_bits);

    // // read holding registers           function 0x03
    // uint16_t read_holding_regs[1];
    // mb.modbus_read_holding_registers(0, 1, read_holding_regs);

    // // read input registers             function 0x04
    // uint16_t read_input_regs[1];
    // mb.modbus_read_input_registers(0, 1, read_input_regs);

    // // write single coil                function 0x05
    // mb.modbus_write_coil(0, true);

    // // write single reg                 function 0x06
    // mb.modbus_write_register(0, 123);

    // // write multiple coils             function 0x0F
    // bool write_cols[4] = {true, true, true, true};
    // mb.modbus_write_coils(0, 4, write_cols);

    // // write multiple regs              function 0x10
    // uint16_t write_regs[4] = {123, 123, 123};
    // mb.modbus_write_registers(0, 4, write_regs);

    // // close connection and free the memory
    // mb.modbus_close();
    // return 0;
}
