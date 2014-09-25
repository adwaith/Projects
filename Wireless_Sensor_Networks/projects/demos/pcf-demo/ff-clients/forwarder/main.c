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


#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <basic_rf.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <pkt.h>
#include <pcf_tdma.h>

// Set USE_EEPROM to 1 to read CHAN and AES key from eeprom
// Set USE_EEPROM to 0 to overide
// MAC ADDRESS doesn't matter in eeprom config utility
#define USE_EEPROM	1
#define CHAN            26      

#define TTL_MAX 	7

// If not reading from EEPROM, set the AES key below
uint8_t aes_key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee, 0xff};


RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;
uint8_t buf[RF_MAX_PAYLOAD_SIZE];
uint8_t clear_buf[RF_MAX_PAYLOAD_SIZE];



void forward_pkt ();
uint32_t mac_address;
uint8_t len;

int
main (void)
{
  uint16_t cnt,my_addr, other_addr, pan_id;
  uint8_t i, chan,tmp_ttl,j;
  int8_t n,v;

  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

//enable watchdot
wdt_reset();
MCUSR &= ~(1<<WDRF);
WDTCSR |= (1<<WDCE) | (1<<WDE);
WDTCSR = (1<<WDE) | (1<<WDP3) | (1<<WDP0); // 8 seconds

  chan = CHAN;
  if (USE_EEPROM !=0 ) {

    v = read_eeprom_mac_address (&mac_address);
    if (v == 1) {
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

  printf ("chan = %d\r\n", chan);
  len=0;
  for(i=0; i<16; i++ ) { len+=aes_key[i]; }
  printf ("AES checksum = %d\r\n", len);



  rfRxInfo.pPayload = buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;

  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 0;
  rfTxInfo.destAddr = 0xffff;
  rfTxInfo.pPayload = buf;

  
  nrk_int_enable ();
  rf_init (&rfRxInfo, chan, 0xffff, 0x0);
 
  aes_setkey(aes_key);

 rf_rx_on ();

  nrk_kprintf( PSTR("pcf forwarding node...\r\n" ));
  printf( "Channel=%u\n",chan );
//  rx_start_callback(forward_pkt);
  nrk_led_set (GREEN_LED);

	  nrk_gpio_direction(NRK_MOSI,NRK_PIN_OUTPUT);
	  nrk_gpio_direction(NRK_MISO,NRK_PIN_OUTPUT);
  while (1)
    {
 
      rf_polling_rx_on ();
      //do { } while((n = rf_rx_check_sfd ()) == 0);
	do {} while ((n = rf_rx_packet_nonblock()) == 0);
      // FIXME: No need to wait...
      v=aes_decrypt(rfRxInfo.pPayload, rfRxInfo.length );
 
 
      tmp_ttl=rfRxInfo.pPayload[TDMA_TTL]&0xf;
	  //printf( "decrypt: %d ttl=%d\r\n",v,tmp_ttl );
      // Check if TTL is greater than 1 and smaller than a max value
      if ( (tmp_ttl > 0) && (tmp_ttl <= TTL_MAX))
  	{
     	  nrk_led_set(RED_LED);
	   // decrease TTL (high bits are protected)
  	  //rfRxInfo.pPayload[TDMA_TTL]--;
     	  rfTxInfo.length = rfRxInfo.length;
	  // Check the magic number, if bad don't retransmit 
	  if(rfRxInfo.pPayload[rfRxInfo.length-1]!= 0xCA ||
             rfRxInfo.pPayload[rfRxInfo.length-2]!= 0xFE ||
             rfRxInfo.pPayload[rfRxInfo.length-3]!= 0xBE ||
             rfRxInfo.pPayload[rfRxInfo.length-4]!= 0xEF ) continue;
	  // All is good, retransmit message
	  //printf( "tmp_ttl=%u pkt_len=%d\r\n",tmp_ttl,rfRxInfo.length );
 	  for(j=0; j<tmp_ttl; j++  )
	  {
	  rfTxInfo.pPayload[TDMA_TTL]--;
          for(i=0; i< rfTxInfo.length; i++ ) clear_buf[i]=buf[i]; 
          v=aes_encrypt(rfTxInfo.pPayload, rfTxInfo.length );
	  //printf( "encrypt: %d\r\n",v );
	  //nrk_spin_wait_us(40000);
	  nrk_gpio_set(NRK_MOSI);
     	  rf_tx_packet (&rfTxInfo);
	  nrk_gpio_clr(NRK_MOSI);
          for(i=0; i< rfTxInfo.length; i++ ) buf[i]=clear_buf[i]; 
	  }
     	  nrk_led_clr(RED_LED);
  	  //for(i=0; i<rfRxInfo.length; i++ ) printf( "%u ",buf[i]);
	  //printf( "\r\n" );
	  //printf( "TTL: %u\r\n",buf[TDMA_TTL]);
	  //printf( "SRC: %u\r\n",buf[TDMA_SRC_LOW]);
  	
	  wdt_reset();
	}
    
    }





}

