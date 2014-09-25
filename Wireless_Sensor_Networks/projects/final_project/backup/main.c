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
/*
Team 15 
Parth Mehta 
Adwaith Venkataraman
Amrita Aurora
*/


#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>
#define TIMEOUT 1000
#define NUM_SLAVES	4	
#define MAC_ADDR	0000	

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type DISCOVER_TASK;
NRK_STK discover_task_stack[NRK_APP_STACKSIZE];
void discover_task (void);

NRK_STK uart_task_stack[NRK_APP_STACKSIZE];
nrk_task_type UART_TASK;
void uart_task(void);

void nrk_create_taskset ();
int16_t get_next_int(char*,uint8_t*,uint8_t);
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t discover_log = 1;
void nrk_register_drivers();
uint8_t data_flag = 1;
char message[250];
uint8_t msg_len;
nrk_sig_t uart_rx_signal;
uint8_t task_flag = 1;
uint8_t rx_pid;
uint8_t data_pid;
struct nodes
{
	uint8_t flag, mac, existing_version, new_version;
	char data[RF_MAX_PAYLOAD_SIZE - 6];
}node[5];

int main ()
{
  uint16_t div;
  uint8_t i;
  for (i=0; i<5; i++)
  {
	node[i].mac = -1;
	node[i].existing_version = -1;
	node[i].new_version = -1;
	node[i].flag = 0;
  }
  node[MAC_ADDR].mac = MAC_ADDR;
  node[MAC_ADDR].new_version = 0;
  node[MAC_ADDR].existing_version = 0;
  node[MAC_ADDR].flag = 1;

  msg_len = strlen (message);
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (LED_RED);
  nrk_led_clr (LED_BLUE);
  nrk_led_clr (LED_ORANGE);
  nrk_led_clr (LED_GREEN);

  nrk_time_set (0, 0);

  bmac_task_config ();

  nrk_create_taskset ();
  nrk_start ();

  return 0;
}


void rx_task()
{
	uint8_t i, len, pos, mac, j;
	int8_t rssi, val;
	uint8_t *local_rx_buf;
	int16_t dest, sender, version, msg_type;
	nrk_time_t check_period;
//	printf("rx_task PID=%d\r\n", nrk_get_pid());
	rx_pid = nrk_get_pid();
	bmac_init(15);
	bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);
	while (1)
	{
		val = bmac_wait_until_rx_pkt();
		nrk_led_set (ORANGE_LED);
		local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
//		printf("%s\r\n", local_rx_buf);
		pos = 0;
		msg_type = get_next_int(local_rx_buf, &pos, len);
		pos+=1;
		sender = get_next_int(local_rx_buf, &pos, len);
		pos+=1;
		dest = get_next_int(local_rx_buf, &pos, len);
		pos+=1;
		version = get_next_int(local_rx_buf, &pos, len);
		pos+=1;		
//		printf("Type %d\nSender %d\nDest %d\r\n", msg_type, sender, dest);
		if(sender == MAC_ADDR)
		{
			nrk_kprintf(PSTR("Self Message Ignore\r\n"));
			nrk_led_clr(ORANGE_LED);
			memset(rx_buf, 0, RF_MAX_PAYLOAD_SIZE);
			bmac_rx_pkt_release();
			continue;
		}

		else
		{
			switch(msg_type)
			{
				case 1:
//					nrk_kprintf(PSTR("In case 1\r\n"));
					node[sender].mac = sender;					
					node[sender].new_version = version;
					if (node[sender].existing_version != node[sender].new_version)
					{
						printf("Sending to node %d\r\n", sender);				
						task_flag = 0;
						sprintf(tx_buf, "3:%d:%d:%s",MAC_ADDR, sender, node[MAC_ADDR].data);
						bmac_tx_pkt(tx_buf, strlen(tx_buf));
						nrk_led_clr (ORANGE_LED);
					}
					task_flag = 1;
//					printf("Node %d\r\n",sender);
					break;
				case 3:

					if (dest == MAC_ADDR & node[sender].flag != 1)
					{
						task_flag = 0;
//						nrk_kprintf(PSTR("In case 3\r\n"));
						node[sender].existing_version = node[sender].new_version;					
//						printf("Receiving from node %d\r\n", sender);						
						for (i=6; i<RF_MAX_PAYLOAD_SIZE; i++)
							node[sender].data[i-6] = local_rx_buf[i];
						printf("\nData from node %d is %s\r\n", sender, node[sender].data);
						node[sender].existing_version = node[sender].new_version;
						sprintf(tx_buf, "3:%d:%d:%s",MAC_ADDR, sender, node[MAC_ADDR].data);
						bmac_tx_pkt(tx_buf, strlen(tx_buf));
			
//						node[sender].flag = 1;
					}
					task_flag = 1;					
					break;
				default:
					nrk_kprintf(PSTR("Invalid Packet\r\n"));
					break;
			}
		}
		
		nrk_led_clr (ORANGE_LED);
		memset(rx_buf, 0, RF_MAX_PAYLOAD_SIZE);
		bmac_rx_pkt_release();
	}
}

