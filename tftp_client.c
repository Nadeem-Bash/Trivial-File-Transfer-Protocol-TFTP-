/*
NAME :          NADEEM BASHEER USMAN
PROJECT :       TRIVIAL FILE TRANSFER PROTOCOL (TFTP)
DESSCRIPTION :  Implemented a TFTP client–server application in C using UDP sockets that supports file upload (PUT/WRQ) and download (GET/RRQ).
                The project is organized into separate client and server modules (folders) and runs on two terminals, one for the server and one for the client. 
                File transfer occurs in 512-byte blocks with ACK-based reliability, supporting Octet (binary) and NetASCII (text) modes.
*/
#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

int current_mode = MODE_DEFAULT; //setting the mode as default

int main() {
    char command[256];
    tftp_client_t client;
    memset(&client, 0, sizeof(client));  // Initialize client structure
    printf("Expected syntax:\n");
    printf("1. connect <ip_address> <port_number>\n");
    printf("2. mode <1|2|3>\n");
    printf("2. get <filename>\n");
    printf("3. put <filename>\n");
    printf("4. quit\n");

    // Main loop for command-line interface
    while (1) {
        printf("tftp> ");
        fgets(command, sizeof(command), stdin);

        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        // Process the command
        process_command(&client, command);
    }

    return 0;
}

// Function to process commands
void process_command(tftp_client_t *client, char *command) 
{ 
    char* command1 = strtok(command," ");
    if(command1 == NULL)
    {
        return;
    }

    if(strcmp(command1, "connect") == 0) 
    {
        //if user give 'connect' command then use strtok to get the ipaddress and port no. 
        char* ip = strtok(NULL, " "); 
        char* port = strtok(NULL, " ");

        if(ip && port)
        {
            connect_to_server(client,ip,atoi(port)); //calling the connect to server function if ipaddress and port number is given
        }
        else
        {
            printf("Usage : connect <ip> <port>\n");
        }
    }

    else if(strcmp(command1, "mode") == 0)
    {
        char* mode = strtok(NULL, " ");
        if(mode)
        {
            int m = atoi(mode); //converting mode to integer
            if(m == 1)
            {
                current_mode = MODE_DEFAULT;
            }
            else if(m == 2)
            {
                current_mode = MODE_OCTET;
            }
            else if(m == 3)
            {
                current_mode = MODE_NETASCII;
            }
            else
            {
                printf("Invalid mode\n");
            }
        }
        else
        {
            printf("Usage : mode <1|2|3>\n");
        }
    }

    else if(strcmp(command1,"get") == 0)
    {
        char* filename = strtok(NULL, " ");

        if(filename)
        {
            get_file(client,filename); //calling function to revieve the file
        }
        else
        {
            printf("Usage : get <filename>\n");
        }
    }

    else if(strcmp(command1,"put") == 0)
    {
        char* filename = strtok(NULL, " "); //calling function to send the file

        if(filename)
        {
            put_file(client,filename);
        }
        else
        {
            printf("Usage : put <filename>\n");
        }
    }

    else if(strcmp(command1,"quit") == 0)
    {
        disconnect(client);
    }

    else
    {
        printf("Unknown command\n");
    }
}

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port) 
{
    // Create UDP socket
    client->sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(client->sockfd < 0)
    {
        perror("Socket creation failed");
        return;
    }

    memset(&client->server_addr,0,sizeof(client->server_addr));     //clearing the server address structure

    // Set socket timeout option


    // Set up server address
    client->server_addr.sin_family = AF_INET;       //AF_INET for ipv4
    client->server_addr.sin_port = htons(port);     //htons is used to convert port to byte order

    if(inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0) //converting the ipstring to binary format(pton) and also validating the ip address
    {
        printf("Invalid IP Adress\n");
        return;
    }

    client->server_len = sizeof(client->server_addr);

    strcpy(client->server_ip,ip); //storing the ip address as a string itself

    printf("Connected to server %s/%d\n",ip,port);

}

void put_file(tftp_client_t *client, char *filename) {
    // Send WRQ request and send file
    int fd;

    fd = open(filename,O_RDONLY);  //opening file in read mode to send
    if(fd < 0)
    {
        printf("Failed to open file\n");
        return;
    }

    printf("Uploading %s...\n",filename);

    send_request(client->sockfd,client->server_addr,filename,WRQ); //sending write request

    receive_request(client->sockfd,client->server_addr,filename,WRQ);// start the file transfer loop

    close(fd); //close the file

}

void get_file(tftp_client_t *client, char *filename) {
    // Send RRQ and recive file 

    send_request(client->sockfd,client->server_addr,filename,RRQ); //sending READ request

    receive_request(client->sockfd,client->server_addr,filename,RRQ);// start the file transfer loop
  
}

void disconnect(tftp_client_t *client) {
    // close fd
    close(client->sockfd);      //Closing the sock in UDP this will disconnect from the server
    client->sockfd = -1;        //it may contain a closed file descriptor
    printf("Disconnected from the server\n");
}

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    tftp_packet packet;
    memset(&packet,0,sizeof(packet)); //clearing the packet

    packet.opcode = htons(opcode); //converting to network byte order

    strcpy(packet.body.request.filename,filename); //uploading the filename

    //setting mode
    if(current_mode ==  MODE_DEFAULT)
    {
        strcpy(packet.body.request.mode,"default");
    }
    else if(current_mode == MODE_OCTET)
    {
        strcpy(packet.body.request.mode,"octet");
    }
    else if(current_mode == MODE_NETASCII)
    {
        strcpy(packet.body.request.mode,"netascii");
    }

    sendto(sockfd,&packet,sizeof(packet),0,(struct sockaddr*)&server_addr,sizeof(server_addr)); //send to function

    printf("%s requested\n",filename);

}

