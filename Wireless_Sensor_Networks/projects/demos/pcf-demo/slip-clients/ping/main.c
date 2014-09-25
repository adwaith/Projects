#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <slipstream.h>
#include <pkt.h>
#include <time.h>


#define NONBLOCKING  0
#define BLOCKING     1


int send_ping (uint32_t mac,int mode);

int verbose;

int main (int argc, char *argv[])
{
  char buffer[MAX_BUF];
  uint8_t rx_buf[MAX_BUF];
  int v, cnt, i, size, socket, state,status;
  uint32_t target_mac;
  PKT_T my_pkt;
  time_t reply_timeout;

  if (argc < 4) {
    printf ("Usage: server port mac-addr [color] [-v]\n");
    printf ("  mac-addr:  mac address in hex (0xff is broadcast)\n");
    printf ("  color:     0 for none, 1 for green, 2 for red, 3 green persist (default red)\n");
    printf ("  Example:   ./ping localhost 5000 0xff 1\n");
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
	state=2;
	if(argc==5) state=atoi(argv[4]);

  v = slipstream_open (argv[1], atoi (argv[2]), BLOCKING);

  //v = send_ping ();

  if(verbose) printf ("Sending ping command [mac=0x%x socket %d, led %d]\n",target_mac,socket,state);
  send_ping (target_mac,state);

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




int send_ping (uint32_t mac, int mode)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];
  // Send a ping at startup
  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.type = PING;
  my_pkt.payload[0] = mode;        
  my_pkt.payload_len = 1;
  size = pkt_to_buf (&my_pkt, buffer);
  v = slipstream_acked_send (buffer, size,3);
  if (v == 0)
    printf ("Error sending\n");
  return v;
}


