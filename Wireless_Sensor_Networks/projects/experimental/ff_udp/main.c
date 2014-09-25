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
//#include <bmac.h>
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



#define BUFFER_SIZE 114
#define BASE_MS 200
#define MAC_ADDR        0x0004
#define BROADCAST_TRIES 1

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_SERIAL_TASK;
NRK_STK tx_serial_task_stack[NRK_APP_STACKSIZE];
void tx_serial_task (void);
void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char serial_tx_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t packet_len=0;
uint8_t packet_ready=0;

uint8_t rx_packet_len=0;
uint8_t packet_received=0;
nrk_sig_t packet_rx_signal;


NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;

char buf[BUFFER_SIZE];
//char receive_buf[BUFFER_SIZE];
int uart1_rx_signal;

void Task1(void);

void nrk_create_taskset();
void nrk_register_drivers();
uint8_t kill_stack(uint8_t val);

uint8_t nrk_uart_data_ready_gps(uint8_t uart_num);
char getc_gps();
void nrk_setup_uart_gps(uint16_t baudrate);

uint8_t process_line();

void my_callback(uint16_t global_slot );

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;

int
main ()
{
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);
//  nrk_setup_uart_gps(UART_BAUDRATE_115K2);

  printf( PSTR("starting...\r\n") );


  nrk_init();
  nrk_time_set(0,0);

//  bmac_task_config ();

  nrk_register_drivers();
  nrk_create_taskset ();
  nrk_start();
  
  return 0;
}

void tx_serial_task ()
{
  int i;

  packet_rx_signal = nrk_signal_create();
  if(packet_rx_signal == NRK_ERROR) {
    printf("error packet_rx signal\n");
  }

  nrk_signal_register(packet_rx_signal);
    
    while(1) {
        if(packet_received == 1) {
            DISABLE_GLOBAL_INT();
            printf("$RAD:");
            for(i=0; i<rx_packet_len-2; i++ )
                printf("%c", (char)serial_tx_buf[i]);
            printf("\r\n");
//            printf("$RAD:%.*s\r\n",rx_packet_len-2,serial_tx_buf);
            packet_received = 0;
            ENABLE_GLOBAL_INT();
        }
        else
            nrk_event_wait(SIG(packet_rx_signal));
    }

}

void rx_task ()
{
    uint8_t cnt,i,length,n;
    printf( "Waiting for packet...\r\n" );

    while(1){
        DPDS1 |= 0x3;
        DDRG |= 0x1;
        PORTG = 0x0;
        DDRE|=0xE0;
        PORTE|=0xE0;
//        rf_rx_on();
//        nrk_wait_until_next_period();
//        TRX_CTRL_2 |= 0x3;
        /*
           rf_init (&rfRxInfo, 13, 0x2420, 0x1215);
           rf_set_rx (&rfRxInfo, 13);  
         */

        rf_rx_on();
        n = 0;
        // Packet on its way
        while (rf_rx_packet_nonblock () != NRK_OK) {
            nrk_wait_until_next_period();
            PORTG = 0x0;
        }
        
        // CRC and checksum passed
        /*printf("$RAD:");
          for(i=0; i<rfRxInfo.length-2; i++ )
          printf("%c", (char)rfRxInfo.pPayload[i]);
          printf("\r\n");*/
        DISABLE_GLOBAL_INT();
        packet_received = 1;
        rx_packet_len = rfRxInfo.length;
        printf("%d\n",rx_packet_len);
        memcpy(&serial_tx_buf,rfRxInfo.pPayload,rx_packet_len);
        nrk_event_signal(packet_rx_signal);
        ENABLE_GLOBAL_INT();
        nrk_wait_until_next_period();

    }


  

}

