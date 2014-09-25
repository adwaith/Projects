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
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <avr/iom128rfa1.h>
#include "i2cmaster.h"
#include "pcm5142.h"
//#include "data2.h"
#include "data_chirp3.h"
#include <basic_rf.h>
//#include "data_chirp1_short.h"

extern volatile uint8_t rx_ready;

RF_RX_INFO rfRxInfo;
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];

int main() {
	uint8_t n;
	uint32_t i = 0, j = 0;
	uint8_t cnt = 0;
	int16_t curr_sample;
	uint8_t data_ptr = 0;

	const __memx int16_t
	* data_pointers[3] = {data0, data1, data2};
	//const __memx int16_t* data_pointers[1] = {data0};

	nrk_setup_ports();
	nrk_setup_uart(UART_BAUDRATE_115K2);

	// Enable amp
	DDRE |= (1 << PE5) | (1 << PE6);
	PORTE |= (1 << PE5); //| (1 << PE6);

	pcm5142_init();
//  nrk_init();

// Setup timer 0
// Set PD7 (T0) as input for external clock
	DDRD |= (0 << PD7);
	// Set PB7 as output for outputting LRCLK
	DDRB |= (1 << PB7);

	// Capture compare set to 16 cycles
	OCR0A = 15;
	// Toggle OC0A on output match, CTC mode
	TCCR0A |= (0 << COM0A1) | (1 << COM0A0) | (1 << WGM01) | (0 << WGM00);
	// Select external clock T0 with no prescaling
	TCCR0B |= (1 << CS02) | (1 << CS01) | (1 << CS00) | (0 << WGM02);

	UCSR1C &= (0 << UDORD1);
	UBRR1 = 0;
	//Setting the XCK1 port pin as output, enables master mode.
	DDRD |= (1 << PE5);
	// Set MSPI mode of operation and SPI data mode 0.
	UCSR1C = (1 << UMSEL11) | (1 << UMSEL10) | (0 << UCPHA1) | (0 << UCPOL1);

	// Fast mode
	UCSR1A = (1 << U2X1);

	// Enable receiver and transmitter.
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);
	//Set baud rate. IMPORTANT: The Baud Rate must be set after the transmitter is enabled
	UBRR1 = 1;

	rfRxInfo.pPayload = rx_buf;
	rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
	rfRxInfo.ackRequest = 0;
	nrk_int_enable();
	rf_init(&rfRxInfo, 26, 0x2420, 0x1215);
	rf_rx_on();

	while (1) {
		// Poll for packet
		//UDR1 = 0;
		nrk_int_enable();
		rf_rx_on();
		while (rf_rx_packet_nonblock() == 0)
			;
		nrk_int_disable();
		rf_rx_off();

		cnt = 0;

		// First word
		curr_sample = data_pointers[data_ptr][i];

		// Turn on amp
		PORTE |= (1 << PE6);

		TCNT0 = 0; // Reset timer counter before sending

		// Start DAC in case it went to sleep
		for (j = 0; j < 800; j++) {
			UDR1 = 0;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 1;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 0;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 2;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 0;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 2;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 0;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = 1;
			while (!(UCSR1A & (1 << UDRE1)))
				;
		}
		// L
		UDR1 = (curr_sample >> 8);
		i++;
		while (!(UCSR1A & (1 << UDRE1)))
			;
		UDR1 = (curr_sample & 0xff);

		while (!(UCSR1A & (1 << UDRE1)))
			;
		// R
		UDR1 = (curr_sample >> 8);

		while (!(UCSR1A & (1 << UDRE1)))
			;
		UDR1 = (curr_sample & 0xff);

		while (1) {
			// Preload next sample
			curr_sample = data_pointers[data_ptr][i];
			//curr_sample = data0[i];

			//L
			UDR1 = (curr_sample >> 8);
			if (cnt == 30) {
				//Turn off amp
				PORTE |= (0 << PE6);
				while (!(UCSR1A & (1 << UDRE1)))
					;
				UDR1 = (curr_sample & 0xff);
				break;
			}

			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = (curr_sample & 0xff);
			i++;
			while (!(UCSR1A & (1 << UDRE1)))
				;
			// R
			UDR1 = (curr_sample >> 8);

			if (i == DATA_LENGTH) {
				if (data_ptr == 2) {
					while (!(UCSR1A & (1 << UDRE1)))
					;
					UDR1 = (curr_sample & 0xff);
					data_ptr = 0;
					i = 0;
					cnt++;
					continue;
				} else {
					data_ptr++;
					i = 0;
				}
			}
			while (!(UCSR1A & (1 << UDRE1)))
				;
			UDR1 = (curr_sample & 0xff);
		}

	}

	return 0;
}
