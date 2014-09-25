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
*  Contributing Authors (specific to this file):
*  Zane Starr
*******************************************************************************/


#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
//#include <twi_base_calls.h>
#include "expansion.h"
#include <hal_firefly3.h>
#include <avr/interrupt.h>
#include <nrk_pin_define.h>
#include <nrk_events.h>
#include <TWI_Master.h>
#include <basic_rf.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>



#define BUFFER_SIZE 80
#define BASE_MS 200
#define MAC_ADDR        0x0003

RF_RX_INFO rfRxInfo;

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

void nrk_create_taskset ();

uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
drk_packet temp_packet;

uint8_t packet_len=0;
uint8_t packet_ready=0;

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;

char buf[BUFFER_SIZE];
//char receive_buf[BUFFER_SIZE];
int uart1_rx_signal;

void Task2(void);
void Task3(void);

void nrk_create_taskset();
void nrk_register_drivers();
uint8_t kill_stack(uint8_t val);

uint8_t nrk_uart_data_ready_gps(uint8_t uart_num);
char getc_gps();
void nrk_setup_uart_gps(uint16_t baudrate);

void process_line();

int uart_lock;


int
main ()
{
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);
  //nrk_setup_uart_gps(UART_BAUDRATE_9K6);
  nrk_setup_uart_gps(UART_BAUDRATE_115K2);

  printf( PSTR("starting...\r\n") );
  
  nrk_init();
  nrk_time_set(0,0);

  nrk_register_drivers();
  nrk_create_taskset ();
  nrk_start();
  
  return 0;
}



void rx_task ()
{
  uint8_t cnt,i,length,n;
 
  uart_lock=0; 
  rfRxInfo.pPayload = rx_buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
  rfRxInfo.ackRequest= 0;
  nrk_int_enable();
  rf_init (&rfRxInfo, 26, 0x2420, 0x1215);
  printf( "Waiting for packet...\r\n" );

  while(1){
	//nrk_gpio_clr(NRK_DEBUG_0);
	/*
	   rf_init (&rfRxInfo, 13, 0x2420, 0x1215);
	   rf_set_rx (&rfRxInfo, 13); 	
	 */
	  		DPDS1 |= 0x3;
			DDRG |= 0x1;
			PORTG |= 0x0;
			DDRE|=0xE0;
			PORTE|=0xE0;

	rf_polling_rx_on();
	while ((n = rf_rx_check_sfd()) == 0)
	  continue; 
	//nrk_gpio_set(NRK_DEBUG_0);
	if (n != 0) {
	  n = 0;
	  // Packet on its way
	  cnt=0;
	  while ((n = rf_polling_rx_packet ()) == 0) {
		if (cnt > 50) {
		  //printf( "PKT Timeout\r\n" );
		  break;		// huge timeout as failsafe
		}
		halWait(4000);
		cnt++;
	  }
	}

	//rf_rx_off();
	if (n == 1) {
	  nrk_led_toggle(RED_LED);
	  nrk_led_toggle(GREEN_LED);
	  while(uart_lock) nrk_wait_until_ticks(10);
	  uart_lock=1;
	  // CRC and checksum passed
	  //printf("packet received\r\n");
	  for(i=0; i<rfRxInfo.length; i++ )
		printf( "%c", rfRxInfo.pPayload[i]);
	  printf(",%d\r\n",rfRxInfo.rssi);
	  uart_lock=0;
	} 
	else if(n != 0){ 
	  //printf( "CRC failed!\r\n" ); nrk_led_set(RED_LED); 
	}
  }
}


void Task2()
{
  int i,n,checksum,index=0;
  uint32_t bbuf;
  char c;
  int line_cnt=0;
  //nrk_time_t cur_time,end_time,diff_time;

  printf( "My node's address is %d\r\n",NODE_ADDR );

  printf( "Task2 PID=%d\r\n",nrk_get_pid());

  while(1) {
	//nrk_time_get(&cur_time);
	
	if(nrk_uart_data_ready_gps(1)) {
	  line_cnt=0; index=0;
	  while (line_cnt!=1) {
		  if(nrk_uart_data_ready_gps(1)){
		  	c = getc_gps();
		//	printf("%c",c);
	      	buf[index++]=c;
			if(c=='\n')
			  line_cnt++;
			if(index==128)
			  index=0;
		  }
	    }
	  buf[index]='\0';

	  while(uart_lock) nrk_wait_until_ticks(10);
	  uart_lock=1;
	  printf("%s",buf);
	  uart_lock=0;
	}
	else{
	nrk_wait_until_next_period();
   }
  }

}

void
nrk_create_taskset()
{
  TaskTwo.task = Task2;
  nrk_task_set_stk( &TaskTwo, Stack2, NRK_APP_STACKSIZE);
  TaskTwo.prio = 3;
  TaskTwo.FirstActivation = TRUE;
  TaskTwo.Type = BASIC_TASK;
  TaskTwo.SchType = PREEMPTIVE;
  TaskTwo.period.secs = 0;
  TaskTwo.period.nano_secs = 50*NANOS_PER_MS; //*NANOS_PER_MS;
  TaskTwo.cpu_reserve.secs = 0;
  TaskTwo.cpu_reserve.nano_secs =  0;//200*NANOS_PER_MS;
  TaskTwo.offset.secs = 0;
  TaskTwo.offset.nano_secs= 0;
  nrk_activate_task (&TaskTwo);

  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 1;
  RX_TASK.period.nano_secs = 0;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 0 * NANOS_PER_MS;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

}

void nrk_register_drivers()
{
}


static uint16_t uart1_rx_buf_start,uart1_rx_buf_end;
static char uart1_rx_buf[128];//[MAX_RX_UART_BUF];

void nrk_setup_uart_gps(uint16_t baudrate)
{
  setup_uart1(baudrate);
 
  #ifdef NRK_UART1_BUF
  uart1_rx_signal=0;
  uart1_rx_buf_start=0;
  uart1_rx_buf_end=0;
  ENABLE_UART1_RX_INT();
  #endif
}

SIGNAL(USART1_RX_vect)
{
char c;
// cli();
DISABLE_UART1_RX_INT();
   UART1_WAIT_AND_RECEIVE(c);
   uart1_rx_buf[uart1_rx_buf_end]=c;
   //if(uart_rx_buf_end==uart_rx_buf_start) sig=1; else sig=0;
   uart1_rx_buf_end++;
   //if(uart_rx_buf_end==uart_rx_buf_start) nrk_kprintf(PSTR("Buf overflow!\r\n" ));
   if(uart1_rx_buf_end==128) {
	   uart1_rx_buf_end=0;
		   }
   uart1_rx_signal=1;
CLEAR_UART1_RX_INT();
ENABLE_UART1_RX_INT();
// sei();
}

char getc_gps()
{
  char tmp;

   //if(uart1_rx_signal <= 0) nrk_kprintf(PSTR("uart1 rx sig failed\r\n" ));
     tmp=uart1_rx_buf[uart1_rx_buf_start];
     uart1_rx_buf_start++;
   if(uart1_rx_buf_start>=128) { uart1_rx_buf_start=0; }
   uart1_rx_signal=0;

   return tmp;
}


uint8_t nrk_uart_data_ready_gps(uint8_t uart_num)
{

  if(uart_num==0) {
	if( UCSR0A & BM(RXC0) ) return 1;
  }   
  if(uart_num==1) {
	if(uart1_rx_buf_start != uart1_rx_buf_end) return 1;
	//if(uart1_rx_signal > 0) return 1;
  }

  return 0;
}