void Task1()
{
  char c;
  int i=0;
  int num_packets = 0;
  nrk_sig_t uart0_rx_signal;

  uart0_rx_signal=nrk_uart_rx_signal_get();
  if(uart0_rx_signal == NRK_ERROR) {
        printf("Error creating uart signal\n");
  }
  if(nrk_signal_register(uart0_rx_signal) != NRK_OK) {
      printf("error registering uart signal\n");
  }
  
  rfRxInfo.pPayload = rx_buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
  nrk_int_enable();
  rf_init (&rfRxInfo, 13, 0x2420, 0x1214);
  DPDS1=0x3;
  DDRG=0x1;
  PORTG=0x1;
  DDRE=0xE0;
  PORTE=0xE0;
  TRX_CTRL_2 |= 0x3;

  while(1) {
      if(nrk_uart_data_ready(NRK_DEFAULT_UART)!=0)
      {    
          c=getchar();
          //printf("%c",c);
          if(c=='$' || i > 0) {
              buf[i++]=c;
          }
          if(c == '\n')
              printf("NEWLINE\n");
          if(c=='\n' && buf[i-2]=='\r') {
              buf[i]='\0';
              packet_len = i;
              //num_packets++;
              if(process_line() == 1)
                i=0;
          }
          if(i > BUFFER_SIZE)
              i=0;
      }
      else{
          nrk_event_wait(SIG(uart0_rx_signal));
          //nrk_wait_until_next_period();
      }
  }
}

uint8_t process_line() {
    int offset=0;
    int i, checksum=0;
    int val;

    if(strncmp(buf,"$RAD:",5)==0) {
        packet_len-=5;
        memcpy(&tx_buf, &(buf[5]),packet_len);
        printf("PACKET READY\n\0");
        packet_ready=1;
    }
    else 
        return 0;

    if(packet_ready == 1) {
        // Code to control the CC2591 
        
        rfTxInfo.pPayload = tx_buf;
        rfTxInfo.length= packet_len;
        rfTxInfo.destAddr = 0xFFFF;
        rfTxInfo.cca = 0;
        rfTxInfo.ackRequest = 0;

        //      printf( "Sending\r\n" );
        for(i=0;i<BROADCAST_TRIES;i++) {
            PORTG=0x1;
            if(rf_tx_packet(&rfTxInfo) != 1)
                printf("--- RF_TX ERROR ---\r\n");
            nrk_spin_wait_us(5000);
        }
       // for(i=0; i<80; i++ )
        //    halWait(10000);
        //              nrk_led_clr(GREEN_LED);
        //for(i=0; i<10; i++ )
          //  halWait(10000);
        packet_ready = 0;
    }
    return 1;
}

void
nrk_create_taskset()
{
  TaskOne.task = Task1;
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 6;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 1;
  TaskOne.period.nano_secs = 0*NANOS_PER_MS; //*NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 0*NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);

  RX_TASK.task = rx_task;
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 3;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 0;
  RX_TASK.period.nano_secs = 10*NANOS_PER_MS;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 0;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  TX_SERIAL_TASK.task = tx_serial_task;
  nrk_task_set_stk( &TX_SERIAL_TASK, tx_serial_task_stack, NRK_APP_STACKSIZE);
  TX_SERIAL_TASK.prio = 3;
  TX_SERIAL_TASK.FirstActivation = TRUE;
  TX_SERIAL_TASK.Type = BASIC_TASK;
  TX_SERIAL_TASK.SchType = PREEMPTIVE;
  TX_SERIAL_TASK.period.secs = 0;
  TX_SERIAL_TASK.period.nano_secs = 30*NANOS_PER_MS;
  TX_SERIAL_TASK.cpu_reserve.secs = 0;
  TX_SERIAL_TASK.cpu_reserve.nano_secs = 0;
  TX_SERIAL_TASK.offset.secs = 0;
  TX_SERIAL_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_SERIAL_TASK);
}

void nrk_register_drivers()
{
}

/*
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
}*/

void my_callback(uint16_t global_slot )
{
    static uint16_t cnt;

    printf( "callback %d %d\n",global_slot,cnt );
    cnt++;
}


RF_RX_INFO *rf_rx_callback (RF_RX_INFO * pRRI)
{
        // Any code here gets called the instant a packet is received from the interrupt   
        printf("GOT PACKET\n");
            return pRRI;
}

