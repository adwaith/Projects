#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <slipstream.h>
#include <pkt.h>
#include <time.h>


#define NONBLOCKING  0
#define BLOCKING     1


int gpio_actuate(uint32_t mac, uint8_t socket, uint8_t state);

int verbose;

int main (int argc, char *argv[])
{
  char buffer[128];
  int status,v, cnt, i, size, socket, state;
  uint32_t target_mac;
  PKT_T my_pkt;
  time_t ts,reply_timeout;
  uint8_t rx_buf[MAX_BUF];

  if (argc < 5) {
    printf ("Usage: server port mac-addr pin_mask [-v]\n");
    printf ("  mac-addr:  mac address in hex\n");
    printf ("  pin_mask:  each of the lower 4 bits sets a gpio state\n");
    printf ("  Example:   ./gpio-ctrl localhost 5000 0x1 0x1\n");
    printf ("             Turns port 0 on and port 1-3 off for mac 0x1\n");
    exit (1);
  }

  state = 0;
  verbose=0;
	if(argc==6)
	{
		if(strcmp(argv[5],"-v")==0) { printf( "Verbose on\n" ); verbose=1; }
	}

	//target_mac=atoi(argv[3]);
	sscanf( argv[3],"%x",&target_mac );
	sscanf( argv[4],"%x",&state);

  v = slipstream_open (argv[1], atoi (argv[2]), BLOCKING);


  socket=0;
  if(verbose) printf ("Sending an actuate command [mac=0x%x pin_mask 0x%x]\n",target_mac,state);
  // Enable outlet 0 on node 0x01
  gpio_actuate (target_mac, socket, state);

  status=0;
  reply_timeout=time(NULL)+2;
    while (reply_timeout > time (NULL)) {
      v = slipstream_receive (rx_buf);
      if (v > 0) {
        v=buf_to_pkt(&rx_buf, &my_pkt);
        if(my_pkt.type==ACK)
        {
                printf( "ACK\n" );
                return 1;
        }
        }
      usleep (1000);
    }

return status;



}



int gpio_actuate (uint32_t mac, uint8_t mode, uint8_t mask)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];

  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.type = APP;
  my_pkt.rssi = 0;
  my_pkt.payload[0] = 1;        // Number of Elements
  my_pkt.payload[1] = PKT_GPIO_CTRL;        // KEY (2->actuate, 1-> Power Packet) 
  my_pkt.payload[2] = mode;   // Outlet number 0/1
  my_pkt.payload[3] = mask;    // Outlet state 0/1
  my_pkt.payload_len = 4;
  size = pkt_to_buf (&my_pkt, buffer);
  if(size>0 )
  	v = slipstream_send (buffer, size);
  else printf( "Malformed format...\n" );
  return v;
}

int send_ping (void)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];
  // Send a ping at startup
  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = 0xffffffff;
  my_pkt.type = PING;
  my_pkt.payload_len = 0;
  size = pkt_to_buf (&my_pkt, buffer);
  v = slipstream_send (buffer, size);
  if (v == 0)
    printf ("Error sending\n");
  return v;
}


