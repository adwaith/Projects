#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <slipstream.h>
#include <pkt.h>
#include <time.h>


#define NONBLOCKING  0
#define BLOCKING     1
#define POWER_SCALER	111.0
#define VOLTAGE_SCALER 	3.11
#define CURRENT_SCALER	36.0

typedef struct ff_power_pkt {
  uint32_t total_secs;
  uint8_t freq;

  uint16_t rms_voltage;
  uint16_t rms_current;
  uint32_t true_power;
  uint8_t energy[6];
  uint64_t long_energy;
  uint8_t socket_state;

  uint16_t rms_voltage1;
  uint16_t rms_current1;
  uint32_t true_power1;
  uint8_t energy1[6];
  uint64_t long_energy1;
  uint8_t socket_state1;
  uint16_t v_p2p_low;
  uint16_t v_p2p_high;
  uint16_t c_p2p_low;
  uint16_t c_p2p_high;
  uint16_t c_p2p_low2;
  uint16_t c_p2p_high2;
  uint16_t v_center;
  uint16_t c_center;
  uint16_t c2_center;
} FF_POWER_PKT;

typedef struct ff_env_pkt{
  uint16_t bat;
  uint16_t light;
  uint16_t temp;
  uint16_t acc_x;
  uint16_t acc_y;
  uint16_t acc_z;
  uint16_t audio_p2p;
  uint32_t digital_temp;
  uint32_t humidity;
  uint32_t pressure;
  uint16_t motion;
  uint8_t gpio_state;
} FF_ENV_PKT;

typedef struct ff_three_phase_pkt{

uint32_t total_secs;
uint16_t period;


// Phase A
int32_t awatthr;
int32_t awatt;
int32_t ava;
int32_t avrms;
int32_t airms;

// Phase B
int32_t bwatthr;
int32_t bwatt;
int32_t bva;
int32_t bvrms;
int32_t birms;

// Phase C
int32_t cwatthr;
int32_t cwatt;
int32_t cva;
int32_t cvrms;
int32_t cirms;

} FF_3PHASE_PKT;




uint8_t three_phase_unpack (uint8_t * payload, FF_3PHASE_PKT * p);
uint8_t power_unpack (uint8_t * payload, FF_POWER_PKT * p);
uint8_t env_unpack(uint8_t * payload, FF_ENV_PKT * p);
int outlet_actuate (uint32_t mac, uint8_t socket, uint8_t state);
int send_ping ();

int verbose;

