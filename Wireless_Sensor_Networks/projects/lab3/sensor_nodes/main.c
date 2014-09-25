/******************************************************************************
*  Nano-RK, a real-time operating system for sensor networks.
*  Copyright (C) 2007, Real-Time and Multimedia Lab, Carnegie Mellon University
*  All rights reserved.
*
*  This is the Open Source Version of Nano-RK included as part of a Dual
*  Licensing Model. If you are unsure which license to use please refer to:
*  http://www.nanork.org/nano-RK/wiki/Licensing
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 2.0 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*******************************************************************************/

#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>

#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>




#define MAC_ADDR 0x0004
#define NUM_NODES 10
#define NEIGHBOR_TTL 2
#define DATA_PERIOD 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type DISCOVER_TASK;
NRK_STK discover_task_stack[NRK_APP_STACKSIZE];
void discover_task (void);

void nrk_create_taskset ();

nrk_sem_t *tx_sem;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

nrk_sem_t *nb_sem;
uint8_t neighbour_list[NUM_NODES] = {0};
uint16_t flood_seq_num = 1;
int16_t flood_seq_nums[NUM_NODES] = {0};

uint16_t sampling_time = 30; // one sample every 30 seconds
uint16_t update_time = 10; // update once every 30 seconds
uint8_t route = 1; // stores the mac address of the node to send data to
uint8_t routing_received = 0; 

uint8_t data_log = 0; 
uint8_t discover_log = 0; 
uint8_t routing_log = 1; 
uint8_t fd; 

void ack_from_neighbour(uint16_t);
void tx_neighbour_list();
int16_t get_next_int(char*,uint8_t*,uint8_t);
void recv_flood_msg();
void data_task();



int main ()
{
  uint16_t div;
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  memset(neighbour_list,0,NUM_NODES);	
  memset(flood_seq_nums,0,NUM_NODES);	
  
  nrk_time_set (0, 0);

  
  bmac_task_config ();

  // Open ADC device as read 
  fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
  if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
  nrk_kprintf(PSTR("Sensor opened\r\n"));

  nrk_create_taskset ();
  tx_sem = nrk_sem_create(1,2);
  nb_sem = nrk_sem_create(1,2);
  if(tx_sem==NULL || nb_sem == NULL ) nrk_kprintf( PSTR("Error creating sem\r\n" ));
  nrk_start ();

  return 0;
}

void rx_task ()
{
  uint8_t i, len,pos;
  int8_t rssi, val;
  uint8_t *local_rx_buf;
	int16_t dest,sender;
  uint16_t sample_value, data_sample_number; 
  nrk_time_t check_period;
  printf ("rx_task PID=%d\r\n", nrk_get_pid ());

  // init bmac on channel 15 
  bmac_init (15);

  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  while (1) {
    // Wait until an RX packet is received
    val = bmac_wait_until_rx_pkt ();
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
//    printf ("Got RX packet len=%d RSSI=%d [", len, rssi);
//    for (i = 0; i < len; i++)
//      printf ("%c", rx_buf[i]);
//    printf ("]\r\n");

		pos = 0; 
		dest = get_next_int(local_rx_buf,&pos,len); 
		pos+=1;
		sender = get_next_int(local_rx_buf,&pos,len);
		pos+=1; 

		if(sender == MAC_ADDR)
		{
			nrk_kprintf(PSTR("Self Message Ignore\r\n"));
    			nrk_led_clr (ORANGE_LED);
    			// Release the RX buffer so future packets can arrive 
			memset(rx_buf,0,RF_MAX_PAYLOAD_SIZE);
    			bmac_rx_pkt_release ();
			continue;
		}

		// received discover message
		if(dest == 0)
		{
			ack_from_neighbour(sender);
			// mutex lock
			nrk_sem_pend(tx_sem);
			sprintf(tx_buf,"%d:%d:AD",sender,MAC_ADDR);
			bmac_tx_pkt(tx_buf, strlen(tx_buf));
			nrk_sem_post(tx_sem);
			// mutex unlock
			memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
			if(discover_log) nrk_kprintf(PSTR("Sent a discover ACK\r\n"));
		}
		else if(dest == MAC_ADDR)
		{
			switch(local_rx_buf[pos]) 
			{
				case 'A':
					if(discover_log) nrk_kprintf(PSTR("ACK from Neighbour\r\n")); // AD ack for discover
					ack_from_neighbour(sender);
					break;
				case 'D': // data msg from peer to node
					if(data_log) nrk_kprintf(PSTR("Data msg forward\r\n"));
					if(route == 0)
					{
						if(data_log) nrk_kprintf(PSTR("Route not established\r\n"));
					}
					pos+=3;
					data_sample_number = get_next_int(local_rx_buf,&pos,len);
					pos++;
					sample_value = get_next_int(local_rx_buf,&pos,len);
					nrk_sem_pend(tx_sem);
					sprintf(tx_buf,"%d:%d:DA:%d:%d",route,sender,data_sample_number,sample_value);
					bmac_tx_pkt(tx_buf,strlen(tx_buf));
					if(data_log) printf("%s\r\n",tx_buf); 
					memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);			
					nrk_sem_post(tx_sem); 

					break; 
				case 'U':
								nrk_kprintf(PSTR("Update ACK not handled\r\n")); 
								break;    
				case 'S':
								nrk_kprintf(PSTR("Sampling ACK not handled\r\n")); 
								break;   
				default:
								nrk_kprintf(PSTR("Invalid Packet\r\n")); 
								break;
			}

		} 
		else if(dest == -1)
		{
			recv_flood_msg(local_rx_buf,len);
		}
			
		 
    nrk_led_clr (ORANGE_LED);
    // Release the RX buffer so future packets can arrive 
	memset(rx_buf,0,RF_MAX_PAYLOAD_SIZE);
    bmac_rx_pkt_release ();

  }

}

