#ifndef _EXPANSION_H
#define _EXPANSION_H

#include <math.h>
#include <stdint.h>
#include <include.h>

#define DEBUG 0

#define ADC_STARTUP_DELAY  1000
#define ADC_SETUP_DELAY  200

uint32_t h_cnt;
uint8_t channel;
uint8_t is_open;

#define MAX_PAYLOAD 64
#define MAC_SIZE 4
#define TIME_STAMP_SIZE 4

typedef struct
{
// Common Header
	uint8_t protocol_id;
	uint8_t priority;
	uint8_t src_mac[MAC_SIZE];
	uint8_t dest_mac[MAC_SIZE];  
	uint8_t seq_num;
	uint8_t ttl;
	uint8_t time_secs[TIME_STAMP_SIZE];
	uint8_t time_nsecs[TIME_STAMP_SIZE];
	uint8_t last_hop[MAC_SIZE];
	uint8_t next_hop[MAC_SIZE];
	uint8_t payload_len;
	uint8_t payload[MAX_PAYLOAD];
	uint8_t checksum;
} drk_packet;


#endif
