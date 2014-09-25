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
 *  Anthony Rowe
 *******************************************************************************/


#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <slip.h>
#include <basic_rf.h>
#include <TWI_Master.h>

#define MAX_SLIP_BUF 128
#define BROADCAST_RETRY 2

#define HDR_SIZE 12
#define GPS_BUF_LEN	255
#define GPS_PRINT_BUF_LEN   255	

NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1 (void);

NRK_STK StackGPS[NRK_APP_STACKSIZE];
nrk_task_type TASK_GPS;
void TaskGPS (void);

NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type TaskThree;
void Task3 (void);

NRK_STK StackGridEYE[NRK_APP_STACKSIZE];
nrk_task_type TASK_GRIDEYE;
void TaskGridEYE (void);

uint8_t slip_rx_buf[MAX_SLIP_BUF];
uint8_t slip_tx_buf[MAX_SLIP_BUF];
uint8_t ack_buf[3];

/*RF buffers*/
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;

uint8_t rx_packet_len=0;

/*GPS functions and storage*/
uint8_t gps_print_buf[GPS_PRINT_BUF_LEN];
uint8_t nrk_uart_data_ready_gps(uint8_t uart_num);
char getc_gps();
void nrk_setup_uart_gps(uint16_t baudrate);
int uart1_rx_signal;

/*Grideye storage*/
uint8_t pixel_addr_l = 0x80;
uint8_t pixel_addr_h = 0x81;
unsigned char grideye_addr = 0x68;
uint16_t raw_therm[8][8];
unsigned char messageBuf[16];
char printbuf[128];


uint8_t uart_tx_busy=0;

void nrk_create_taskset ();

int main ()
{
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);
  //  nrk_setup_uart (UART_BAUDRATE_230K4);
  nrk_setup_uart_gps(UART_BAUDRATE_115K2);

  TWI_Master_Initialise();
  grideye_addr = 0x68;

  printf ("Starting up...\r\n");

  nrk_init ();

  nrk_led_clr (ORANGE_LED);
  nrk_led_clr (BLUE_LED);
  nrk_led_clr (GREEN_LED);
  nrk_led_clr (RED_LED);

  nrk_time_set (0, 0);
  nrk_create_taskset ();
  nrk_start ();

  return 0;
}

void Task1 ()
{
  uint16_t cnt;
  uint8_t len,i;
  //printf ("My node's address is %d\r\n", NODE_ADDR);

  //printf ("Task1 PID=%d\r\n", nrk_get_pid ());


  rfRxInfo.pPayload = rx_buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
  nrk_int_enable();
  rf_init (&rfRxInfo, 13, 0x2420, 0x1214);

  cnt = 0;
  slip_init (stdin, stdout, 0, 0);
	rf_rx_on();
  while (1) {
    


    while (rf_rx_packet_nonblock () != NRK_OK) {
      nrk_wait_until_next_period();

    }
		
		
    rx_packet_len = rfRxInfo.length;
    for(i=0; i<rfRxInfo.length; i++ ) slip_tx_buf[i]=rfRxInfo.pPayload[i];
		slip_tx_buf[10] = rfRxInfo.rssi;
		
    while(uart_tx_busy==1) nrk_wait_until_next_period();
    uart_tx_busy=1;
    slip_tx (slip_tx_buf, rx_packet_len);
    uart_tx_busy=0;
		

  }
}

void TaskGPS ()
{
  uint8_t i,n,checksum,index=0;
  char c;
  uint8_t line_cnt=0;

  printf( "Task2 PID=%d\r\n",nrk_get_pid());

  while(1) {
    if(nrk_uart_data_ready_gps(1)) {
      c = getc_gps();
      gps_print_buf[index++]=c;
      if(c=='\n') {
        gps_print_buf[index]='\0';
    		while(uart_tx_busy==1) nrk_wait_until_next_period();
      	uart_tx_busy=1;
        printf("%s\n", gps_print_buf);
      	uart_tx_busy=0;
      
        index=0;
      }
      if(index>=GPS_PRINT_BUF_LEN)
        index=0;
    }
    else{
      nrk_wait_until_next_period();
    }
  }

}

