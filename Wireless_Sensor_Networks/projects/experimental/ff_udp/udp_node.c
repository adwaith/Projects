#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include "udp_node.h"
#include <errno.h>

#define BAUDRATE B115200
#define DATABITS CS8
#define STOPBITS CSTOPB
#define PARITYON 0
#define PARITY 0
#define USB_PORT "/dev/ttyUSB0"


int port_fd = -1;
struct termios serial_tio;
FILE * file;

void main() {
    int udp_pid;
    int flags;	
	bzero(&serial_tio, sizeof(serial_tio));

	port_fd = open(USB_PORT, O_RDWR | O_NOCTTY);
	if (port_fd < 0) {
		perror("Failed to open serial port");
		return;
	}

  flags = fcntl(port_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(port_fd, F_SETFL, flags);

	serial_tio.c_cflag = BAUDRATE | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL| CREAD;
	serial_tio.c_iflag = IGNPAR;
	serial_tio.c_lflag = ICANON;
	serial_tio.c_cc[VMIN]= 1;
	serial_tio.c_cc[VTIME] = 0;
	tcflush(port_fd, TCIFLUSH);
	tcsetattr (port_fd, TCSANOW, &serial_tio);

    if (!(udp_pid = fork())) {
        udp_client();
    }

    udp_server();
}



void udp_client() {
    int sockfd,n;
    int cnt = 0;
    int len, flags;
    struct sockaddr_in servaddr,cliaddr;
    char sendline[MAX_PACKET_LENGTH];
    char rx_buf[MAX_PACKET_LENGTH];

    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
    
    bzero(&servaddr,sizeof(servaddr));
    bzero(&sendline,400);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(UDP_SERVER_ADDR);
    servaddr.sin_port=htons(UDP_SERVER_PORT);

    sleep(1); //let server go first
    while (1) {
        if(cnt == 0) {
            sprintf(sendline,"TAKEOFF");
            sendto(sockfd,sendline,7,0,
                (struct sockaddr *)&servaddr,sizeof(servaddr));
        }
        n = recvfrom(sockfd,rx_buf,MAX_PACKET_LENGTH,0,(struct sockaddr *)&servaddr,&len);
        if(n > 0) {
            printf("RECEIVED - %s\n",rx_buf);
        }
        cnt = (cnt + 1) % 100;
        usleep(50000);
    }

}

void udp_server() {
    int sockfd, n, flags;
    struct sockaddr_in servaddr, cliaddr;
    uint8_t packet_id = 0;
    socklen_t len;
    int res;

    rec_seq = 0;
    curr_id = 255;

    sockfd=socket(AF_INET, SOCK_DGRAM,0);

    flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(UDP_SERVER_ADDR);//htonl(INADDR_ANY);
    servaddr.sin_port=htons(UDP_SERVER_PORT);
    bind(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr));

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port=htons(UDP_SERVER_PORT);

    while (1)
    {
        n = recvfrom(sockfd,server_trans_buf,MAX_PACKET_LENGTH,0,(struct sockaddr *)&cliaddr,&len);
        if(n > 0) {
            udp_transmit_line(n, packet_id);
            packet_id++;
            if(packet_id==10)
                packet_id++;
    //        printf("%s\n",server_trans_buf);
        }
        n=0;
        while(n != -1) { //read packets until no more packets to be read
            n = udp_receive_line();
            if(n > 0) {
                res=sendto(sockfd,server_rec_buf,n,0,(struct sockaddr *)&cliaddr, sizeof(cliaddr));
                if(res < 0)
                    printf("Error sending packet to client - %s\n",strerror(errno));
            }
            usleep(70000);
        }
    }

}

void udp_transmit_line(int packet_len, uint8_t packet_id) {
    int bytesSent;
    int i, n, offset, buf_offset = 0, checksum = 0;

    //Can send as 1 packet
    if (packet_len <= MAX_FRAGMENT_LENGTH) {
        offset = sprintf(trans_line_buffer,"%s",PACKET_IDENT);
        offset += sprintf(&trans_line_buffer[offset],"%c%c%c%c%c",
                (char)packet_id,(char)DEFAULT_TTL,(char)1,(char)1,(char)0);
        //offset += snprintf(&trans_line_buffer[offset],MAX_FRAGMENT_LENGTH,
        //      "%s",server_trans_buf);
        memcpy(&trans_line_buffer[offset],server_trans_buf,packet_len);
        offset+=packet_len;

        for (n = 0; n < offset; n++) checksum ^= trans_line_buffer[n];
        offset += snprintf(&trans_line_buffer[offset],5,"%02X\r\n", checksum);

        if(port_fd < 0) {
            printf("Cant send udp packet: no open serial port\n");
            return;
        }

        bytesSent = write(port_fd, trans_line_buffer, offset);
        if(bytesSent != offset) {
            printf("Error sending udp radio packet\n");
        }
                    printf("Sent %d bytes\n", bytesSent);
    }
    else {
        //split packet into fragments
        int num_fragments = (packet_len + MAX_FRAGMENT_LENGTH - 1) / MAX_FRAGMENT_LENGTH;
        int frag_len = 0;

        buf_offset = 0;
        //printf("NUM FRAGMENTS: %d\n",num_fragments);
        for(i=1;i<=num_fragments;i++) {
            offset = sprintf(trans_line_buffer,"%s",PACKET_IDENT);
            offset += sprintf(&trans_line_buffer[offset],"%c%c%c%c%c",
                    (char)packet_id,(char)DEFAULT_TTL,(char)i,(char)num_fragments,(char)0);

            //offset += snprintf(&trans_line_buffer[offset],MAX_FRAGMENT_LENGTH,
            //      "%s",server_trans_buf + buf_offset);
            if(i == num_fragments) {
                frag_len = packet_len - (MAX_FRAGMENT_LENGTH*(num_fragments - 1));
            }
            else
                frag_len = MAX_FRAGMENT_LENGTH;
            memcpy(&trans_line_buffer[offset],server_trans_buf + buf_offset,frag_len);
            offset+=frag_len;

            checksum = 0;
            for (n = 0; n < offset; n++) checksum ^= trans_line_buffer[n];
            offset += snprintf(&trans_line_buffer[offset],5,"%02X\r\n", checksum);

            if(port_fd < 0) {
                printf("Cant send udp packet: no open serial port\n");
                return;
            }

            bytesSent = write(port_fd, trans_line_buffer, offset);
            if(bytesSent != offset) {
                printf("Error sending udp radio packet\n");
            }
              printf("Sent %d bytes\n", bytesSent);
            //        for(j=0;j<offset;j++)
            //          printf("%c",trans_line_buffer[j]); 
            usleep(15000);
            buf_offset += MAX_FRAGMENT_LENGTH;
        }
    }
}

