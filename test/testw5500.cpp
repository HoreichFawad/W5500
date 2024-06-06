#include "mbed.h"
#include "w5500_header.hpp"


void runW5500Test() {
    SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
    W5500 obj(&spi, PB_12, PA_10);//, nRESET(p9); // reset pin is dummy, don't affect any pin of WIZ550io
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    uint8_t version=obj.getVERSIONR();
    if(version == 0x04)
    {
        printf("Chip version is correct\n");
    }
    obj.setMR(0x04);
    uint8_t val=obj.getMR();
    if(val==0x04)
        printf("MR has been configured successfully\n");
    uint8_t gatewayAddress[4]={192,168,0,1};
    obj.setGAR(&gatewayAddress[0]);
    obj.getGAR(&gatewayAddress[0]);
    if(gatewayAddress[0]==192 && gatewayAddress[1]==168 && gatewayAddress[2]==0 && gatewayAddress[3]==1)
    {
        printf("GAR has been configured successfully\n");
    }
    obj.setIMR(0xF0);
    val=obj.getIMR();
    if(val==0xF0)
        printf("IMR has been configured successfully\n");
    obj.setRTR(0x0FA0);
    uint16_t value=obj.getRTR();
    if(value==0x0FA0){
        printf("RTR has been configured successfully\n");
    }
    obj.setPTIMER(0xC8);
    val=obj.getPTIMER();
    if(val==0xC8)
    {
        printf("PTIMER has been configured successfully\n");
    }
    obj.setSn_MR(0,0x01);
    val=obj.getSn_MR(0);
    if(val==0x01)
    {
        printf("Protocol has been set to TCP\n");
    }
    val=obj.getSn_SR(0);
    printf("The status of socket 0 is %d \n",val);
    obj.setSn_PORT(0,0x1388);
    value=obj.getSn_PORT(0);
    if(value==0x1388)
    {
        printf("Port is configured to %d\n",value);
    }
}