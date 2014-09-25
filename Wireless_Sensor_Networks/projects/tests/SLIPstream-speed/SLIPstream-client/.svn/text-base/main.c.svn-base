#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <slipstream.h>

#define NONBLOCKING  0
#define BLOCKING     1


int main (int argc, char *argv[])
{
  char buffer[128];
  int v,cnt,i;

  if (argc != 3) {
    printf ("Usage: server port\n");
    exit (1);
  }

  v=slipstream_open(argv[1],atoi(argv[2]),NONBLOCKING);
  
  cnt = 0;
  while (1) {
    cnt++;
    //sprintf (buffer, "This is a sample slip string: Count %d\n", cnt);

    for(i=0; i<100; i++ )
	buffer[i]=i;

    buffer[0]=cnt;
    printf( "tx %d\n",cnt );
    v=slipstream_send(buffer,100);

    //v=slipstream_send(buffer,strlen(buffer)+1);
    if (v == 0) printf( "Error sending\n" );
    //printf( "sent %d bytes\n",strlen(buffer)+1 );

/*
    v=slipstream_receive( buffer );
    if (v > 0) {
      printf ("Got: ");
      for (i = 0; i < v; i++)
        printf ("%c", buffer[i]);
      printf ("\n");
    }
*/
// Pause for 1 second 
    v=usleep (30000);
	if(v==-1) printf( "usleep failed\n" );
    //sleep (1);
  }



}

