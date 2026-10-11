/* Common file for server & client */

#include "tftp.h"

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename,char* mode) 
{
    // Implement file sending logic here
    tftp_packet packet;     //declaring structure for sending the packet
    tftp_packet ack;        //declaring the structure for recieving the acknowledgent
    socklen_t len = sizeof(client_addr);
    int fd = open(filename,O_RDONLY);
    if(fd < 0)
    {
        printf("Failed to open file in server\n");
        return;
    }
    int block_no = 1;
    while(1)
    {
        if(strcmp(mode,"default") == 0)
        {
            packet.opcode = htons(DATA); //SETTING OP CODE AS DATA
            packet.body.data_packet.block_number = htons(block_no);

            char buffer[512];
            int bytes = read(fd,buffer,512);
            if(bytes < 0)
            {
                printf("Error reading file(server)\n"); //if read function fails
                close(fd);
                return;
            }
            
            if(bytes == 0) //reached EOF
            {
                break;
            }

            memcpy(packet.body.data_packet.data,buffer,bytes); //memcpy is used because data can be anything

            sendto(sockfd,&packet,4+bytes,0,(struct sockaddr*)&client_addr,client_len);

            int n = recvfrom(sockfd,&ack,BUFFER_SIZE,0,(struct sockaddr*)&client_addr,&client_len);

            if(n<0)
            {
                printf("failed to recieve acknowledgment\n");
                close(fd);
                return;
            }

            block_no++;

            if(bytes < 512)
            {
                break; //stop reading if last bytes have also been read;
            }
        }

        
        else if(strcmp(mode,"octet") == 0)
        {
            char buff[1];
            int bytes = read(fd,buff,1);

            if(bytes < 0)
            {
                printf("Error reading file(server)\n"); //if read function fails
                close(fd);
                return;
            }

            if(bytes == 0) //sending empty packet so client recieves the empoty packet and stops
            {
                sendto(sockfd,&packet,4,0,(struct sockaddr*)&client_addr,client_len);
                break;
            }
            memcpy(packet.body.data_packet.data,buff,1); //memcpy used because data can be anything
            sendto(sockfd,&packet,4+1,0,(struct sockaddr*)&client_addr,client_len);

            int n = recvfrom(sockfd,&ack,BUFFER_SIZE,0,(struct sockaddr*)&client_addr,&client_len);

            if(n<0)
            {
                printf("failed to recieve acknowledgment\n");
                close(fd);
                return;
            }

            block_no++;
        }

        if(strcmp(mode,"netascii") == 0)
        {
            char buff[3];
            int bytes = read(fd,buff,1);

            if(bytes < 0)
            {
                printf("Error reading file(server)\n"); //if read function fails
                close(fd);
                return;
            }

            if(bytes == 0) //send an empty packet
            {
                sendto(sockfd,&packet,4,0,(struct sockaddr*)&client_addr,client_len);
                break;
            }

            if(buff[0] == '\n')
            {
                buff[0] = '\r';
                buff[1] = '\n';

                memcpy(packet.body.data_packet.data,buff,2);

                sendto(sockfd,&packet,4+2,0,(struct sockaddr*)&client_addr,client_len);
            }
            else
            {
                memcpy(packet.body.data_packet.data,buff,2);
                sendto(sockfd,&packet,4+1,0,(struct sockaddr*)&client_addr,client_len);
            }

            int n = recvfrom(sockfd,&ack,BUFFER_SIZE,0,(struct sockaddr*)&client_addr,&client_len);

            if(n<0)
            {
                printf("failed to recieve acknowledgment\n");
                close(fd);
                return;
            }

            block_no++;
        }
    }
    close(fd);
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename,char* mode) 
{
    // Implement file receiving logic here
    tftp_packet packet;
    tftp_packet ack;

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); //opening file creating if file not present
    if(fd < 0)
    {
        printf("Failed to open file in server\n");
        return;
    }

    ack.opcode = htons(ACK);
    ack.body.data_packet.block_number = htons(0);

    sendto(sockfd,&ack,sizeof(ack),0,(struct sockaddr*)&client_addr,client_len);

    while(1)
    {
        int n = recvfrom(sockfd,&packet,BUFFER_SIZE,0,(struct sockaddr*)&client_addr,&client_len);

        if(n < 0)
        {
            printf("Failed to receive DATA\n");
            close(fd);
            return;
        }

        if(ntohs(packet.opcode) == DATA)
        {
            int data_len = n - 4; //subtracting 2 bytes opcode and 2 bytes block number
            if(strcmp(mode,"netascii")== 0)
            {
                for(int i = 0;i<data_len;i++)
                {
                    if(i+1 && packet.body.data_packet.data[i] == '\r' && packet.body.data_packet.data[i+1] == '\n')
                    {
                        write(fd,"\n",1);
                        i++; //skip the \r
                    }
                    else
                    {
                        write(fd,&packet.body.data_packet.data[i],1);
                    }
                }
            } 
            else
            {
                write(fd,packet.body.data_packet.data,data_len); //else write the recieved file as it is

            }

            ack.opcode = htons(ACK); //sending back acknowledgment when reciebed
            ack.body.ack_packet.block_number = packet.body.data_packet.block_number;

            sendto(sockfd,&ack,sizeof(ack),0,(struct sockaddr*)&client_addr,client_len);

            if(strcmp(mode,"default")==0 && data_len < 512)
            {
                break; //if recieved data is less than 512 means last packet
            }
            else if(strcmp(mode,"default") != 0 && data_len == 0)
            {
                break; //it is the last empty packet
            }
        }
    }
    close(fd);
}