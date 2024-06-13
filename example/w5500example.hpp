#pragma once
#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE			2048
#endif
int clientExampleTCP();
int serverExampleTCP();
int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port);
uint32_t strToIP_(const char *str);