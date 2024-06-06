#include "mbed.h"
#include "w5500_header.hpp"
#include "TCPSocketServer.h"
#include "TCPSocketConnection.h"
#include "EthernetInterface.h"

int serverExampleTCP() {
    SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
    EthernetInterface net(&spi, PB_12, PA_10);
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    printf("Starting server...\n");

    // Bring up the network interface
    int result = net.connect();
    if (result != 0) {
        printf("Error connecting: %d\n", result);
        return -1;
    }
    printf("Connected. IP address: %s\n", net.getIPAddress());

    // Create a TCP socket
    TCPSocketServer server;
    // server.open(&net);

    // Bind the socket to port 5000
    server.bind(5000);

    // Listen for incoming connections
    server.listen();

    // Accept a new connection
    TCPSocketConnection* newConnection;
    result=server.accept(*newConnection);
    if(result==0)
        printf("Connection accepted\n");
    else
        printf("Connection failed\n");
    // Read data from the client
    char buffer[256];
    int len = newConnection->receive(buffer, sizeof(buffer));
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate the received string
        printf("Received: %s\n", buffer);

        // Echo the data back to the client
        newConnection->send(buffer, len);
    }

    // Close the socket
    newConnection->close();
    server.close();

    // Bring down the network interface
    net.disconnect();

    printf("Server done\n");
}

int clientExampleTCP() {
    SPI spi(PB_15, PB_14, PB_13); // mosi, miso, sclk
    EthernetInterface net(&spi, PB_12, PA_10);
    spi.format(8,0); // 8bit, mode 0
    spi.frequency(1000000); // 1MHz
    wait_us(1000*1000); // 1 second for stable state
    printf("Starting client...\n");
    // Bring up the network interface
    int result = net.connect();
    if (result != 0) {
        printf("Error connecting: %d\n", result);
        return -1;
    }
    printf("Connected. IP address: %s\n", net.getIPAddress());

    // Create a TCP socket
    TCPSocketConnection socket;
    // socket.open(&net);

    // Connect to the server
    result = socket.connect("0.0.0.0",5000);
    if (result != 0) {
        printf("Error connecting to server: %d\n", result);
        return -1;
    }

    // Send data to the server
    char *message = "Hello, Server!";
    socket.send(message, strlen(message));

    // Receive data from the server
    char buffer[256];
    int len = socket.receive(buffer, sizeof(buffer));
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate the received string
        printf("Received from server: %s\n", buffer);
    }

    // Close the socket
    socket.close();

    // Bring down the network interface
    net.disconnect();

    printf("Client done\n");
}