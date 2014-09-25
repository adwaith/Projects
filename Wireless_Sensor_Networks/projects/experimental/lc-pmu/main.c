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
#include <nrk_ext_int.h>

#define MAX_SLIP_BUF 114
#define BROADCAST_RETRY 2

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
uint8_t ack_buf[1];

/*RF buffers*/
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;

uint8_t rx_packet_len=0;

/*GPS functions and storage*/
uint8_t buf[114];
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
  uint8_t len;
  printf ("My node's address is %d\r\n", NODE_ADDR);

  printf ("Task1 PID=%d\r\n", nrk_get_pid ());
  cnt = 0;
  slip_init (stdin, stdout, 0, 0);
  
  rf_rx_on();
  while (1) {
    nrk_led_set (ORANGE_LED);
    //sprintf (slip_tx_buf, "Hello %d", cnt);
    //len = strlen (slip_tx_buf);
    // Packet on its way
    PORTG = 0x0;
    while (rf_rx_packet_nonblock () != NRK_OK) {
        nrk_wait_until_next_period();
        PORTG = 0x0;
    }
    rx_packet_len = rfRxInfo.length;
    memcpy(slip_tx_buf,rfRxInfo.pPayload,rx_packet_len);

    slip_tx (slip_tx_buf, rx_packet_len);
    //    nrk_wait_until_next_period ();
    nrk_led_clr (ORANGE_LED);
  }
}

void pps_int()
{
printf( "pps int!\r\n" );

}

void zero_cross_int()
{
printf( "zero cross int!\r\n" );

}


void TaskGPS ()
{
    int i,n,checksum,index=0;
    char c;
    int line_cnt=0,val;

    printf( "Task2 PID=%d\r\n",nrk_get_pid());

    // Set PPS as input
    nrk_gpio_direction(NRK_PORTB_5, NRK_PIN_INPUT);

    // Configure PPS interrupt on level change
    nrk_ext_int_configure(NRK_PC_INT_5, NRK_LEVEL_TRIGGER, &pps_int );
    nrk_ext_int_enable( NRK_PC_INT_5 );
   
    // Configure the 60Hz zero crossing on port D.1 which is ext_int_1 
    nrk_gpio_direction(NRK_PORTD_1, NRK_PIN_INPUT);
    nrk_ext_int_configure(NRK_EXT_INT_1, NRK_LEVEL_TRIGGER, &zero_cross_int );
    nrk_ext_int_enable( NRK_EXT_INT_1);

    while(1) {
	val=nrk_gpio_get(NRK_PC_INT_5);
	printf( "pin=%d\r\n",val );
	nrk_wait_until_next_period();
    }

    while(1) {
        if(nrk_uart_data_ready_gps(1)) {
            c = getc_gps();
            buf[index++]=c;
            if(c=='\n') {
                buf[index]='\0';
                printf(buf);
            //    slip_tx(buf,index);
                index=0;
            }
            if(index==256)
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
  int8_t i;
  uint8_t pckts=0;
  printf ("Task3 PID=%d\r\n", nrk_get_pid ());
  
  rfRxInfo.pPayload = rx_buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
  nrk_int_enable();
  rf_init (&rfRxInfo, 13, 0x2420, 0x1214);
  

  DPDS1=0x3;
  DDRG=0x1;
  PORTG=0x1;
  DDRE=0xE0;
  PORTE=0xE0;
  TRX_CTRL_2 |= 0x1;
  
  
  while (slip_started () != 1)
    nrk_wait_until_next_period ();

  while (1) {
    nrk_led_toggle (GREEN_LED);

    v = slip_rx (slip_rx_buf, MAX_SLIP_BUF);
    
    if (v > 0) {
      //nrk_kprintf (PSTR ("Task3 got data: "));
    /*  for (i = 0; i < v; i++)
        printf ("%c", slip_rx_buf[i]);
      printf ("\r\n");*/
      ack_buf[0] = 'A';
      slip_tx (ack_buf, 1);
      slip_tx (ack_buf, 1);

      rfTxInfo.pPayload = slip_rx_buf;
      rfTxInfo.length= v;
      rfTxInfo.destAddr = 0xFFFF;
      rfTxInfo.cca = 0;
      rfTxInfo.ackRequest = 0;

      pckts++;
      PORTG=0x1;
      for (i=0;i<BROADCAST_RETRY;i++) {
        if(rf_tx_packet(&rfTxInfo) != 1)
            printf("--- RF_TX ERROR ---\r\n");
            nrk_spin_wait_us(10000);
      }
    }
    else {
      ack_buf[0] = 'N';
      slip_tx (ack_buf, 1);
      nrk_spin_wait_us(1000);
      slip_tx (ack_buf, 1);
      nrk_spin_wait_us(5000);
      //nrk_kprintf (PSTR ("Task3 data failed\r\n"));
    }
    //nrk_sleep_us(10000);
    //nrk_wait_until_next_period ();
  }
}

void TaskGridEYE () {
    while (1) {
        pixel_addr_l=0x80;
        pixel_addr_h=0x81;

        int row, col, status;
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
  TaskOne.task = Task1;
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 3;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 5 * NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 0 * NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs = 0;
  nrk_activate_task (&TaskOne);

  TASK_GPS.task = TaskGPS;
  nrk_task_set_stk( &TASK_GPS, StackGPS, NRK_APP_STACKSIZE);
  TASK_GPS.prio = 2;
  TASK_GPS.FirstActivation = TRUE;
  TASK_GPS.Type = BASIC_TASK;
  TASK_GPS.SchType = PREEMPTIVE;
  TASK_GPS.period.secs = 0;
  TASK_GPS.period.nano_secs = 150 * NANOS_PER_MS;
  TASK_GPS.cpu_reserve.secs = 0;
  TASK_GPS.cpu_reserve.nano_secs = 0;
  TASK_GPS.offset.secs = 0;
  TASK_GPS.offset.nano_secs = 0;
  nrk_activate_task (&TASK_GPS);

  TaskThree.task = Task3;
  TaskThree.Ptos = (void *) &Stack3[NRK_APP_STACKSIZE - 1];
  TaskThree.Pbos = (void *) &Stack3[0];
  TaskThree.prio = 4;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 0;
  TaskThree.period.nano_secs = 250 * NANOS_PER_MS;//250 * NANOS_PER_MS;
  TaskThree.cpu_reserve.secs = 0;
  TaskThree.cpu_reserve.nano_secs = 0;//200 * NANOS_PER_MS;
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
  TASK_GRIDEYE.period.nano_secs = 50*NANOS_PER_MS; //*NANOS_PER_MS;
  TASK_GRIDEYE.cpu_reserve.secs = 0;
  TASK_GRIDEYE.cpu_reserve.nano_secs =  0;//200*NANOS_PER_MS;
  TASK_GRIDEYE.offset.secs = 0;
  TASK_GRIDEYE.offset.nano_secs= 0;
  nrk_activate_task (&TASK_GRIDEYE);
}


static uint16_t uart1_rx_buf_start,uart1_rx_buf_end;
static char uart1_rx_buf[256];//[MAX_RX_UART_BUF];

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
   if(uart1_rx_buf_end==256) {
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
   if(uart1_rx_buf_start>=256) { uart1_rx_buf_start=0; }
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
