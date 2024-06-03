#include "W5500_header.hpp"
#include "mbed.h"
#include "mbed_debug.h"
#include "DNSClient.h"
#ifdef USE_W5500

// Debug is disabled by default
#define w5500_DBG 0

#if w5500_DBG
#define DBG(...)                                                 \
    do                                                           \
    {                                                            \
        debug("%p %d %s ", this, __LINE__, __PRETTY_FUNCTION__); \
        debug(__VA_ARGS__);                                      \
    } while (0);
// #define DBG(x, ...) debug("[W5500:DBG]"x"\r\n", ##__VA_ARGS__);
#define WARN(x, ...) debug("[W5500:WARN]" x "\r\n", ##__VA_ARGS__);
#define ERR(x, ...) debug("[W5500:ERR]" x "\r\n", ##__VA_ARGS__);
#define INFO(x, ...) debug("[W5500:INFO]" x "\r\n", ##__VA_ARGS__);
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define INFO(x, ...)
#endif

#define DBG_SPI 0

#if !defined(MBED_CONF_W5500_SPI_SPEED)
#define MBED_CONF_W5500_SPI_SPEED 100000
#endif
#define BYTE_PLACEHOLDER "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BIN(byte)          \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

#define debug_print(...) printf(__VA_ARGS__)
#else
#define debug_print(...)
#endif

using namespace std::chrono_literals;
W5500* W5500::inst;

W5500::W5500(PinName mosi, PinName miso, PinName sclk, PinName _cs, PinName _reset):
    cs(_cs), reset_pin(_reset)
{
    spi = new SPI(mosi, miso, sclk);
    cs = 1;
    reset_pin = 1;
    inst = this;
    sock_any_port = SOCK_ANY_PORT_NUM;
}

W5500::W5500(SPI* spi, PinName _cs, PinName _reset):
    cs(_cs), reset_pin(_reset)
{
    this->spi = spi;
    cs = 1;
    reset_pin = 1;
    inst = this;
    sock_any_port = SOCK_ANY_PORT_NUM;
}
// W5500::W5500(
//     PinName cs,
//     PinName sclk,
//     PinName miso,
//     PinName mosi,
//     PinName reset_pin,
//     uint32_t frequency

//     ) : _cs_pin(cs),
//         _sclk_pin(sclk),
//         _miso_pin(miso),
//         _mosi_pin(mosi),
//         _reset_pin(reset_pin),
//         _spi(nullptr),
//         _cs(nullptr),
//         _miso(nullptr),
//         _mosi(nullptr),
//         _sclk(nullptr),
//         _isr(nullptr),
//         _isr_idle(nullptr),
//         _frequency(frequency)
// {
//     inst = this;
//     sock_any_port = SOCK_ANY_PORT_NUM;
// }

// void W5500::enable()
// {
//     if (!_spi)
//     {
//         _sclk.reset();
//         _mosi.reset();
//         _miso.reset();
//         _spi = std::make_unique<mbed::SPI>(_mosi_pin, _miso_pin, _sclk_pin);
//         _spi->frequency(_frequency);
//         _spi->format(8, 0); // or _spi.format(8, 3)
//         _cs = std::make_unique<mbed::DigitalInOut>(_cs_pin, PIN_INPUT, PinMode::PullUp, 0);
//     }
// }

/*********************************************************************************
 * PRIVATE MEMBERS
 */

void W5500::spiRead(uint16_t addr, uint8_t cb, uint8_t *buf, uint16_t len)
{
    cs = 0;
    spi->write(addr >> 8);
    spi->write(addr & 0xff);
    spi->write(cb);
    for(int i = 0; i < len; i++) {
        buf[i] = spi->write(0);
    }
    cs = 1;

#if DBG_SPI
    debug("[SPI]R %04x(%02x %d)", addr, cb, len);
    for(int i = 0; i < len; i++) {
        debug(" %02x", buf[i]);
        if (i > 16) {
            debug(" ...");
            break;
        }
    }
    debug("\r\n");
    if ((addr&0xf0ff)==0x4026 || (addr&0xf0ff)==0x4003) {
        wait_ms(200);
    }
#endif
}

