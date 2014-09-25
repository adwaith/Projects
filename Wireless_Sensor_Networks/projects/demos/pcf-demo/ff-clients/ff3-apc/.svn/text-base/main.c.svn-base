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
#include <pcf_tdma.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_eeprom.h>
#include <pkt.h>
#include <nrk_ext_int.h>


//#define UART1


// if SET_MAC is 0, then read MAC from EEPROM
// otherwise use the coded value
#define SET_MAC  0x0000
#define CHAN	26

PKT_T	tx_pkt;
PKT_T	rx_pkt;

tdma_info tx_tdma_fd;
tdma_info rx_tdma_fd;

uint8_t rx_buf[TDMA_MAX_PKT_SIZE];
uint8_t tx_buf[TDMA_MAX_PKT_SIZE];

uint32_t mac_address;
volatile uint8_t gpio_pin;


nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);


nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type APC_TASK;
NRK_STK apc_task_stack[NRK_APP_STACKSIZE];
void apc_task (void);


void nrk_create_taskset ();

uint32_t small_count,large_count;
uint8_t buf[32];
uint8_t aes_key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee, 0xff};

int main ()
{
  uint16_t div;
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_9K6);



  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  nrk_time_set (0, 0);

  tdma_task_config();

  nrk_create_taskset ();
  nrk_start ();

  return 0;
}


void apc_task()
{
int8_t i;
uint8_t c;
uint8_t index;

#ifdef UART1
setup_uart1(UART_BAUDRATE_9K6);
#endif
index=0;
small_count=0;
large_count=0;
nrk_kprintf( PSTR("apc_Task started\r\n"));

while(1) {
#ifdef UART1
        c=getc1();
#else
	c=getc0();
#endif
        if(c!=10 && c!=13 && index<32 )
        {
                 buf[index]=c;
                 index++;
        }
        if(c==10) {
                buf[index]=0;
                sscanf(buf,"%lu,%lu",&small_count, &large_count);
                printf( "small=%lu, large=%lu\r\n",small_count,large_count );
                nrk_led_toggle(BLUE_LED);
        index=0;
        }
}


}



void rx_task ()
{
  nrk_time_t t;
  uint16_t cnt;
  int8_t v;
  uint8_t len, i;
  uint8_t chan;


  cnt = 0;
  nrk_kprintf (PSTR ("Nano-RK Version "));
  printf ("%d\r\n", NRK_VERSION);




  printf ("RX Task PID=%u\r\n", nrk_get_pid ());
  t.secs = 5;
  t.nano_secs = 0;

  chan = CHAN;
  if (SET_MAC == 0x00) {

    v = read_eeprom_mac_address (&mac_address);
    if (v == NRK_OK) {
      v = read_eeprom_channel (&chan);
      v=read_eeprom_aes_key(aes_key);
    }
    else {
      while (1) {
        nrk_kprintf (PSTR
                     ("* ERROR reading MAC address, run eeprom-set utility\r\n"));
        nrk_wait_until_next_period ();
      }
    }
  }
  else
    mac_address = SET_MAC;

  printf ("MAC ADDR: %x\r\n", mac_address & 0xffff);
  printf ("chan = %d\r\n", chan);
  len=0;
  for(i=0; i<16; i++ ) { len+=aes_key[i]; }
  printf ("AES checksum = %d\r\n", len);




  tdma_init (TDMA_CLIENT, chan, mac_address);


  tdma_aes_setkey(aes_key);
  tdma_aes_enable();

  while (!tdma_started ())
    nrk_wait_until_next_period ();

  v = tdma_tx_slot_add (mac_address&0xffff);

  if (v != NRK_OK)
    nrk_kprintf (PSTR ("Could not add slot!\r\n"));

  while (1) {
    // Update watchdog timer
    // nrk_sw_wdt_update(0);
    v = tdma_recv (&rx_tdma_fd, &rx_buf, &len, TDMA_BLOCKING);
    if (v == NRK_OK) {
     // printf ("src: %u\r\nrssi: %d\r\n", rx_tdma_fd.src, rx_tdma_fd.rssi);
     // printf ("slot: %u\r\n", rx_tdma_fd.slot);
     // printf ("cycle len: %u\r\n", rx_tdma_fd.cycle_size);
      v=buf_to_pkt(&rx_buf, &rx_pkt);
      if((rx_pkt.dst_mac&0xff) == (mac_address&0xff)) 
      {
      printf ("len: %u\r\npayload: ", len);
      for (i = 0; i < len; i++)
        printf ("%d ", rx_buf[i]);
      printf ("\r\n");




      }


    }

    //  nrk_wait_until_next_period();
  }

}

