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

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;
uint8_t buf[RF_MAX_PAYLOAD_SIZE];


void forward_pkt ();

int
main (void)
{
  uint16_t cnt,my_addr, other_addr, pan_id;
  uint8_t i, chan;
  int8_t n;

  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  chan = 26;

  rfRxInfo.pPayload = buf;
  rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;

  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 0;
  rfTxInfo.destAddr = 0xffff;

  nrk_int_enable ();
  rf_init (&rfRxInfo, chan, 0xffff, 0x0000);
  rf_rx_on ();

  nrk_kprintf( PSTR("pcf forwarding node...\r\n" ));
  printf( "Channel=%u\n",chan );
//  rx_start_callback(forward_pkt);
  nrk_led_set (GREEN_LED);
  // No SLEEP
  while (1)
    {

      rf_polling_rx_on ();
      do { } while((n = rf_rx_check_sfd ()) == 0);
      do {} while ((n = rf_polling_rx_packet ()) == 0);

      	  printf( "RSSI: %u, ",rfRxInfo.rssi );
  	  for(i=0; i<rfRxInfo.length; i++ ) printf( "%u ",buf[i]);
	  printf( "\r\n" );
    
    }





}