void W5500::spiWrite(uint16_t addr, uint8_t cb, const uint8_t *buf, uint16_t len)
{
    cs = 0;
    spi->write(addr >> 8);
    spi->write(addr & 0xff);
    spi->write(cb);
    for(int i = 0; i < len; i++) {
        spi->write(buf[i]);
    }
    cs = 1;

#if DBG_SPI
    debug("[SPI]W %04x(%02x %d)", addr, cb, len);
    for(int i = 0; i < len; i++) {
        debug(" %02x", buf[i]);
        if (i > 16) {
            debug(" ...");
            break;
        }
    }
    debug("\r\n");
#endif
}

uint32_t strToIp(const char *str)
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

void printfBytes(char *str, uint8_t *buf, int len)
{
    printf("%s %d:", str, len);
    for (int i = 0; i < len; i++)
    {
        printf(" %02x", buf[i]);
    }
    printf("\n");
}

void printHex(uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            printf("%p", buf + i);
        }
        printf(" %02x", buf[i]);
        if ((i % 16) == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void debugHex(uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
        {
            debug("%p", buf + i);
        }
        debug(" %02x", buf[i]);
        if ((i % 16) == 15)
        {
            debug("\n");
        }
    }
    debug("\n");
}

/*********************************************************************************
 * PUBLIC MEMBERS
 */

bool W5500::setMac()
{

    for (int i = 0; i < 6; i++)
        reg_wr<uint8_t>(SHAR + i, mac[i]);

    return true;
}

bool W5500::setIP()
{
    reg_wr<uint32_t>(SIPR, ip);
    reg_wr<uint32_t>(GAR, gateway);
    reg_wr<uint32_t>(SUBR, netmask);
    return true;
}

bool W5500::linkStatus()
{
    if ((reg_rd<uint8_t>(PHYCFGR) & 0x01) != 0x01)
        return false;

    return true;
}

bool W5500::setProtocol(int socket, Protocol p)
{
    if (socket < 0)
    {
        return false;
    }
    sreg<uint8_t>(socket, Sn_MR, p);
    return true;
}

bool W5500::connect(int socket, const char *host, int port, int timeout_ms)
{
    if (socket < 0)
    {
        return false;
    }
    sreg<uint8_t>(socket, Sn_MR, TCP);
    scmd(socket, OPEN);
    sreg_ip(socket, Sn_DIPR, host);
    sreg<uint16_t>(socket, Sn_DPORT, port);
    sreg<uint16_t>(socket, Sn_PORT, newPort());
    scmd(socket, CONNECT);
    Timer t;
    t.reset();
    t.start();
    while (!isConnected(socket))
    {
        if (t.read_ms() > timeout_ms)
        {
            return false;
        }
    }
    return true;
}

bool W5500::getHostByName(const char *host, uint32_t *ip)
{
    uint32_t addr = strToIp(host);
    char buf[17];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
    if (strcmp(buf, host) == 0)
    {
        *ip = addr;
        return true;
    }
    DNSClient client;
    if (client.lookup(host))
    {
        *ip = client.ip;
        return true;
    }
    return false;
}

bool W5500::disconnect()
{
    return true;
}

void W5500::reset()
{
    reset_pin = 1;
    reset_pin = 0;
    wait_us(500); // 500us (w5500)
    reset_pin = 1;
    wait_us(400 * 1000); // 400ms (w5500)

#if defined(USE_WIZ550IO_MAC)
    // reg_rd_mac(SHAR, mac); // read the MAC address inside the module
#endif

    // reg_wr_mac(SHAR, mac);

    // set RX and TX buffer size
    for (int socket = 0; socket < MAX_SOCK_NUM; socket++)
    {
        sreg<uint8_t>(socket, Sn_RXBUF_SIZE, 2);
        sreg<uint8_t>(socket, Sn_TXBUF_SIZE, 2);
    }
}

