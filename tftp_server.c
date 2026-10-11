/*
NAME :          NADEEM BASHEER USMAN
PROJECT :       TRIVIAL FILE TRANSFER PROTOCOL (TFTP)
DESSCRIPTION :  Implemented a TFTP client–server application in C using UDP sockets that supports file upload (PUT/WRQ) and download (GET/RRQ).
                The project is organized into separate client and server modules (folders) and runs on two terminals, one for the server and one for the client. 
                File transfer occurs in 512-byte blocks with ACK-based reliability, supporting Octet (binary) and NetASCII (text) modes.
*/
#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<fcntl.h>

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;


    // Create UDP socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
    {
        printf("socket creation failed\n");
        exit(1);
    }
    
    // Set socket timeout option
    //TODO Use setsockopt() to set timeout option

    
    // Set up server address
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY is used to accept all types of addresses , including loopback 

    // Bind the socket
    if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) //if server fails to bind it prints the error and exits
    {
        perror("bind failed\n");
        close(sockfd);
        exit(1);
    }

    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while (1) {
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("Receive failed or timeout occurred");
            continue;
        }

        handle_client(sockfd, client_addr, client_len, &packet);   
    }

    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    // Extract the TFTP operation (read or write) from the received packet
    int opcode = ntohs(packet->opcode); //converting netwrok order to byte order

    // and call send_file or receive_file accordingly
    if(opcode == RRQ)
    {
        send_file(sockfd,client_addr,client_len,packet->body.request.filename,packet->body.request.mode);
    }
    else if(opcode == WRQ)
    {
        receive_file(sockfd,client_addr,client_len,packet->body.request.filename,packet->body.request.mode);
    }
    else
    {
        printf("Invalid request recieved\n");
    }
}