void Task3 ()
{
  int8_t v;
 
  uint8_t pckts=0;
  printf ("radio stuff Task3 PID=%d\r\n", nrk_get_pid ());


  while (slip_started () != 1)
    nrk_wait_until_next_period ();

  while (1) {

    v = slip_rx (slip_rx_buf, MAX_SLIP_BUF);
		
		printf("nanork@ bytesread %d\n",v);
		//for (i=0;i<v;i++) printf("<%d>",slip_rx_buf[i]);
		//printf("\n");

    if (v > HDR_SIZE) {

      ack_buf[0] = 'Z';
			ack_buf[1] = slip_rx_buf[1];
			ack_buf[2] = slip_rx_buf[2];
    	while(uart_tx_busy==1) nrk_wait_until_next_period();
      uart_tx_busy=1;
      slip_tx (ack_buf, 3);
      uart_tx_busy=0;
      
   

      rfTxInfo.pPayload = slip_rx_buf;
      rfTxInfo.length= v;
      rfTxInfo.destAddr = 0xFFFF;
      rfTxInfo.cca = 0;
      rfTxInfo.ackRequest = 0;

    	nrk_led_toggle (RED_LED);
      pckts++;
      PORTG=0x1;
      
      if(rf_tx_packet(&rfTxInfo) != 1) {
          printf("--- RF_TX ERROR ---\r\n");
          nrk_spin_wait_us(10000);
        } 

     
  	}

  }
}

void TaskGridEYE () {
  while (1) {
    pixel_addr_l=0x80;
    pixel_addr_h=0x81;

    int row, col;
    for (row = 0; row < 8; row++) {
      for (col = 0; col < 8; col++) {
        /* Request low byte */
        messageBuf[0] = (grideye_addr<<TWI_ADR_BITS) |
          (FALSE<<TWI_READ_BIT);
        messageBuf[1] = pixel_addr_l;
        TWI_Start_Transceiver_With_Data(messageBuf, 2);

        /* Read low byte */
        messageBuf[0] = (grideye_addr<<TWI_ADR_BITS) |
          (TRUE<<TWI_READ_BIT);
        TWI_Start_Transceiver_With_Data(messageBuf, 2);
        TWI_Get_Data_From_Transceiver(messageBuf, 2);
        uint16_t pixel_l = messageBuf[1];

        /* Request high byte */
        messageBuf[0] = (grideye_addr<<TWI_ADR_BITS) |
          (FALSE<<TWI_READ_BIT);
        messageBuf[1] = pixel_addr_h;
        TWI_Start_Transceiver_With_Data(messageBuf, 2);

        /* Read high byte */
        messageBuf[0] = (grideye_addr<<TWI_ADR_BITS) |
          (TRUE<<TWI_READ_BIT);
        TWI_Start_Transceiver_With_Data(messageBuf, 2);
        TWI_Get_Data_From_Transceiver(messageBuf, 2);
        uint16_t pixel_h = messageBuf[1];

        /* Store pixel and advance */
        raw_therm[row][col] = (pixel_h << 8) | pixel_l;
        pixel_addr_l += 2;
        pixel_addr_h += 2;
      }
      int n = sprintf(printbuf, "$GRIDEYE,%d,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X*",
          row,
          raw_therm[row][0], raw_therm[row][1],
          raw_therm[row][2], raw_therm[row][3],
          raw_therm[row][4], raw_therm[row][5],
          raw_therm[row][6], raw_therm[row][7]);
      uint8_t i;
      uint8_t checksum = 0;
      for (i = 1; i < n - 1; i++) checksum ^= printbuf[i];
      printf("%s%02X\r\n", printbuf, checksum);
    }
    nrk_wait_until_next_period();
  }
}