void discover_task() {
	uint8_t discover_wakeups = 10; 
	uint8_t i; 

  while (!bmac_started ())
    nrk_wait_until_next_period ();

	while(1) 
	{
		discover_wakeups++;
		if( discover_wakeups < update_time && discover_wakeups > 3)
		{
			nrk_led_clr(BLUE_LED);
		}
		else if(discover_wakeups  <= 3)
		{
			if(discover_wakeups == 1)
			{
				nrk_led_set(BLUE_LED);
				nrk_sem_pend(nb_sem);
				for(i=0;i<NUM_NODES;i++)
				{
					if(neighbour_list[i]>0)
						neighbour_list[i]--; 
				}
				nrk_sem_post(nb_sem);
			}
			nrk_sem_pend(tx_sem);
			sprintf(tx_buf,"0:%d:DM",MAC_ADDR);
			bmac_tx_pkt(tx_buf,strlen(tx_buf));
			memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
			nrk_sem_post(tx_sem);
			if(discover_log) nrk_kprintf(PSTR("TX discover\r\n"));		
		}
		else if( discover_wakeups >= update_time - 1)
		{
			discover_wakeups = 0; 
		}
			nrk_wait_until_next_period();
	}

}
void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  RX_TASK.Ptos = (void *) &rx_task_stack[NRK_APP_STACKSIZE - 1];
  RX_TASK.Pbos = (void *) &rx_task_stack[0];
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 0;
  RX_TASK.period.nano_secs = 200 * NANOS_PER_MS;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  DISCOVER_TASK.task = discover_task;
  DISCOVER_TASK.Ptos = (void *) &discover_task_stack[NRK_APP_STACKSIZE - 1];
  DISCOVER_TASK.Pbos = (void *) &discover_task_stack[0];
  DISCOVER_TASK.prio = 1;
  DISCOVER_TASK.FirstActivation = TRUE;
  DISCOVER_TASK.Type = BASIC_TASK;
  DISCOVER_TASK.SchType = PREEMPTIVE;
  DISCOVER_TASK.period.secs = 1;
  DISCOVER_TASK.period.nano_secs = 0;
  DISCOVER_TASK.cpu_reserve.secs = 1;
  DISCOVER_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  DISCOVER_TASK.offset.secs = 0;
  DISCOVER_TASK.offset.nano_secs = 0;
  nrk_activate_task (&DISCOVER_TASK);



  printf ("Create done\r\n");
}

void ack_from_neighbour(uint16_t neighbour) {
	uint8_t i =0;
	if(neighbour >= NUM_NODES)
	{
		printf("invalid neighbour number\r\n");
		return; 
	}
	nrk_sem_pend(nb_sem);	
	neighbour_list[neighbour] = NEIGHBOR_TTL;
	nrk_sem_post(nb_sem);	
}

