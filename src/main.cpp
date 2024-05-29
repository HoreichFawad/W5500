
#include "mbed.h"
#include "w5500_header.hpp"


int main() {
    WIZnet_Chip obj;
    obj.enable();
    // uint8_t buf[6] 
    uint8_t buffer=0b11011000; 
            //       10011000
    obj.spiWrite(0x001C,0x04,&buffer,1);
    obj.spiRead(0x001C,0x00,&buffer,1);
    uint8_t version=obj.getVERSIONR();
    if(version == 0x04)
    {
        printf("Chip version is correct");
    }
    printf("Version is %d\n",version);
    printf("%d\n",buffer);
}          