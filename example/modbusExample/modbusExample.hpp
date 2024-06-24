#pragma once
#include <stdint.h>
#include <stddef.h>
int modbusExample();
int modbus_write_register(uint16_t address, const uint16_t &value);
void modbuserror_handle(const uint8_t *msg, int func);
ssize_t modbus_receive(uint8_t* buf);
int modbus_write(uint16_t address, uint16_t amount, int func, const uint16_t *value);
ssize_t modbus_send(uint8_t *to_send, size_t length);
void modbus_build_request(uint8_t *to_send, uint16_t address, int func);
bool socketConfiguration();
bool setConfigurations(const char* IP_Addrc, const char* IP_Subnetc, const char* IP_Gatewayc, const char* DIP_Addr);
bool init();
uint32_t strToIP_(const char *str);