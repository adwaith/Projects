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
//#include <nrk_eeprom.h>
#include <pkt.h>
#include <ade7878.h>


// if SET_MAC is 0, then read MAC from EEPROM
// otherwise use the coded value
#define SET_MAC  	0x0	
#define CHAN		26	



uint8_t aes_key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee, 0xff};



PKT_T	tx_pkt;
PKT_T	rx_pkt;

tdma_info tx_tdma_fd;
tdma_info rx_tdma_fd;

uint8_t rx_buf[TDMA_MAX_PKT_SIZE];
uint8_t tx_buf[TDMA_MAX_PKT_SIZE];

uint32_t mac_address;
uint16_t cal_done;
uint8_t send_ack;

nrk_task_type IO_TASK;
NRK_STK io_task_stack[NRK_APP_STACKSIZE];
void io_task (void);

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);


nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

// Magic memory alignment?
nrk_task_type TaskFour;

void nrk_create_taskset ();


//ADE7878 related............................

// Treat all as unsigned to make packing data easier
uint16_t period_u16;

uint32_t awatt_s32;
uint32_t ava_s32;
uint32_t avrms_s32;
uint32_t airms_s32;
uint32_t awatthr_s32;

uint32_t bwatt_s32;
uint32_t bva_s32;
uint32_t bvrms_s32;
uint32_t birms_s32;
uint32_t bwatthr_s32;

uint32_t cwatt_s32;
uint32_t cva_s32;
uint32_t cvrms_s32;
uint32_t cirms_s32;
uint32_t cwatthr_s32;



//End of ADE7878.............................






void nrk_create_taskset ();

void tdma_error(void);

uint32_t total_secs;

int main ()
{
  uint16_t div;

nrk_watchdog_enable();

  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_led_set(RED_LED); 
  nrk_kprintf(PSTR("Starting up ADE\r\n"));
  nrk_watchdog_reset();
  ade_nrk_setup ();
  nrk_watchdog_reset();  
  nrk_kprintf(PSTR("ADE Started\r\n"));


  nrk_led_set(GREEN_LED); 
  nrk_init ();
  nrk_time_set (0, 0);


  tdma_set_error_callback(&tdma_error);
  tdma_task_config();

  nrk_watchdog_reset();
  nrk_create_taskset ();
  nrk_start ();

  return 0;
}

void tdma_error(void)
{

return NRK_ERROR;
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



  tdma_init (TDMA_CLIENT, chan, (mac_address));

  tdma_aes_setkey(aes_key);
  tdma_aes_enable();

  while (!tdma_started ())
    nrk_wait_until_next_period ();

  v = tdma_tx_slot_add (mac_address&0xffff);

  if (v != NRK_OK)
    nrk_kprintf (PSTR ("Could not add slot!\r\n"));

   // setup a software watch dog timer
   t.secs=30;
   t.nano_secs=0;
   nrk_sw_wdt_init(0, &t, NULL);
   nrk_sw_wdt_start(0);

  while (1) {
    // Update watchdog timer
    nrk_sw_wdt_update(0);
    v = tdma_recv (&rx_tdma_fd, &rx_buf, &len, TDMA_BLOCKING);
    if (v == NRK_OK) {
     // printf ("src: %u\r\nrssi: %d\r\n", rx_tdma_fd.src, rx_tdma_fd.rssi);
     // printf ("slot: %u\r\n", rx_tdma_fd.slot);
     // printf ("cycle len: %u\r\n", rx_tdma_fd.cycle_size);
      v=buf_to_pkt(&rx_buf, &rx_pkt);
      if(((rx_pkt.dst_mac&0xffff) == (mac_address&0xffff)) || (rx_pkt.dst_mac==0xffff)) 
      {
	if(rx_pkt.type==PING)
	{
	   send_ack=1;
                nrk_led_clr(0);
                nrk_led_clr(1);
        if(rx_pkt.payload[0]==PING_1)
                {
                nrk_led_set(0);
                nrk_wait_until_next_period();
                nrk_wait_until_next_period();
                nrk_wait_until_next_period();
                nrk_led_clr(0);
                }
        if(rx_pkt.payload[0]==PING_2)
                {
                nrk_led_set(1);
                nrk_wait_until_next_period();
                nrk_wait_until_next_period();
                nrk_wait_until_next_period();
                nrk_led_clr(1);
                }
        if(rx_pkt.payload[0]==PING_PERSIST)
                {
                nrk_led_set(0);
                }


	}

      }


    }

  }

}

uint8_t ctr_cnt[4];

