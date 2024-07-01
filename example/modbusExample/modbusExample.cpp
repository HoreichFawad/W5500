#include "w5500example.hpp"
#include "w5500_header.hpp"
#include "testw5500.hpp"
#include "TCPSocketServer.h"
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"
#include "modbusExample.hpp"

ModbusTCPClient::ModbusTCPClient(W5500* w5500) : networkInterface(w5500)
{
    _slaveid = 1;
    _msg_id = 1;
    _connected = false;
    err = false;
    err_no = 0;
    error_msg = "";
}

bool ModbusTCPClient::connect()
{
    _connected = networkInterface->establishConnection();
    if(_connected)
        printf("Connection Established\n");
    return _connected;
}

void ModbusTCPClient::modbusSetSlaveId(int id)
{
     _slaveid=id;
}

void ModbusTCPClient::modbusBuildRequest(uint8_t *to_send, uint16_t address, int func) const
{
    to_send[0] = (uint8_t)(_msg_id>>8u);
    to_send[1] = (uint8_t)(_msg_id & 0x00FFu);
    to_send[2] = 0;
    to_send[3] = 0;
    to_send[4] = 0;
    to_send[6] = (uint8_t)_slaveid;
    to_send[7] = (uint8_t)func;
    to_send[8] = (uint8_t)(address >> 8u);
    to_send[9] = (uint8_t)(address & 0x00FFu);
}

ssize_t ModbusTCPClient::modbusSend(uint8_t *to_send, size_t length)
{
    _msg_id++;
    return networkInterface->socket0Send(to_send,length);
}

int ModbusTCPClient::modbusWrite(uint16_t address, uint16_t amount, int func, const uint16_t *value)
{
    int status = 0;
    uint8_t *to_send;
    if (func == WRITE_COIL || func == WRITE_REG)
    {
        to_send = new uint8_t[12];
        modbusBuildRequest(to_send, address, func);
        to_send[5] = 6;
        to_send[10] = (uint8_t)(value[0] >> 8u);
        to_send[11] = (uint8_t)(value[0] & 0x00FFu);
        status = modbusSend(to_send, 12);
    }
    else if (func == WRITE_REGS)
    {
        to_send = new uint8_t[13 + 2 * amount];
        modbusBuildRequest(to_send, address, func);
        to_send[5] = (uint8_t)(7 + 2 * amount);
        to_send[10] = (uint8_t)(amount >> 8u);
        to_send[11] = (uint8_t)(amount & 0x00FFu);
        to_send[12] = (uint8_t)(2 * amount);
        for (int i = 0; i < amount; i++)
        {
            to_send[13 + 2 * i] = (uint8_t)(value[i] >> 8u);
            to_send[14 + 2 * i] = (uint8_t)(value[i] & 0x00FFu);
        }
        status = modbusSend(to_send, 13 + 2 * amount);
    }
    else if (func == WRITE_COILS)
    {
        to_send = new uint8_t[14 + (amount - 1) / 8];
        modbusBuildRequest(to_send, address, func);
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
        status = modbusSend(to_send, 14 + (amount - 1) / 8);
    }
    delete[] to_send;
    return status;
}

ssize_t ModbusTCPClient::modbusReceive(uint8_t* buf) const
{
    return networkInterface->socket0Receive(buf);
}

void ModbusTCPClient::modbusErrorHandle(const uint8_t *msg, int func)
{
    err = false;
    error_msg = "NO ERR";
    if (msg[7] == func + 0x80)
    {
        err = true;
        switch (msg[8])
        {
        case EX_ILLEGAL_FUNCTION:
            error_msg = "1 Illegal Function";
            break;
        case EX_ILLEGAL_ADDRESS:
            error_msg = "2 Illegal Address";
            break;
        case EX_ILLEGAL_VALUE:
            error_msg = "3 Illegal Value";
            break;
        case EX_SERVER_FAILURE:
            error_msg = "4 Server Failure";
            break;
        case EX_ACKNOWLEDGE:
            error_msg = "5 Acknowledge";
            break;
        case EX_SERVER_BUSY:
            error_msg = "6 Server Busy";
            break;
        case EX_NEGATIVE_ACK:
            error_msg = "7 Negative Acknowledge";
            break;
        case EX_MEM_PARITY_PROB:
            error_msg = "8 Memory Parity Problem";
            break;
        case EX_GATEWAY_PROBLEMP:
            error_msg = "10 Gateway Path Unavailable";
            break;
        case EX_GATEWAY_PROBLEMF:
            error_msg = "11 Gateway Target Device Failed to Respond";
            break;
        default:
            error_msg = "UNK";
            break;
        }
    }
}

void ModbusTCPClient::setBadCon()
{
    err = true;
    error_msg = "BAD CONNECTION";
}

void ModbusTCPClient::setBadInput()
{
    err = true;
    error_msg = "BAD FUNCTION INPUT";
}

int ModbusTCPClient::modbusRead(uint16_t address, uint16_t amount, int func)
{
    uint8_t to_send[12];
    modbusBuildRequest(to_send, address, func);
    to_send[5] = 6;
    to_send[10] = (uint8_t)(amount >> 8u);
    to_send[11] = (uint8_t)(amount & 0x00FFu);
    return modbusSend(to_send, 12);
}