void nrk_create_taskset ()
{
  //priority of all 4 tasks was initially (1-4): 
  //GridEye, GPS, Task 1, Task 3
  TaskOne.task = Task1;
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 2;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 10 * NANOS_PER_MS;//changed from 5 to 150
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 0 * NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs = 0;
  nrk_activate_task (&TaskOne);

  TASK_GPS.task = TaskGPS;
  nrk_task_set_stk( &TASK_GPS, StackGPS, NRK_APP_STACKSIZE);
  TASK_GPS.prio = 3;
  TASK_GPS.FirstActivation = TRUE;
  TASK_GPS.Type = BASIC_TASK;
  TASK_GPS.SchType = PREEMPTIVE;
  TASK_GPS.period.secs = 0;
  TASK_GPS.period.nano_secs = 10 * NANOS_PER_MS; //change from 50 to 150
  TASK_GPS.cpu_reserve.secs = 0;
  TASK_GPS.cpu_reserve.nano_secs = 0;
  TASK_GPS.offset.secs = 0;
  TASK_GPS.offset.nano_secs = 0;
  nrk_activate_task (&TASK_GPS);

  TaskThree.task = Task3;
  nrk_task_set_stk( &TaskThree, Stack3, NRK_APP_STACKSIZE);
  TaskThree.prio = 2;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 0;
  TaskThree.period.nano_secs = 10 * NANOS_PER_MS;//changed from 250 to 150
  TaskThree.cpu_reserve.secs = 1;
  TaskThree.cpu_reserve.nano_secs = 0;
  TaskThree.offset.secs = 0;
  TaskThree.offset.nano_secs = 0;
  nrk_activate_task (&TaskThree);

 TASK_GRIDEYE.task = TaskGridEYE;
  nrk_task_set_stk( &TASK_GRIDEYE, StackGridEYE, NRK_APP_STACKSIZE);
  TASK_GRIDEYE.prio = 1;
  TASK_GRIDEYE.FirstActivation = TRUE;
  TASK_GRIDEYE.Type = BASIC_TASK;
  TASK_GRIDEYE.SchType = PREEMPTIVE;
  TASK_GRIDEYE.period.secs = 0;
  TASK_GRIDEYE.period.nano_secs = 150*NANOS_PER_MS; //*NANOS_PER_MS; //changed 50 to 150
  TASK_GRIDEYE.cpu_reserve.secs = 0;
  TASK_GRIDEYE.cpu_reserve.nano_secs =  0;//200*NANOS_PER_MS;
  TASK_GRIDEYE.offset.secs = 0;
  TASK_GRIDEYE.offset.nano_secs= 0;
  nrk_activate_task (&TASK_GRIDEYE);
}


static uint16_t uart1_rx_buf_start, uart1_rx_buf_end;
static char uart1_rx_buf[GPS_BUF_LEN];// store gps packets from UART1

void nrk_setup_uart_gps(uint16_t baudrate)
{
  setup_uart1(baudrate); //ulib

//in nrk_cfg.h
#ifdef NRK_UART1_BUF
  uart1_rx_signal=0;
  uart1_rx_buf_start=0;//this never moves
  uart1_rx_buf_end=0; //+1, every time we get a new byte
  ENABLE_UART1_RX_INT();
#endif
}


//TODO: signals when a new byte is in the buf (gps)
SIGNAL(USART1_RX_vect)
{
  char c;

  DISABLE_UART1_RX_INT(); //in /src/plataform/.../include/hal.h
  UART1_WAIT_AND_RECEIVE(c); //in /src/plataform/.../include/hal.h , sets C to the byte in the uart
  uart1_rx_buf[uart1_rx_buf_end]=c;
  if(uart1_rx_buf_end+1==uart1_rx_buf_start) nrk_kprintf( PSTR("GPS overflow!\r\n") );
  uart1_rx_buf_end++;
  if(uart1_rx_buf_end>=GPS_BUF_LEN)	uart1_rx_buf_end=0; 
  uart1_rx_signal = 1;
  CLEAR_UART1_RX_INT();
  ENABLE_UART1_RX_INT();

}

char getc_gps()
{
  char tmp;

  //if(uart1_rx_signal <= 0) nrk_kprintf(PSTR("uart1 rx sig failed\r\n" ));
  tmp=uart1_rx_buf[uart1_rx_buf_start];
  uart1_rx_buf_start++;
  if(uart1_rx_buf_start>=GPS_BUF_LEN) { uart1_rx_buf_start=0; }
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
