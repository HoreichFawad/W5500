#include "mbed.h"
#include "w5500_header.hpp"


void runW5500Test() {
    W5500 obj;
    obj.enable();
    uint8_t version=obj.getVERSIONR();
    if(version == 0x04)
    {
        printf("Chip version is correct");
    }
    printf("Version is %d\n",version);
}     