void tx_task ()
{
  uint8_t j, i, val, cnt;
  int8_t len;
  int8_t v;
  nrk_sig_t tx_done_signal;
  nrk_sig_mask_t ret;
  nrk_time_t t;


  send_ack=0;
  cal_done=0;
  printf ("tx_task PID=%d\r\n", nrk_get_pid ());

  // Wait until the tx_task starts up bmac
  // This should be called by all tasks using bmac that

   // setup a software watch dog timer
   t.secs=10;
   t.nano_secs=0;
   nrk_sw_wdt_init(0, &t, NULL);
   nrk_sw_wdt_start(0);



  while (!tdma_started ())
    nrk_wait_until_next_period ();


  ade_write8(LCYCMODE, 0x38);
  ade_write8(ACCMODE, 0x03);
  ade_write32(WTHR0, 0x00FF6A6B);
  ade_write32(WTHR1, 0x00000001);
  while (1) {


    nrk_sw_wdt_update(0);  

    // For blocking transmits, use the following function call.
    // For this there is no need to register  
    nrk_time_get(&total_secs); 
    period_u16 = ade_read16 (PERIOD);  // Freq=256000/PERIOD;

    // Phase A
    awatthr_s32= ade_read32 (AWATTHR);  // Total Active Energy
    awatt_s32= ade_read32 (AWATT);	// Active Power
    ava_s32= ade_read32 (AVA);		// Apparent power
    avrms_s32= ade_read32 (AVRMS);	// V RMS
    airms_s32= ade_read32 (AIRMS);	// I RMS

    // Phase B
    bwatthr_s32= ade_read32 (BWATTHR);  // Total Active Energy
    bwatt_s32= ade_read32 (BWATT);	// Active Power
    bva_s32= ade_read32 (BVA);		// Apparent power
    bvrms_s32= ade_read32 (BVRMS);	// V RMS
    birms_s32= ade_read32 (BIRMS);	// I RMS

    // Phase C
    cwatthr_s32= ade_read32 (CWATTHR);  // Total Active Energy
    cwatt_s32= ade_read32 (CWATT);	// Active Power
    cva_s32= ade_read32 (CVA);		// Apparent power
    cvrms_s32= ade_read32 (CVRMS);	// V RMS
    cirms_s32= ade_read32 (CIRMS);	// I RMS


   
    // Insert some data from the real meter
    tx_pkt.payload[0] = 1;  // ELEMENTS
    tx_pkt.payload[1] = 8;  // Key
    tx_pkt.payload[2] = (total_secs >> 24) & 0xff;
    tx_pkt.payload[3] = (total_secs >> 16) & 0xff;
    tx_pkt.payload[4] = (total_secs >> 8) & 0xff;
    tx_pkt.payload[5] = (total_secs) & 0xff;
    tx_pkt.payload[6] = (period_u16 >> 8) & 0xff;
    tx_pkt.payload[7] = (period_u16) & 0xff;
    tx_pkt.payload[8] = (awatt_s32 >> 24) & 0xff;
    tx_pkt.payload[9] = (awatt_s32 >> 16) & 0xff;
    tx_pkt.payload[10] = (awatt_s32 >> 8) & 0xff;
    tx_pkt.payload[11] = (awatt_s32) & 0xff;
    tx_pkt.payload[12] = (ava_s32 >> 24) & 0xff;
    tx_pkt.payload[13] = (ava_s32 >> 16) & 0xff;
    tx_pkt.payload[14] = (ava_s32 >> 8) & 0xff;
    tx_pkt.payload[15] = (ava_s32) & 0xff;
    tx_pkt.payload[16] = (avrms_s32 >> 24) & 0xff;
    tx_pkt.payload[17] = (avrms_s32 >> 16) & 0xff;
    tx_pkt.payload[18] = (avrms_s32 >> 8) & 0xff;
    tx_pkt.payload[19] = (avrms_s32) & 0xff;
    tx_pkt.payload[20] = (airms_s32 >> 24) & 0xff;
    tx_pkt.payload[21] = (airms_s32 >> 16) & 0xff;
    tx_pkt.payload[22] = (airms_s32 >> 8) & 0xff;
    tx_pkt.payload[23] = (airms_s32) & 0xff;
    tx_pkt.payload[24] = (awatthr_s32>> 24) & 0xff;
    tx_pkt.payload[25] = (awatthr_s32>> 16) & 0xff;
    tx_pkt.payload[26] = (awatthr_s32>> 8) & 0xff;
    tx_pkt.payload[27] = (awatthr_s32) & 0xff;

    tx_pkt.payload[28] = (bwatt_s32 >> 24) & 0xff;
    tx_pkt.payload[29] = (bwatt_s32 >> 16) & 0xff;
    tx_pkt.payload[30] = (bwatt_s32 >> 8) & 0xff;
    tx_pkt.payload[31] = (bwatt_s32) & 0xff;
    tx_pkt.payload[32] = (bva_s32 >> 24) & 0xff;
    tx_pkt.payload[33] = (bva_s32 >> 16) & 0xff;
    tx_pkt.payload[34] = (bva_s32 >> 8) & 0xff;
    tx_pkt.payload[35] = (bva_s32) & 0xff;
    tx_pkt.payload[36] = (bvrms_s32 >> 24) & 0xff;
    tx_pkt.payload[37] = (bvrms_s32 >> 16) & 0xff;
    tx_pkt.payload[38] = (bvrms_s32 >> 8) & 0xff;
    tx_pkt.payload[39] = (bvrms_s32) & 0xff;
    tx_pkt.payload[40] = (birms_s32 >> 24) & 0xff;
    tx_pkt.payload[41] = (birms_s32 >> 16) & 0xff;
    tx_pkt.payload[42] = (birms_s32 >> 8) & 0xff;
    tx_pkt.payload[43] = (birms_s32) & 0xff;
    tx_pkt.payload[44] = (bwatthr_s32>> 24) & 0xff;
    tx_pkt.payload[45] = (bwatthr_s32>> 16) & 0xff;
    tx_pkt.payload[46] = (bwatthr_s32>> 8) & 0xff;
    tx_pkt.payload[47] = (bwatthr_s32) & 0xff;

    tx_pkt.payload[48] = (cwatt_s32 >> 24) & 0xff;
    tx_pkt.payload[49] = (cwatt_s32 >> 16) & 0xff;
    tx_pkt.payload[50] = (cwatt_s32 >> 8) & 0xff;
    tx_pkt.payload[51] = (cwatt_s32) & 0xff;
    tx_pkt.payload[52] = (cva_s32 >> 24) & 0xff;
    tx_pkt.payload[53] = (cva_s32 >> 16) & 0xff;
    tx_pkt.payload[54] = (cva_s32 >> 8) & 0xff;
    tx_pkt.payload[55] = (cva_s32) & 0xff;
    tx_pkt.payload[56] = (cvrms_s32 >> 24) & 0xff;
    tx_pkt.payload[57] = (cvrms_s32 >> 16) & 0xff;
    tx_pkt.payload[58] = (cvrms_s32 >> 8) & 0xff;
    tx_pkt.payload[59] = (cvrms_s32) & 0xff;
    tx_pkt.payload[60] = (cirms_s32 >> 24) & 0xff;
    tx_pkt.payload[61] = (cirms_s32 >> 16) & 0xff;
    tx_pkt.payload[62] = (cirms_s32 >> 8) & 0xff;
    tx_pkt.payload[63] = (cirms_s32) & 0xff;
    tx_pkt.payload[64] = (cwatthr_s32>> 24) & 0xff;
    tx_pkt.payload[65] = (cwatthr_s32>> 16) & 0xff;
    tx_pkt.payload[66] = (cwatthr_s32>> 8) & 0xff;
    tx_pkt.payload[67] = (cwatthr_s32) & 0xff;


    tx_pkt.payload_len=68;

    tx_pkt.src_mac=mac_address;
    tx_pkt.dst_mac=0;
    tx_pkt.type=APP;

	// if there is a pending ACK, drop the application packet
        if(send_ack==1)
        {
        tx_pkt.type=ACK;
        tx_pkt.payload_len=0;
        send_ack=0;
        }


    len=pkt_to_buf(&tx_pkt,&tx_buf );
    if(len>0)
    {
    v = tdma_send (&tx_tdma_fd, &tx_buf, len, TDMA_BLOCKING);
    if (v == NRK_OK) {
      //nrk_kprintf (PSTR ("App Packet Sent\n"));
    }
    } else nrk_wait_until_next_period();

	nrk_sw_wdt_update(0);  

  }

}

void nrk_create_taskset ()
{
 
  
  
  nrk_task_set_entry_function( &TX_TASK, tx_task);
  nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
  TX_TASK.prio = 2;
  TX_TASK.FirstActivation = TRUE;
  TX_TASK.Type = BASIC_TASK;
  TX_TASK.SchType = PREEMPTIVE;
  TX_TASK.period.secs = 1;
  TX_TASK.period.nano_secs = 0;
  TX_TASK.cpu_reserve.secs = 0;
  TX_TASK.cpu_reserve.nano_secs = 0;
  TX_TASK.offset.secs = 0;
  TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_TASK);

  nrk_task_set_entry_function( &RX_TASK, rx_task);
  nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 1;
  RX_TASK.period.nano_secs = 0;
  RX_TASK.cpu_reserve.secs = 0;
  RX_TASK.cpu_reserve.nano_secs = 0;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);



}