uint8_t ctr_cnt[4];

void tx_task ()
{
  uint8_t j, i, val, cnt;
  int8_t len;
  int8_t v,fd;
  uint8_t buf[2];
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t r_period;

  printf ("tx_task PID=%d\r\n", nrk_get_pid ());

  // Wait until the tx_task starts up bmac
  // This should be called by all tasks using bmac that

  while (!tdma_started ())
    nrk_wait_until_next_period ();

 


  while (1) {
    	tx_pkt.payload[0] = 1;  // ELEMENTS
   	tx_pkt.payload[1] = 7;  // Key
	tx_pkt.payload[2]=(small_count& 0xff);
	tx_pkt.payload[3]=(small_count>>8)&0xff;
	tx_pkt.payload[4]=(small_count>>16)&0xff;
	tx_pkt.payload[5]=(small_count>>24)&0xff;
	tx_pkt.payload[6]=(large_count& 0xff);
	tx_pkt.payload[7]=(large_count>>8)&0xff;
	tx_pkt.payload[8]=(large_count>>16)&0xff;
	tx_pkt.payload[9]=(large_count>>24)&0xff;
    	tx_pkt.payload_len=10;

    	tx_pkt.src_mac=mac_address;
    	tx_pkt.dst_mac=0;
    	tx_pkt.type=APP;

    	len=pkt_to_buf(&tx_pkt,&tx_buf );
    if(len>0)
    {
    v = tdma_send (&tx_tdma_fd, &tx_buf, len, TDMA_BLOCKING);
    if (v == NRK_OK) {
      //nrk_kprintf (PSTR ("App Packet Sent\n"));
    }
    } else nrk_wait_until_next_period();

  }



}

void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  nrk_task_set_stk (&RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 0;
  RX_TASK.period.nano_secs = 100 * NANOS_PER_MS;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 0;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  TX_TASK.task = tx_task;
  nrk_task_set_stk (&TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
  TX_TASK.prio = 2;
  TX_TASK.FirstActivation = TRUE;
  TX_TASK.Type = BASIC_TASK;
  TX_TASK.SchType = PREEMPTIVE;
  TX_TASK.period.secs = 1;
  TX_TASK.period.nano_secs = 0;
  TX_TASK.cpu_reserve.secs = 0;
  TX_TASK.cpu_reserve.nano_secs = 500 * NANOS_PER_MS;
  TX_TASK.offset.secs = 0;
  TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_TASK);


  APC_TASK.task = apc_task;
  nrk_task_set_stk (&APC_TASK, apc_task_stack, NRK_APP_STACKSIZE);
  APC_TASK.prio = 1;
  APC_TASK.FirstActivation = TRUE;
  APC_TASK.Type = BASIC_TASK;
  APC_TASK.SchType = PREEMPTIVE;
  APC_TASK.period.secs = 10;
  APC_TASK.period.nano_secs = 0;
  APC_TASK.cpu_reserve.secs = 0;
  APC_TASK.cpu_reserve.nano_secs = 0;
  APC_TASK.offset.secs = 0;
  APC_TASK.offset.nano_secs = 0;
  nrk_activate_task (&APC_TASK);




}


