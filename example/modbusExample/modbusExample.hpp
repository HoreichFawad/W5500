#include <cstring>
#include <stdint.h>
#include <string>
#include "mbed.h"
#include "EthernetInterface.h"

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

#define BAD_CON -1
class ModbusTCPClient
{
    public:
        bool err{};
        int err_no{};
        std::string error_msg;

        ModbusTCPClient(const char* IP_Addrc, const char* IP_Subnetc, const char* IP_Gatewayc, const char* DIP_Addr,uint8_t* MAC_Addrc);
        ~ModbusTCPClient();

        void modbusSetSlaveId(int id);

        int modbusReadCoils(uint16_t address, uint16_t amount, bool *buffer);
        int modbusReadInputBits(uint16_t address, uint16_t amount, bool *buffer);
        int modbusReadHoldingRegisters(uint16_t address, uint16_t amount, uint16_t *buffer);
        int modbusReadInputRegisters(uint16_t address, uint16_t amount, uint16_t *buffer);

        int modbusWriteCoil(uint16_t address, const bool &to_write);
        int modbusWriteRegister(uint16_t address, const uint16_t &value);
        int modbusWriteCoils(uint16_t address, uint16_t amount, const bool *value);
        int modbusWriteRegisters(uint16_t address, uint16_t amount, const uint16_t *value);
        bool socketConfiguration();
        bool setConfigurations(const char* IP_Addrc, const char* IP_Subnetc, const char* IP_Gatewayc, const char* DIP_Addr);
        bool init();
    private:
        bool _connected{};
        uint16_t PORT{};
        uint32_t _msg_id{};
        int _slaveid{};
        std::string HOST;

        void modbusBuildRequest(uint8_t *to_send, uint16_t address, int func) const;

        int modbusRead(uint16_t address, uint16_t amount, int func);
        int modbusWrite(uint16_t address, uint16_t amount, int func, const uint16_t *value);

        ssize_t modbusSend(uint8_t *to_send, size_t length);
        ssize_t modbusReceive(uint8_t *buffer) const;

        void modbusErrorHandle(const uint8_t *msg, int func);

        void setBadCon();
        void setBadInput();
    };
    int modbusExample();
    uint32_t strToIP_(const char *str);