int ModbusTCPClient::modbusReadCoils(uint16_t address, uint16_t amount, bool *buffer)
{
    if (_connected)
    {
        if (amount > 2040)
        {
            setBadInput();
            return EX_BAD_DATA;
        }
        modbusRead(address, amount, READ_COILS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, READ_COILS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++)
        {
            buffer[i] = (bool)((to_rec[9u + i / 8u] >> (i % 8u)) & 1u);
        }
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusReadInputBits(uint16_t address, uint16_t amount, bool *buffer)
{
    if (_connected)
    {
        if (amount > 2040)
        {
            setBadInput();
            return EX_BAD_DATA;
        }
        modbusRead(address, amount, READ_INPUT_BITS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++)
        {
            buffer[i] = (bool)((to_rec[9u + i / 8u] >> (i % 8u)) & 1u);
        }
        modbusErrorHandle(to_rec, READ_INPUT_BITS);
        return 0;
    }
    else
    {
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusReadHoldingRegisters(uint16_t address, uint16_t amount, uint16_t *buffer)
{
    if (_connected)
    {
        modbusRead(address, amount, READ_REGS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, READ_REGS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++)
        {
            buffer[i] = ((uint16_t)to_rec[9u + 2u * i]) << 8u;
            buffer[i] += (uint16_t)to_rec[10u + 2u * i];
        }
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusReadInputRegisters(uint16_t address, uint16_t amount, uint16_t *buffer)
{
    if (_connected)
    {
        modbusRead(address, amount, READ_INPUT_REGS);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, READ_INPUT_REGS);
        if (err)
            return err_no;
        for (auto i = 0; i < amount; i++)
        {
            buffer[i] = ((uint16_t)to_rec[9u + 2u * i]) << 8u;
            buffer[i] += (uint16_t)to_rec[10u + 2u * i];
        }
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusWriteCoil(uint16_t address, const bool &to_write)
{
    if (_connected)
    {
        int value = to_write * 0xFF00;
        modbusWrite(address, 1, WRITE_COIL, (uint16_t *)&value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, WRITE_COIL);
        if (err)
            return err_no;
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusWriteCoils(uint16_t address, uint16_t amount, const bool *value)
{
    if (_connected)
    {
        uint16_t *temp = new uint16_t[amount];
        for (int i = 0; i < amount; i++)
        {
            temp[i] = (uint16_t)value[i];
        }
        modbusWrite(address, amount, WRITE_COILS, temp);
        delete[] temp;
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, WRITE_COILS);
        if (err)
            return err_no;
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}

int ModbusTCPClient::modbusWriteRegisters(uint16_t address, uint16_t amount, const uint16_t *value)
{
    if (_connected)
    {
        modbusWrite(address, amount, WRITE_REGS, value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, WRITE_REGS);
        if (err)
            return err_no;
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}


int ModbusTCPClient::modbusWriteRegister(uint16_t address, const uint16_t &value)
{
    if (_connected)
    {
        modbusWrite(address, 1, WRITE_REG, &value);
        uint8_t to_rec[MAX_MSG_LENGTH];
        ssize_t k = modbusReceive(to_rec);
        if (k == -1)
        {
            setBadCon();
            return BAD_CON;
        }
        modbusErrorHandle(to_rec, WRITE_COIL);
        if (err)
            return err_no;
        return 0;
    }
    else
    {
        setBadCon();
        return BAD_CON;
    }
}
ModbusTCPClient::~ModbusTCPClient(){

}
int modbusExample()
{   
    SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
    W5500 w5500(&spi, PB_12, PA_10);
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    uint8_t MAC_Addrc[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};
    w5500.socket0ConfigModbus("192.168.13.164","255.255.255.0","192.168.11.1","192.168.13.165",&MAC_Addrc[0]);
    ModbusTCPClient modbus(&w5500);
    modbus.connect();
    modbus.modbusSetSlaveId(11);

    // // read coil                        function 0x01
    // // bool read_coil;
    // modbus.modbusReadCoils(1, 1, &read_coil);

    // // read input bits(discrete input)  function 0x02
    // bool read_bits;
    // modbus.modbusReadInputBits(3, 1, &read_bits);

    // // read holding registers           function 0x03
    // uint16_t read_holding_regs[1];
    // modbus.modbusReadHoldingRegisters(0, 1, read_holding_regs);

    // // read input registers             function 0x04
    // uint16_t read_input_regs[1];
    // modbus.modbusReadInputRegisters(0, 1, read_input_regs);

    // // write single coil                function 0x05
    // modbus.modbusWriteCoil(0, true);

    // // write single reg                 function 0x06
    // modbus.modbusWriteRegister(0, 123);

    // // write multiple coils             function 0x0F
    bool write_cols[4] = {false, true, false, true};
    modbus.modbusWriteCoils(0, 4, write_cols);

    // // write multiple regs              function 0x10
    // uint16_t write_regs[4] = {21, 22, 23,24};
    // modbus.modbusWriteRegisters(4, 4, write_regs);

    // // close connection and free the memory
    // networkInterface.close(0);
    return 0;
}