void discover_task() {

	data_pid = nrk_get_pid();
	while (!bmac_started ())
		nrk_wait_until_next_period ();

	while(task_flag) 
	{
			sprintf(tx_buf,"1:%d:-1:%d",MAC_ADDR, node[MAC_ADDR].new_version);
			bmac_tx_pkt(tx_buf,strlen(tx_buf));
			memset(tx_buf,0,RF_MAX_PAYLOAD_SIZE);
			if(discover_log) nrk_kprintf(PSTR("TX discover\r\n"));		
			nrk_wait_until_next_period();
	}

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

void uart_task()
{
char c;
uint8_t j=0;
nrk_sig_t uart_rx_signal;
nrk_sig_mask_t sm;

//  printf( "My node's address is %d\r\n",NODE_ADDR );
//  printf( "rx_task PID=%d\r\n",nrk_get_pid());

  // Get the signal for UART RX  
  uart_rx_signal=nrk_uart_rx_signal_get();
  // Register your task to wakeup on RX Data 
  if(uart_rx_signal==NRK_ERROR) nrk_kprintf( PSTR("Get Signal ERROR!\r\n") );
  nrk_signal_register(uart_rx_signal);
  task_flag = 0;
  while(1) {

	// Wait for UART signal
	while(nrk_uart_data_ready(NRK_DEFAULT_UART)!=0)
        {
//		nrk_rem_from_readyQ(rx_pid);
//		nrk_rem_from_readyQ(data_pid);
		// Read Character
		node[MAC_ADDR].data[j] = getchar();
//		c=getchar();
//		printf( "%c",node[MAC_ADDR].data[j]);
//		if(c=='x') nrk_led_set(GREEN_LED);
//		else nrk_led_clr(GREEN_LED);
		j++;
		if (node[MAC_ADDR].data[j-1] == 'g')
		{
			break;
			node[MAC_ADDR].new_version++;
		}
	}
	printf("Data packet: %s\r\n", node[MAC_ADDR].data);
	sm=nrk_event_wait(SIG(uart_rx_signal));
	if(sm != SIG(uart_rx_signal))
	nrk_kprintf( PSTR("RX signal error") );
	nrk_kprintf( PSTR("\r\ngot uart data: ") );
//	nrk_add_to_readyQ(rx_pid);
//	nrk_add_to_readyQ(data_pid);
	task_flag = 1;

	}
}

void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  RX_TASK.Ptos = (void *) &rx_task_stack[NRK_APP_STACKSIZE - 1];
  RX_TASK.Pbos = (void *) &rx_task_stack[0];
  RX_TASK.prio = 1;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 1;
  RX_TASK.period.nano_secs = 0 * NANOS_PER_MS;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 50 * NANOS_PER_MS;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  DISCOVER_TASK.task = discover_task;
  DISCOVER_TASK.Ptos = (void *) &discover_task_stack[NRK_APP_STACKSIZE - 1];
  DISCOVER_TASK.Pbos = (void *) &discover_task_stack[0];
  DISCOVER_TASK.prio = 2;
  DISCOVER_TASK.FirstActivation = TRUE;
  DISCOVER_TASK.Type = BASIC_TASK;
  DISCOVER_TASK.SchType = PREEMPTIVE;
  DISCOVER_TASK.period.secs = 6;
  DISCOVER_TASK.period.nano_secs = 0 * NANOS_PER_MS;
  DISCOVER_TASK.cpu_reserve.secs = 0;
  DISCOVER_TASK.cpu_reserve.nano_secs = 250 * NANOS_PER_MS;
  DISCOVER_TASK.offset.secs = 0;
  DISCOVER_TASK.offset.nano_secs = 0;
  nrk_activate_task (&DISCOVER_TASK);

  UART_TASK.task = uart_task;
  nrk_task_set_stk( &uart_task, uart_task_stack, NRK_APP_STACKSIZE);
  UART_TASK.prio = 1;
  UART_TASK.FirstActivation = TRUE;
  UART_TASK.Type = BASIC_TASK;
  UART_TASK.SchType = PREEMPTIVE;
  UART_TASK.period.secs = 3;
  UART_TASK.period.nano_secs = 100*NANOS_PER_MS;
  UART_TASK.cpu_reserve.secs = 1;
  UART_TASK.cpu_reserve.nano_secs =  0*NANOS_PER_MS;
  UART_TASK.offset.secs = 0;
  UART_TASK.offset.nano_secs= 0;
  nrk_activate_task (&UART_TASK);

  printf ("Create done\r\n");
}