int main (int argc, char *argv[])
{
  char buffer[128];
  int v, cnt, i, size, socket, state;
  uint32_t target_mac;
  FF_POWER_PKT my_pwr_pkt;
  FF_ENV_PKT my_env_pkt;
  FF_3PHASE_PKT my_three_phase_pkt;
  PKT_T my_pkt;
  time_t ts;
		uint32_t small_cnt, large_cnt;
	
  if (argc < 3) {
    printf ("Usage: server port [-v]\n");
    exit (1);
  }

  state = 0;
  verbose=0;
	if(argc==4)
	{
		if(strcmp(argv[3],"-v")==0) { printf( "Verbose on" ); verbose=1; }
	}


  v = slipstream_open (argv[1], atoi (argv[2]), BLOCKING);

  v = send_ping ();

  cnt = 0;
  while (1) {

    v = slipstream_receive (buffer);
    if (v > 0) {
      v = buf_to_pkt (buffer, &my_pkt);
	//printf( "pkt: %d\n",v );
      if (v == 1) {

/*
 * printf ("TYPE: %d\n", my_pkt.type);
        printf ("RSSI: %u\n", my_pkt.rssi);
        printf ("SRC MAC: %d\n", my_pkt.src_mac);
        printf ("DST MAC: %d\n", my_pkt.dst_mac);
        printf ("PAYLOAD LEN: %d\n", my_pkt.payload_len);
        printf ("PAYLOAD: [");
        for (i = 0; i < my_pkt.payload_len; i++)
          printf ("%d ", my_pkt.payload[i]);
        printf ("]\n");
   */ 
 	if(my_pkt.type==APP)
 	{
	ts=time(NULL);
	switch(my_pkt.payload[1])
		{	
		case 1:
       			power_unpack (my_pkt.payload, &my_pwr_pkt);
						if(verbose)
			printf( "(time,mac,rssi,total_secs,freq,v-rms,i-rms,t-pwr,energy,i-rms2,t-pwr2,energy2): %u,%x,%u,%u,",ts,my_pkt.src_mac,my_pkt.rssi,my_pwr_pkt.total_secs );
				else
			printf( "P,%u,%x,%u,%u,",ts,my_pkt.src_mac,my_pkt.rssi,my_pwr_pkt.total_secs );

				printf( "%u,%u,%u,%u,%lu",my_pwr_pkt.freq, my_pwr_pkt.rms_voltage,my_pwr_pkt.rms_current, my_pwr_pkt.true_power,my_pwr_pkt.long_energy);
				printf( ",%u,%u,%lu",my_pwr_pkt.rms_current1, my_pwr_pkt.true_power1,my_pwr_pkt.long_energy1);
				printf( ",%u,%u",my_pwr_pkt.v_p2p_low, my_pwr_pkt.v_p2p_high);
				printf( ",%u,%u",my_pwr_pkt.c_p2p_low, my_pwr_pkt.c_p2p_high);
				printf( ",%u,%u",my_pwr_pkt.c_p2p_low2, my_pwr_pkt.c_p2p_high2);
				printf( ",%u,%u,%u,%u",my_pwr_pkt.v_center, my_pwr_pkt.c_center, my_pwr_pkt.c2_center, my_pwr_pkt.socket_state);
			//        printf ("%d,%d)",
			//                ((float) my_pwr_pkt.true_power) / POWER_SCALER,
			//                my_pwr_pkt.true_power);
        		printf( "\n" );
		break;
		case 3:
			env_unpack(my_pkt.payload, &my_env_pkt );
			if(verbose)
			printf( "(time,mac,rssi,bat,light,temp,acc_x,acc_y,acc_z,audio_p2p,digital-temp,humidity,pressure,motion,gpio_state): %u,%x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",ts,my_pkt.src_mac, my_pkt.rssi,my_env_pkt.bat, my_env_pkt.light, my_env_pkt.temp, my_env_pkt.acc_x, my_env_pkt.acc_y, my_env_pkt.acc_z, my_env_pkt.audio_p2p, my_env_pkt.digital_temp,my_env_pkt.humidity, my_env_pkt.pressure,my_env_pkt.motion,my_env_pkt.gpio_state );
			else
			printf( "S,%u,%x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",ts,my_pkt.src_mac, my_pkt.rssi, my_env_pkt.bat, my_env_pkt.light, my_env_pkt.temp, my_env_pkt.acc_x, my_env_pkt.acc_y, my_env_pkt.acc_z, my_env_pkt.audio_p2p,my_env_pkt.digital_temp,my_env_pkt.humidity, my_env_pkt.pressure, my_env_pkt.motion, my_env_pkt.gpio_state );

			printf( "\n" );	
		break;
		case 4:
			printf("(time,mac,rssi,state): %u,%x,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,my_pkt.payload[2] );
		break;
		case 5:
		if(verbose)
			printf("(time,mac,rssi,gain): %u,%x,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi, my_pkt.payload[2] );
		else
			printf("B,%u,%u,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,my_pkt.payload[2] );
		break;
		case 6:
		// Payload index 2 represents counts per minute
		if(verbose)
			printf("(time,mac,rssi,cpm): %u,%x,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,my_pkt.payload[2] );
		else
			printf("R,%u,%x,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,my_pkt.payload[2] );
		break;
		case 7:
		// Payload index 2,3,4,5 represent a little endian value for small particles counted
		// Payload index 6,7,8,9 represent a little endian value for large particles counted
		small_cnt=my_pkt.payload[2] | my_pkt.payload[3]<<8 | my_pkt.payload[4]<<16 | my_pkt.payload[5]<<24;	
		large_cnt=my_pkt.payload[6] | my_pkt.payload[7]<<8 | my_pkt.payload[8]<<16 | my_pkt.payload[9]<<24;	
		if(verbose)
			printf("(time,mac,rssi,small_cnt,large_cnt): %u,%x,%u,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,small_cnt,large_cnt );
		else
			printf("A,%u,%x,%u,%u,%u\n",ts,my_pkt.src_mac,my_pkt.rssi,small_cnt,large_cnt);
		break;

		case 8:
       			three_phase_unpack (my_pkt.payload, &my_three_phase_pkt);
			if(verbose)
			printf( "3 Phase meter (time,mac,rssi,total_secs,period,awatthr,awatt,ava,avrms,airms,bwatt,bwatthr,bva,bvrms,birms,cwatt,cwatthr,cva,cvrms,cirms): %u,%x,%u,%u,",ts,my_pkt.src_mac,my_pkt.rssi,my_three_phase_pkt.total_secs );
				else
			printf( "M,%u,%x,%u,%u,",ts,my_pkt.src_mac,my_pkt.rssi,my_three_phase_pkt.total_secs );
		
			printf( "%u,%d,%d,%d,%d,%d,",my_three_phase_pkt.period, my_three_phase_pkt.awatthr,my_three_phase_pkt.awatt,my_three_phase_pkt.ava,my_three_phase_pkt.avrms,my_three_phase_pkt.airms );	
			printf( "%d,%d,%d,%d,%d,",my_three_phase_pkt.bwatthr,my_three_phase_pkt.bwatt,my_three_phase_pkt.bva,my_three_phase_pkt.bvrms,my_three_phase_pkt.birms );	
			printf( "%d,%d,%d,%d,%d",my_three_phase_pkt.cwatthr,my_three_phase_pkt.cwatt,my_three_phase_pkt.cva,my_three_phase_pkt.cvrms,my_three_phase_pkt.cirms );	

        		printf( "\n" );
		break;
	

		default:
		if(verbose)
		printf( "unkown pkt type (%d): ",my_pkt.payload[1] );
		else
		printf( "U,%d",my_pkt.payload[1] );

		for(i=2; i<my_pkt.payload_len; i++ )
			printf( ",%d",my_pkt.payload[i] );
			printf( "\n" );	
		}
		


      	}
      }

    }
    else
      printf ("Error reading packet\n");
    // Count to 5 and then send an actuate packet 
  /*  cnt++;
    if (cnt > 5) {
	    target_mac=0x04;
	    socket=0;
	    state=1;
      printf ("Sending an actuate command [mac=0x%x socket %d, state %d]\n",target_mac,socket,state);
      // Enable outlet 0 on node 0x01
      outlet_actuate (target_mac, socket, state);
      cnt = 0;
    }*/
  }



}



int outlet_actuate (uint32_t mac, uint8_t socket, uint8_t state)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];

  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.type = APP;
  my_pkt.payload[0] = 1;        // Number of Elements
  my_pkt.payload[1] = 2;        // KEY (2->actuate, 1-> Power Packet) 
  my_pkt.payload[2] = socket;   // Outlet number 0/1
  my_pkt.payload[3] = state;    // Outlet state 0/1
  my_pkt.payload_len = 4;
  size = pkt_to_buf (&my_pkt, buffer);
  v = slipstream_send (buffer, size);
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


uint8_t env_unpack (uint8_t * payload, FF_ENV_PKT * p)
{
	int i;
p->bat = payload[2]<<8 | payload[3];
p->light = payload[4]<<8 | payload[5];
p->temp = payload[6]<<8 | payload[7];
p->acc_x = payload[8]<<8 | payload[9];
p->acc_y = payload[10]<<8 | payload[11];
p->acc_z = payload[12]<<8 | payload[13];
p->audio_p2p = payload[14]<<8 | payload[15];
p->humidity = payload[16]<<24 | payload[17]<< 16 | payload[18]<<8 | payload[19];
p->digital_temp= payload[20]<<24 | payload[21]<< 16 | payload[22]<<8 | payload[23];
p->pressure= payload[24]<<24 | payload[25]<< 16 | payload[26]<<8 | payload[27];
p->motion = payload[28]<<8 | payload[29];
p->gpio_state = payload[30];
}


uint8_t three_phase_unpack (uint8_t * payload, FF_3PHASE_PKT * p)
{

  p->total_secs =
    ((uint32_t) payload[2] << 24) | ((uint32_t) payload[3] << 16) |
    ((uint32_t) payload[4] << 8) | (uint32_t) payload[5];
  p->period= ((uint16_t)payload[6]<<8) | ((uint16_t)payload[7]);

  p->awatt= ((uint32_t)payload[8]<<24) | ((uint32_t)payload[9]<<16) | ((uint32_t)payload[10]<<8)  | ((uint32_t)payload[11])   ;
  p->ava= ((uint32_t)payload[12]<<24) | ((uint32_t)payload[13]<<16) | ((uint32_t)payload[14]<<8)  | ((uint32_t)payload[15])   ;
  p->avrms = ((uint32_t)payload[16]<<24) | ((uint32_t)payload[17]<<16) | ((uint32_t)payload[18]<<8)  | ((uint32_t)payload[19])   ;
  p->airms = ((uint32_t)payload[20]<<24) | ((uint32_t)payload[21]<<16) | ((uint32_t)payload[22]<<8)  | ((uint32_t)payload[23])   ;
  p->awatthr= ((uint32_t)payload[24]<<24) | ((uint32_t)payload[25]<<16) | ((uint32_t)payload[26]<<8)  | ((uint32_t)payload[27])   ;

  p->bwatt= ((uint32_t)payload[28]<<24) | ((uint32_t)payload[29]<<16) | ((uint32_t)payload[30]<<8)  | ((uint32_t)payload[31])   ;
  p->bva= ((uint32_t)payload[32]<<24) | ((uint32_t)payload[33]<<16) | ((uint32_t)payload[34]<<8)  | ((uint32_t)payload[35])   ;
  p->bvrms = ((uint32_t)payload[36]<<24) | ((uint32_t)payload[37]<<16) | ((uint32_t)payload[38]<<8)  | ((uint32_t)payload[39])   ;
  p->birms = ((uint32_t)payload[40]<<24) | ((uint32_t)payload[41]<<16) | ((uint32_t)payload[42]<<8)  | ((uint32_t)payload[43])   ;
  p->bwatthr= ((uint32_t)payload[44]<<24) | ((uint32_t)payload[45]<<16) | ((uint32_t)payload[46]<<8)  | ((uint32_t)payload[47])   ;

  p->cwatt= ((uint32_t)payload[48]<<24) | ((uint32_t)payload[49]<<16) | ((uint32_t)payload[50]<<8)  | ((uint32_t)payload[51])   ;
  p->cva= ((uint32_t)payload[52]<<24) | ((uint32_t)payload[53]<<16) | ((uint32_t)payload[54]<<8)  | ((uint32_t)payload[55])   ;
  p->cvrms = ((uint32_t)payload[56]<<24) | ((uint32_t)payload[57]<<16) | ((uint32_t)payload[58]<<8)  | ((uint32_t)payload[59])   ;
  p->cirms = ((uint32_t)payload[60]<<24) | ((uint32_t)payload[61]<<16) | ((uint32_t)payload[62]<<8)  | ((uint32_t)payload[63])   ;
  p->cwatthr= ((uint32_t)payload[64]<<24) | ((uint32_t)payload[65]<<16) | ((uint32_t)payload[66]<<8)  | ((uint32_t)payload[67])   ;




}


uint8_t power_unpack (uint8_t * payload, FF_POWER_PKT * p)
{

  p->total_secs =
    ((uint32_t) payload[2] << 24) | ((uint32_t) payload[3] << 16) |
    ((uint32_t) payload[4] << 8) | (uint32_t) payload[5];
  p->freq = payload[6];
  p->rms_voltage = (payload[7] << 8) | payload[8];


  p->rms_current = (payload[9] << 8) | payload[10];
  p->true_power =
    ((uint32_t) payload[11] << 16) | ((uint32_t) payload[12] << 8) |
    (uint32_t) payload[13];
  p->energy[5] = payload[14];
  p->energy[4] = payload[15];
  p->energy[3] = payload[16];
  p->energy[2] = payload[17];
  p->energy[1] = payload[18];
  p->energy[0] = payload[19];
  p->long_energy = (uint64_t)payload[19]<<40 | (uint64_t)payload[18]<<32 | (uint64_t)payload[17]<<24 | payload[16]<<16 | payload[15]<<8 | payload[14];
  p->socket_state = payload[20];

  p->rms_current1 = (payload[21] << 8) | payload[22];
  p->true_power1 =
    ((uint32_t) payload[23] << 16) | ((uint32_t) payload[24] << 8) |
    (uint32_t) payload[25];
  
  p->energy1[5] = payload[26];
  p->energy1[4] = payload[27];
  p->energy1[3] = payload[28];
  p->energy1[2] = payload[29];
  p->energy1[1] = payload[30];
  p->energy1[0] = payload[31];
  p->long_energy1 =(uint64_t)payload[31]<<40 | (uint64_t)payload[30]<<32 | (uint64_t)payload[29]<<24 | payload[28]<<16 | payload[27]<<8 | payload[26];
  p->socket_state1 = payload[32];
  p->v_p2p_low = (uint16_t)payload[33]<<8 | (uint16_t) payload[34];
  p->v_p2p_high = (uint16_t)payload[35]<<8 | (uint16_t) payload[36];
  p->c_p2p_low= (uint16_t)payload[37]<<8 | (uint16_t) payload[38];
  p->c_p2p_high= (uint16_t)payload[39]<<8 | (uint16_t) payload[40];
  p->c_p2p_low2= (uint16_t)payload[41]<<8 | (uint16_t) payload[42];
  p->c_p2p_high2= (uint16_t)payload[43]<<8 | (uint16_t) payload[44];
  p->v_center= (uint16_t)payload[45]<<8 | (uint16_t) payload[46];
  p->c_center= (uint16_t)payload[47]<<8 | (uint16_t) payload[48];
  p->c2_center= (uint16_t)payload[49]<<8 | (uint16_t) payload[50];

}