int16_t get_next_int(char*rx_buf, uint8_t* pos,uint8_t len) {

	if(rx_buf[*pos] == '-')
	{
		*pos = *pos + 2;
		return -1;
	}
		
	int16_t sum = 0;
	while(*pos < len && rx_buf[*pos] != '\0' && rx_buf[*pos] >='0' && rx_buf[*pos]<='9') 
	{
		sum *= 10;
		sum += (rx_buf[*pos]-'0');
		*pos = *pos+1;
	}
	return sum;	
}

void tx_neighbour_list() {

	nrk_sem_pend(tx_sem);
	sprintf(tx_buf, "-1:%d:%d:NL:",MAC_ADDR,flood_seq_num);
	flood_seq_num++;
	uint8_t i=0;
	uint8_t neighbour = 0; 
	for(i=0;i<NUM_NODES;i++)
	{
		
		nrk_sem_pend(nb_sem);	
		if(neighbour_list[i] > 0)
		{
			neighbour = 1; 
			nrk_sem_post(nb_sem);	
			sprintf(tx_buf+strlen(tx_buf),"%d,",i);
		}
		else
			nrk_sem_post(nb_sem);	
	}
	if(neighbour==0)
	{
		nrk_sem_post(tx_sem);
		return;
	}
	
	bmac_tx_pkt(tx_buf,strlen(tx_buf));
	if(routing_log) printf("Tx Neighbor list: %s\r\n",tx_buf);
	memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
	nrk_sem_post(tx_sem);
}

void recv_flood_msg(char* rx_buf, int16_t len) {

	uint16_t pos = 3,sender,seq_num,mac_addr,light;
	int16_t sample_num;
	sender = get_next_int(rx_buf,&pos,len); 
	pos++;
	seq_num = get_next_int(rx_buf,&pos,len); 
	pos++;

	if(sender >= NUM_NODES)
	{
		printf("Invalid sender Number\n");
		return;
	}

	// the flood index in msg is lower ignore msg.
	if(flood_seq_nums[sender] >= seq_num)
	{
		printf("the flood index in msg is lower ignore msg\r\n");
		return;
	}
	// update the flood index from new seq number
	flood_seq_nums[sender] = seq_num;
	if(pos >= len)
	{
		printf("pos >= len\n");
		return;
	}

	switch(rx_buf[pos])
	{
		case 'N':
			if(routing_log) nrk_kprintf(PSTR("Neighbor list flooding: "));
			//mutex lock
			nrk_sem_pend(tx_sem);
			sprintf(tx_buf,"%s",rx_buf);
			bmac_tx_pkt(tx_buf,strlen(tx_buf));
			if(routing_log) printf("%s\r\n",rx_buf);
			nrk_sem_post(tx_sem);
			// mutex unlock
			memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
			break;
		case 'F':
			pos+=3; 
			sample_num = get_next_int(rx_buf,&pos,len);
			if(data_log) printf("Data req for no. %d\r\n",sample_num);
			if(route == 0)
			{
				if(data_log) nrk_kprintf(PSTR("ROUTING NOT RECEIVED\r\n"));
			}
			else {
				
				nrk_set_status(fd,SENSOR_SELECT,LIGHT);
				nrk_read(fd,&light,2);
			
				nrk_sem_pend(tx_sem);
				sprintf(tx_buf,"%d:%d:DA:%d:%d",route,MAC_ADDR,sample_num,light);
				bmac_tx_pkt(tx_buf,strlen(tx_buf));
				if(data_log) printf("Sending data %s\r\n",tx_buf);
				memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
				nrk_sem_post(tx_sem);
			}
			break;
		case 'R':
			pos+=3; 
			if(routing_log) nrk_kprintf(PSTR("Routing table info\n"));
			if(pos >= len)	
			{
				if(routing_log) nrk_kprintf(PSTR("No table attached\r\n"));
				break;
			}
			while(pos < len && get_next_int(rx_buf,&pos,len) != MAC_ADDR)
			{
				pos++; 
				get_next_int(rx_buf,&pos,len);	
				pos++;
			}
			pos++;
			route = get_next_int(rx_buf,&pos,len);
			if(route == 0)
				if(routing_log) nrk_kprintf(PSTR("Routing not received\n"));
			else
			{
				if(routing_log) printf("Route is %d",route);
				routing_received = 1; 
			}
			break;
		case 'L':
			// tx nrighbor list	 
			tx_neighbour_list();
			break;
		default:
			printf("invalid message");
			break;
	}	
	

}