bool W5500::close(int socket)
{
    if (socket < 0)
    {
        return false;
    }
    // if not connected, return
    if (sreg<uint8_t>(socket, Sn_SR) == SOCK_CLOSED)
    {
        return true;
    }
    if (sreg<uint8_t>(socket, Sn_MR) == TCP)
    {
        scmd(socket, DISCON);
    }
    scmd(socket, CLOSE);
    sreg<uint8_t>(socket, Sn_IR, 0xff);
    return true;
}

int W5500::waitReadable(int socket, int wait_time_ms, int req_size)
{
    if (socket < 0)
    {
        return -1;
    }
    Timer t;
    t.reset();
    t.start();
    while (1)
    {
        // int size = sreg<uint16_t>(socket, Sn_RX_RSR);
        int size, size2;
        // during the reading Sn_RX_RSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        do
        {
            size = sreg<uint16_t>(socket, Sn_RX_RSR);
            size2 = sreg<uint16_t>(socket, Sn_RX_RSR);
        } while (size != size2);

        if (size > req_size)
        {
            return size;
        }
        if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms)
        {
            break;
        }
    }
    return -1;
}

int W5500::waitWriteable(int socket, int wait_time_ms, int req_size)
{
    if (socket < 0)
    {
        return -1;
    }
    Timer t;
    t.reset();
    t.start();
    while (1)
    {
        // int size = sreg<uint16_t>(socket, Sn_TX_FSR);
        int size, size2;
        // during the reading Sn_TX_FSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        do
        {
            size = sreg<uint16_t>(socket, Sn_TX_FSR);
            size2 = sreg<uint16_t>(socket, Sn_TX_FSR);
        } while (size != size2);
        if (size > req_size)
        {
            return size;
        }
        if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms)
        {
            break;
        }
    }
    return -1;
}

int W5500::send(int socket, const char *str, int len)
{
    if (socket < 0)
    {
        return -1;
    }
    uint16_t ptr = sreg<uint16_t>(socket, Sn_TX_WR);
    uint8_t cntl_byte = (0x14 + (socket << 5));
    spiWrite(ptr, cntl_byte, (uint8_t *)str, len);
    sreg<uint16_t>(socket, Sn_TX_WR, ptr + len);
    scmd(socket, SEND);
    uint8_t tmp_Sn_IR;
    while (((tmp_Sn_IR = sreg<uint8_t>(socket, Sn_IR)) & INT_SEND_OK) != INT_SEND_OK)
    {
        // @Jul.10, 2014 fix contant name, and udp sendto function.
        switch (sreg<uint8_t>(socket, Sn_SR))
        {
        case SOCK_CLOSED:
            close(socket);
            return 0;
            // break;
        case SOCK_UDP:
            // ARP timeout is possible.
            if ((tmp_Sn_IR & INT_TIMEOUT) == INT_TIMEOUT)
            {
                sreg<uint8_t>(socket, Sn_IR, INT_TIMEOUT);
                return 0;
            }
            break;
        default:
            break;
        }
    }
    sreg<uint8_t>(socket, Sn_IR, INT_SEND_OK);

    return len;
}

int W5500::recv(int socket, char *buf, int len)
{
    if (socket < 0)
    {
        return -1;
    }
    uint16_t ptr = sreg<uint16_t>(socket, Sn_RX_RD);
    uint8_t cntl_byte = (0x18 + (socket << 5));
    spiRead(ptr, cntl_byte, (uint8_t *)buf, len);
    sreg<uint16_t>(socket, Sn_RX_RD, ptr + len);
    scmd(socket, RECV);
    return len;
}

int W5500::newSocket()
{
    for (int s = 0; s < MAX_SOCK_NUM; s++)
    {
        if (sreg<uint8_t>(s, Sn_SR) == SOCK_CLOSED)
        {
            return s;
        }
    }
    return -1;
}

uint16_t W5500::newPort()
{
    uint16_t port = rand();
    port |= 49152;
    return port;
}

void W5500::scmd(int socket, Command cmd)
{
    sreg<uint8_t>(socket, Sn_CR, cmd);
    while (sreg<uint8_t>(socket, Sn_CR));
}
