#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <slipstream.h>
#include <pkt.h>
#include <time.h>


#define NONBLOCKING  0
#define BLOCKING     1



int node_sleep (uint32_t mac, uint8_t state);

int verbose;

int main (int argc, char *argv[])
{
  char buffer[128];
  int v, cnt, i, size, socket, state;
  uint32_t target_mac;
  PKT_T my_pkt;
  time_t ts;

  if (argc < 5) {
    printf ("Usage: server port mac-addr state [-v]\n");
    printf ("  mac-addr:  mac address in hex (0xff is broadcast)\n");
    printf ("  state:     0 for off, 1 for wakeup, 2 for sleep\n");
    printf ("  Example:   ./node-sleep localhost 5000 0x1 1\n");
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
	state=atoi(argv[4]);

  v = slipstream_open (argv[1], atoi (argv[2]), BLOCKING);

  //v = send_ping ();

  if(verbose) printf ("Sending an sleep command [mac=0x%x socket %d, state %d]\n",target_mac,socket,state);
  // Enable outlet 0 on node 0x01
  node_sleep (target_mac, state);

}



int node_sleep (uint32_t mac, uint8_t state)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];

  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.type = NW_CONFIG;
  my_pkt.payload[0] = 1;        // Number of Elements
  my_pkt.payload[1] = NW_CONFIG_SLEEP;        // KEY (1-> sleep ) 
  my_pkt.payload[2] = state;    // Outlet state 0/1
  my_pkt.payload_len = 3;
  size = pkt_to_buf (&my_pkt, buffer);
  v = slipstream_acked_send(buffer, size,3);
  return v;
}