void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    
    if(opcode == RRQ) //get logic
    {
        tftp_packet packet; //declaring the structure for data we are going to recieve
        socklen_t len = sizeof(server_addr);

        tftp_packet ack;//declaring the structure for which we are going to send the acknoledgment
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); //opening the file 
        if(fd < 0)
        {
            printf("failed to open file in client\n");
            return;
        }
        while(1)
        {
            int n = recvfrom(sockfd,&packet,BUFFER_SIZE,0,(struct sockaddr*)&server_addr,&len); //recieve the packet sent by the server
            if(n<0)
            {
                printf("error recieving file\n");
                break;
            }
            if(ntohs(packet.opcode) == DATA)
            {
                int data_len = n-4; //size of data recieved
                if(current_mode == MODE_NETASCII) //if it is netascii we need to change \r\n to \n
                {
                    for(int i = 0;i<data_len;i++)
                    {
                        if(i + 1 < data_len && packet.body.data_packet.data[i] == '\r' && packet.body.data_packet.data[i+1] == '\n')
                        {
                            write(fd,"\n",1); //write \n
                            i++; //skip \n
                        }
                        else
                        {
                            write(fd,&packet.body.data_packet.data[i],1);
                        }
                    }
                }
                else
                {
                    write(fd,packet.body.data_packet.data,data_len);
                }
                memset(&ack,0,sizeof(ack));
                ack.opcode = htons(ACK);
                ack.body.ack_packet.block_number = packet.body.data_packet.block_number;

                sendto(sockfd,&ack,sizeof(ack),0,(struct sockaddr*)&server_addr,len); //sending acknowledgment to server

                if(data_len < 512 && current_mode == MODE_DEFAULT) //last packet is recieved
                {
                    break;
                }
                else if(data_len == 0  && current_mode != MODE_DEFAULT)
                {
                    break;
                }
            }
        }
        close(fd);
    }

    if(opcode == WRQ)
    {
        tftp_packet ack;
        socklen_t len =sizeof(server_addr);

        tftp_packet packet;
        int fd = open(filename,O_RDONLY);
        if(fd < 0)
        {
            printf("failed to open file\n");
            return;
        }
        int block_no = 1;
        while(1)
        {
            int n = recvfrom(sockfd,&ack,BUFFER_SIZE,0,(struct sockaddr*)&server_addr,&len); //recieve the acknowledgment sent by the server
            if(n < 0)
            {
                printf("Failed to recieve acknowledgment\n");
                close(fd);
                return;
            }
            if(ntohs(ack.opcode) == ACK)
            {
                packet.body.data_packet.block_number = htons(block_no);
                packet.opcode = htons(DATA);

                if(current_mode == MODE_DEFAULT)  //for default mode
                {
                    char buff[512];
                    int bytes = read(fd,buff,512);
                    if(bytes < 0) //read error check
                    {
                        printf("read error\n");
                        close(fd);
                        return;
                    }
                    if(bytes == 0) //checking if eof reached
                    {
                        break;
                    }
                    memcpy(packet.body.data_packet.data,buff,bytes); //memcpy used because it can be any data
                    sendto(sockfd,&packet,4+bytes,0,(struct sockaddr*)&server_addr,len);
                    block_no++;
                    if(bytes < 512) //if all bytes have been read
                    {
                        break;
                    }
                }

                else if(current_mode == MODE_OCTET)  //for octet mode
                {
                    char buff[1];
                    int bytes = read(fd,buff,1);
                    if(bytes < 0)
                    {
                        printf("read error\n");
                        close(fd);
                        return;
                    }

                    if(bytes == 0) //checking if EOF reached
                    {
                        sendto(sockfd,&packet,4,0,(struct sockaddr*)&server_addr,len);
                        break;
                    }

                    memcpy(packet.body.data_packet.data,buff,bytes); //memcpy used because it can be any data
                    sendto(sockfd,&packet,4+bytes,0,(struct sockaddr*)&server_addr,len);
                    block_no++;
                }

                else if(current_mode == MODE_NETASCII)  //for netascii mode
                {
                    char buff[3];
                    int bytes = read(fd,buff,1);
                    if(bytes < 0)
                    {
                        printf("read error\n");
                        close(fd);
                        return;
                    }

                    if(bytes == 0) //checking if EOF reached
                    {
                        sendto(sockfd,&packet,4,0,(struct sockaddr*)&server_addr,len);
                        break;
                    }
                    if(buff[0] == '\n')
                    {
                        buff[0] = '\r';
                        buff[1] = '\n';
                        memcpy(packet.body.data_packet.data,buff,bytes+1); //memcpy used because it can be any data
                        sendto(sockfd,&packet,4+2,0,(struct sockaddr*)&server_addr,len);
                    }
                    else
                    {
                    memcpy(packet.body.data_packet.data,buff,bytes); //memcpy used because it can be any data
                    sendto(sockfd,&packet,4+bytes,0,(struct sockaddr*)&server_addr,len);
                    }
                    block_no++;
                }
            }
        }
        close(fd);
    }
}