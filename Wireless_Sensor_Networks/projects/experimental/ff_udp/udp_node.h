#ifndef DRK_UDP_H
#define DRK_UDP_H

#include <stdlib.h>
#include <stdio.h>

#define UDP_SERVER_PORT         (32000)
#define UDP_SERVER_ADDR         ("127.0.0.10")
#define MAX_PACKET_LENGTH       (1024)
#define MAX_FRAGMENT_LENGTH     (80)
#define PACKET_IDENT            ("$RAD:")
#define PACKET_IDENT_LEN        (5)
#define DEFAULT_TTL             (5)
#define EXTRA_BYTES             (10) //5 for tail, 5 for packet headers
#define SENSOR_LINE_LENGTH      (160)

struct drk_udp_packet
{
    uint8_t packet_id;
    uint8_t ttl;
    uint8_t seq_num;
    uint8_t seq_total;
    uint8_t rssi;
    uint8_t payload_len;
    char data[MAX_FRAGMENT_LENGTH];
    struct timespec receive_time;
};


void udp_client();
void udp_server();

char server_trans_buf[MAX_PACKET_LENGTH];
char trans_line_buffer[MAX_FRAGMENT_LENGTH + PACKET_IDENT_LEN + EXTRA_BYTES];
char packet[MAX_PACKET_LENGTH];

char server_rec_buf[MAX_PACKET_LENGTH];
int rec_seq;
int curr_id;

/* Transmits the last received udp packet by the udp_server 
 * BROADCAST_TRIES times over serial to the firefly.
 * If packet length > packet fragment size then send
 * fragmented packets over serial */

/*PACKET STRUCTURE:
  $RAD_PACKET:[PACKET ID][TTL][SEQ_NUM][SEQ_TOTAL][RSSI][DATA][CHECKSUM][\r][\n]
 */
void udp_transmit_line(int packet_len, uint8_t packet_id);

/* Returns length of packet contained in 'server_rec_buf'
   to be sent back to user over udp. 
   Returns 0 if no packets available or -1 on failure */
int udp_receive_line();


#endif
