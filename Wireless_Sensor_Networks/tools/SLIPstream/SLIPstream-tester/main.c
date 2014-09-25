#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <slipstream.h>

#define NONBLOCKING  0
#define BLOCKING     1

#define PKT_LEN 100

int main (int argc, char *argv[])
{
  uint8_t tx_buffer[128];
  uint8_t rx_buffer[128];
  uint8_t cnt,i;
  int error;
  int v;

  if (argc != 3) {
    printf ("Usage: server port\n");
    exit (1);
  }

  v=slipstream_open(argv[1],atoi(argv[2]),NONBLOCKING);
  
  cnt = 0;
  while (1) {
    cnt++;
    for(i=0; i<PKT_LEN; i++ ) tx_buffer[i]=cnt;
    //v=slipstream_acked_send(buffer,strlen(buffer)+1,3);
    v=slipstream_send(tx_buffer,PKT_LEN);
    if (v == 0) printf( "Error sending\n" );
    else printf( "sent pkt %d\n",cnt );

    //usleep (10000);
    error=0;
    do{ 
	v=slipstream_receive( rx_buffer ); 
	} while(v==-1);
    if (v > 0) {
	printf( "got pkt: %d ",v );
	for(i=0; i<PKT_LEN; i++ )
		if(rx_buffer[i]!=cnt) {
		error=1;
		printf( "Error at index %d where %d should have been %d\n", i,rx_buffer[i],cnt );
		break;
		}
    } else
    {
    error=1;
    printf( "Packet failed, size=%d\n",v );
    }
	 
    if(error==0 ) printf( "Packet round-trip success!\n" );
// Pause for 1 second 
    //usleep (100000);
  }



}

