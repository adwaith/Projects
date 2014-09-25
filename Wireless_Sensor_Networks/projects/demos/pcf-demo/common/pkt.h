#ifndef _PKT_H_
#define _PKT_H_

#include <stdint.h>


#define PKT_POWER_DATA		1
#define PKT_PLUG_CTRL		2
#define PKT_FF_ENV_DATA		3
#define PKT_GPIO_READ		4
#define PKT_SYNTONISTOR		5
#define PKT_RADIATION		6
#define PKT_AIR_PARTICLE_CNT	7
#define PKT_3_PHASE_METER	8
#define PKT_GPIO_CTRL		9


#define PKT_TYPE_INDEX		0
#define PKT_RSSI_INDEX		1
#define PKT_SRC_MAC_0_INDEX	2
#define PKT_SRC_MAC_1_INDEX	3
#define PKT_SRC_MAC_2_INDEX	4
#define PKT_SRC_MAC_3_INDEX	5
#define PKT_DST_MAC_0_INDEX	6
#define PKT_DST_MAC_1_INDEX	7
#define PKT_DST_MAC_2_INDEX	8
#define PKT_DST_MAC_3_INDEX	9
#define PKT_PAYLOAD_LEN_INDEX	10	
#define PKT_PAYLOAD_START_INDEX 11

#define PAYLOAD_TYPE_INDEX 1
#define PAYLOAD_DATA_START_INDEX 2



// Packet Types
#define UNUSED		0
#define PING		1
#define APP		2
#define NW_CONFIG	3
#define ACK		4


// NW_CONFIG_TYPES
#define NW_CONFIG_SLEEP		1

// PING TYPES
#define PING_NONE	0
#define PING_1		1
#define PING_2		2
#define PING_PERSIST	3

#define MAX_PAYLOAD	100
#define MIN_PAYLOAD	3


typedef struct {
  uint8_t type;
  uint8_t rssi;
  uint32_t src_mac;
  uint32_t dst_mac;
  uint8_t payload_len;
  uint8_t payload[MAX_PAYLOAD];
  uint8_t checksum;
} PKT_T;

int pkt_to_buf (PKT_T * pkt, uint8_t * buf);
int buf_to_pkt (uint8_t * buf, PKT_T * pkt);




#endif
