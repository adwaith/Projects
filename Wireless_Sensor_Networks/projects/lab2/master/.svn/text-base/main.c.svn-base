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
#define MAC_ADDR	(NUM_SLAVES+1)	

nrk_task_type WHACKY_TASK;
NRK_STK whacky_task_stack[NRK_APP_STACKSIZE];
void whacky_task (void);

void nrk_create_taskset ();
uint8_t whacky_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];

void nrk_register_drivers();
uint8_t num_turns = 0;
uint32_t score = 0;

int main ()
{
  uint16_t div;
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


void change_slave(uint8_t* slave_id, uint8_t* num_timeouts) {

	nrk_time_t rnd_seed;
	int rand_slave = 0; 

	do {
			nrk_time_get(&rnd_seed);
			srand(rnd_seed.nano_secs);
			rand_slave = rand();	
			rand_slave = rand_slave % NUM_SLAVES;	

    	}while(*slave_id == rand_slave);
	*slave_id = rand_slave;
	*num_timeouts = 0; 

}

int8_t process_packet(uint8_t * local_buf,uint8_t len,char *msg_type,uint8_t *slave_id,
						uint8_t* wait_for_timeout, uint8_t* num_timeouts)
{
	uint8_t pos = 0; 

	if(len <= 0)
		return NRK_ERROR;

	while(pos < len && local_buf[pos] != ':')
		pos++;
	
	if(pos + 3 >= len)
	{		
		nrk_kprintf(PSTR("Err1"));
		return NRK_ERROR;
	}

	if(local_buf[pos+2] == 'U')
	{
		
		if(pos + 5 >= len)
		{		
			nrk_kprintf(PSTR("Err2"));
			return NRK_ERROR;
		}
			
		if(local_buf[pos+5] == '0')
		{
			*wait_for_timeout = 1; 
			return NRK_OK;
		}
		else if(local_buf[pos+5] == '1')
		{
			*wait_for_timeout = 0; 
			*msg_type = 'D';
			return NRK_OK;
		}
		else 
		{		
			nrk_kprintf(PSTR("Err3"));
			return NRK_ERROR;
		}
	}
	else if(local_buf[pos+2] == 'D')
	{
		*wait_for_timeout = 0;
		if(pos+5 < len)
		{	pos = pos + 5;
			int sum = 0;
			while(pos < len && local_buf[pos] != '\0' && local_buf[pos] >='0' && local_buf[pos]<='9') 
			{
				sum *= 10;
				sum += (local_buf[pos]-'0');
				pos++;
			}
			score +=sum;
		}
		else
			nrk_kprintf(PSTR("invalid message\r\n"));	
		printf("\nYour Score so far: %u\n", score);
		num_turns++;
		if (num_turns == 10)
			return 0;
		change_slave(slave_id,num_timeouts);
		*msg_type = 'U';
 		return NRK_OK;
	}
	else	
	{		
		nrk_kprintf(PSTR("Err4"));
		return NRK_ERROR;
	}	
	
}


void whacky_task ()
{
  uint8_t i, len, fd;
  int8_t rssi, val;
  uint8_t *local_buf;
  uint16_t light;
  uint8_t slave_id,num_timeouts,rx_pkt;
  uint8_t timeout;
  uint8_t wait_for_timeout;
  char option;
  char msg_type; 
  nrk_sig_t uart_rx_signal;

  //Timer management
  nrk_time_t start_time, end_time;

  printf ("whacky_task PID=%d\r\n", nrk_get_pid ());
  
  // This shows you how to wait until a key is pressed to start
  nrk_kprintf( PSTR("Press 's' to start\r\n" ));

  // Get the signal for UART RX
  uart_rx_signal=nrk_uart_rx_signal_get();
  // Register task to wait on signal
  nrk_signal_register(uart_rx_signal); 
 // Modify this logic to start your code, when you press 's' 
  do{
  if(nrk_uart_data_ready(NRK_DEFAULT_UART))
	option=getchar();
  else nrk_event_wait(SIG(uart_rx_signal));
  } while(option!='s');
  
  nrk_kprintf( PSTR("Starting Game!!\n" ));
// init bmac on channel 15 
  bmac_init (15);

  // This sets the next RX buffer.
  // This can be called at anytime before releasing the packet
  // if you wish to do a zero-copy buffer switch
  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  // initialize the slave id and msg type
  slave_id = 0;
  msg_type = 'U';
  wait_for_timeout = 1;
  num_timeouts = 0;
 
  while (1) 
  {
    sprintf (tx_buf, "%d:L%c", slave_id, msg_type);
    nrk_led_set (BLUE_LED);
    val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
    if(val != NRK_OK)
		nrk_kprintf(PSTR("Could not Transmit!\r\n"));
    
    // Task gets control again after TX complete
    printf("Sent %d:L%c\r\n",slave_id,msg_type);
    nrk_led_clr (BLUE_LED);

    printf(PSTR("Waiting for Response\r\n"));
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);

    // Wait until an RX packet is received
	rx_pkt = 0;
    timeout = 0;
    nrk_time_get(&start_time);
    while(1)
	{
		if(bmac_rx_pkt_ready())
		{
			rx_pkt = 1; 
			local_buf = bmac_rx_pkt_get (&len, &rssi);
			printf ("Got RX packet len=%d RSSI=%d [", len, rssi);
			for (i = 0; i < len; i++)
				printf ("%c", local_buf[i]);
			printf ("]\r\n");
			nrk_led_clr (ORANGE_LED);
			val = process_packet(local_buf,len,&msg_type,&slave_id,&wait_for_timeout,&num_timeouts);
			if (val == 0)
			{
				nrk_kprintf(PSTR("GAME OVER\r\n"));
				exit(0);
			}			
			if( val != NRK_OK)
			{
				nrk_kprintf(PSTR("Error in packet received\r\n"));
				wait_for_timeout = 0; 
			}
						
			// Release the RX buffer so future packets can arrive 
			bmac_rx_pkt_release ();
			if(wait_for_timeout)
				continue;
			else
				break;
		}
		 // Implement timeouts
		nrk_time_get(&end_time);
		if(end_time.nano_secs > start_time.nano_secs)
		{
			if(((end_time.secs-start_time.secs)*1000+(end_time.nano_secs-start_time.nano_secs)/1000000) > TIMEOUT - num_turns*50)
			{
				timeout = 1;
				break;
			}
		}
		else
		{
			if(((end_time.secs-start_time.secs)*1000-(start_time.nano_secs-end_time.nano_secs)/1000000) > TIMEOUT - num_turns*50)
			{
				timeout = 1;
				break;
			}
		}
	}

    if(timeout == 1)
	{
		nrk_kprintf(PSTR("Rx Timed Out!\r\n"));
		
		if(rx_pkt == 1)
			num_timeouts = 0; 
		if(rx_pkt == 0)
			num_timeouts++;

		if(rx_pkt == 0 && num_timeouts > 2)
			change_slave(&slave_id,&num_timeouts);
		
	}
    
  }
}

void nrk_create_taskset ()
{
  WHACKY_TASK.task = whacky_task;
  nrk_task_set_stk( &WHACKY_TASK, whacky_task_stack, NRK_APP_STACKSIZE);
  WHACKY_TASK.prio = 2;
  WHACKY_TASK.FirstActivation = TRUE;
  WHACKY_TASK.Type = BASIC_TASK;
  WHACKY_TASK.SchType = PREEMPTIVE;
  WHACKY_TASK.period.secs = 2;
  WHACKY_TASK.period.nano_secs = 0;
  WHACKY_TASK.cpu_reserve.secs = 1;
  WHACKY_TASK.cpu_reserve.nano_secs = 500*NANOS_PER_MS;
  WHACKY_TASK.offset.secs = 0;
  WHACKY_TASK.offset.nano_secs = 0;
  nrk_activate_task (&WHACKY_TASK);

  printf ("Create done\r\n");
}
