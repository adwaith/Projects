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
#include <nrk.h>


void my_callback(uint16_t global_slot );

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
//------------------------------------------------------------------------------
//      void main (void)
//
//      DESCRIPTION:
//              Startup routine and main loop
//------------------------------------------------------------------------------
int main (void)
{
    uint8_t i,length,in_rssi;
    uint32_t cnt,in_cnt;

    nrk_setup_ports(); 
    nrk_setup_uart (UART_BAUDRATE_115K2);
 
    printf( "Basic TX...\r\n" ); 
    nrk_led_set(0); 
    nrk_led_set(1); 
    nrk_led_clr(2); 
    nrk_led_clr(3); 
/*
	    while(1) {
		   
				for(i=0; i<40; i++ )
					halWait(10000);
		    nrk_led_toggle(1);

	    }

*/

    rfRxInfo.pPayload = rx_buf;
    rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
	
    nrk_int_enable();
    rf_init (&rfRxInfo, 26, 0x2420, 0x1214);
    rf_power_up();

    cnt=0;
    while(1){
		// Power AMP TX mode
	  	DPDS1 |= 0x3;
		DDRG |= 0x1;
		PORTG |= 0x1;
		DDRE|=0xE0;
		PORTE|=0xE0;

    		rfTxInfo.pPayload=tx_buf;
    		sprintf( tx_buf, "%lu,%d,%lu",in_cnt,in_rssi, cnt); 
    		rfTxInfo.length= strlen(tx_buf) + 1;
		rfTxInfo.destAddr = 0xffff;
		rfTxInfo.cca = 0;
		rfTxInfo.ackRequest = 0;
		
		printf( "Ping\r\n" );
		if(rf_tx_packet(&rfTxInfo) != 1)
		printf("--- RF_TX ERROR ---\r\n");
		cnt++;


		// Power AMP RX mode
		DPDS1   |= 0x3;
        	DDRG    |= 0x1;
        	PORTG   &= ~(0x1);
        	DDRE    |= 0xE0;
       		PORTE   |= 0xE0;	

		rf_polling_rx_on();	
				for(i=0; i<10; i++ )
					halWait(10000);
		nrk_led_toggle(RED_LED);
		if(rf_rx_packet_nonblock()==1)
			{
			in_rssi=rfRxInfo.rssi;
			sscanf(rx_buf,"%lu",&in_cnt );
			printf( "RX: %lu %d\r\n",in_cnt,in_rssi );
				
			}
		}

}


void my_callback(uint16_t global_slot )
{
		static uint16_t cnt;

		printf( "callback %d %d\n",global_slot,cnt );
		cnt++;
}



/**
 *  This is a callback if you require immediate response to a packet
 */
RF_RX_INFO *rf_rx_callback (RF_RX_INFO * pRRI)
{
    // Any code here gets called the instant a packet is received from the interrupt   
    return pRRI;
}