int udp_receive_line(){
    struct drk_udp_packet temp_packet;
    int bytesRead;
    int packet_length;
    int i, offset = PACKET_IDENT_LEN;
    int payload_len;

    bytesRead = read(port_fd, packet, SENSOR_LINE_LENGTH-1);
    if(bytesRead < 0)
        //printf("%s\n",strerror(errno));
        return -1;
    if(bytesRead == 0 || strncmp(packet, "$RAD", 4)) {
        return -1;
    }
    printf("Received %d bytes\n",bytesRead);
    packet_length = bytesRead;
    temp_packet.packet_id = packet[offset++];
    temp_packet.ttl = packet[offset++];
    temp_packet.seq_num = packet[offset++];
    temp_packet.seq_total = packet[offset++];
    temp_packet.rssi = packet[offset++];

    temp_packet.payload_len = packet_length - PACKET_IDENT_LEN - 5 - 4;
    if(temp_packet.payload_len < 0)
        return 0;
    for (i = 0; i < temp_packet.payload_len; i++) {
        temp_packet.data[i] = packet[offset++];
    }
//    printf("Packet [%d/%d] rec_seq:%d\n", temp_packet.seq_num,temp_packet.seq_total, rec_seq);
    //not in a sequence
    if(rec_seq == 0) {
        if(temp_packet.seq_num != 1) { //received a packet out of sequence
            return 0;
        }

        if(temp_packet.seq_num == 1 && temp_packet.seq_total == 1
                && temp_packet.packet_id == curr_id) {
            return 0;
        }
        
        //received unfragmented packet
        if(temp_packet.seq_num == 1 && temp_packet.seq_total == 1) {
            bzero(server_rec_buf,MAX_PACKET_LENGTH);
            memcpy(server_rec_buf,temp_packet.data,temp_packet.payload_len);
            curr_id = temp_packet.packet_id;
            return temp_packet.payload_len;
        }
        
        //recieved a new fragmented packet
        if(temp_packet.seq_num == 1 && temp_packet.seq_total > 1) {
      //      printf("0received fragmented packet %d\n", temp_packet.seq_num);
            bzero(server_rec_buf,MAX_PACKET_LENGTH);
            memcpy(server_rec_buf,temp_packet.data,temp_packet.payload_len);
            if(temp_packet.payload_len != MAX_FRAGMENT_LENGTH)
                printf("MATH ERROR\n");
            rec_seq++;
            curr_id=temp_packet.packet_id;
            return 0;
        }
    }
    else {
        //received duplicate packet
        if(temp_packet.seq_num == rec_seq && curr_id == temp_packet.packet_id) {
        //    printf("1received fragmented packet %d\n", temp_packet.seq_num);
            return 0;
        }
        //received next packet in sequence
        else if(temp_packet.seq_num == rec_seq + 1 && curr_id == temp_packet.packet_id) {
          //  printf("received fragmented packet %d\n", temp_packet.seq_num);
            bzero(server_rec_buf,MAX_PACKET_LENGTH);
            memcpy(&server_rec_buf[MAX_FRAGMENT_LENGTH*rec_seq], temp_packet.data,
                    temp_packet.payload_len);
            if(temp_packet.seq_total == rec_seq + 1) { //done with sequence
                rec_seq=0;
                return MAX_FRAGMENT_LENGTH*rec_seq + temp_packet.payload_len;
            }
            else {
                rec_seq++;
                return 0;
            }
        }
        //received a new unfragmented packet
        else if (temp_packet.seq_num == 1 && temp_packet.seq_total == 1) {
            bzero(server_rec_buf,MAX_PACKET_LENGTH);
            memcpy(server_rec_buf,temp_packet.data,temp_packet.payload_len);
            rec_seq=0;
            return temp_packet.payload_len;
        
        }

        //received a new fragmented packet
        else if (temp_packet.seq_num == 1 && temp_packet.seq_total > 1) {
            bzero(server_rec_buf,MAX_PACKET_LENGTH);
            memcpy(server_rec_buf,temp_packet.data,temp_packet.payload_len);
            if(temp_packet.payload_len != MAX_FRAGMENT_LENGTH)
                printf("MATH ERROR\n");
            rec_seq=1;
            curr_id=temp_packet.packet_id;
            return 0;
        }
    }
    return 0;
}
