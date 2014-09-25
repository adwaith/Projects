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
//#include <nrk_driver_list.h>
//#include <nrk_driver.h>
//#include <ff_basic_sensor.h>
#include "adc_driver.h"

#define ADC_SETUP_DELAY  25
#define NUM_SAMPLES 15000

// This is the total number of nodes supported
#define TOTAL_NODES	3
#define MSG_SIZE	3
#define MSG_BUF_SIZE	(MSG_SIZE*TOTAL_NODES)

uint8_t channel;

#define ADC_INIT() \
    do { \
	ADCSRA = BM(ADPS2) | BM(ADPS1) ; \
	ADMUX = BM(REFS0); \
} while (0)

#define ADC_SET_CHANNEL(channel) do { ADMUX = (ADMUX & ~0x1F) | (channel); } while (0)

// Enables/disables the ADC
#define ADC_ENABLE() do { ADCSRA |= BM(ADEN); } while (0)
#define ADC_DISABLE() do { ADCSRA &= ~BM(ADEN); } while (0)

#define ADC_SAMPLE_SINGLE() \
    do { \
ADCSRA |= BM(ADSC); \
while (!(ADCSRA & 0x10)); \
} while(0)

// Macros for obtaining the latest sample value
#define ADC_GET_SAMPLE_10(x) \
do { \
x =  ADCL; \
x |= ADCH << 8; \
} while (0)

#define ADC_GET_SAMPLE_8(x) \
do { \
x = ((uint8_t) ADCL) >> 2; \
x |= ((int8_t) ADCH) << 6; \
} while (0)

void init_adc() {
// Initialize values here
	ADC_INIT ()
	;
	ADC_ENABLE ()
	;
	channel = 0;
	ADC_SET_CHANNEL(0);
}

uint16_t get_adc_val() {
	uint16_t adc_val;
	ADC_SAMPLE_SINGLE()
	;
	delay();
	ADC_GET_SAMPLE_10(adc_val);
	//Clear interrupt flag
	//ADCSRA &= ~(1<<ADIF);
	return adc_val;
}
void delay() {
	nrk_spin_wait_us(ADC_SETUP_DELAY);
}

int main() {
	uint8_t buf[NUM_SAMPLES], ready, index;
	uint16_t val, i;
	char c;

	nrk_setup_ports();
	nrk_setup_uart(UART_BAUDRATE_115K2);

	nrk_kprintf(PSTR("starting...\r\n"));

	// Init ADC
	init_adc();
	// Set channel to 0
	ADC_SET_CHANNEL(0);
	nrk_kprintf(PSTR("waiting...\r\n"));

	while (1) {
		//for (i = 0; i < 50; i++)
		//	nrk_spin_wait_us(65000);
		nrk_led_set(RED_LED);
		do {
			c = getchar();
			if (index == MSG_BUF_SIZE && c == '\r')
				ready = 1;
			if (index < MSG_BUF_SIZE)
				index++;
			else {
				index = 0;
				nrk_led_set(RED_LED);
			}
			if (c == '\r' && ready == 0) {
				index = 0;
				nrk_led_set(RED_LED);
			}
		} while (!ready);
		nrk_led_clr(RED_LED);

		for (i = 0; i < NUM_SAMPLES; i++) {
			nrk_gpio_toggle(NRK_UART1_TXD);
			val = get_adc_val();
			buf[i] = val & 0xFF;
			//printf("%u\r\n", buf[i]);
		}

		for (i = 0; i < NUM_SAMPLES; i++)
			printf("%u\r\n", buf[i]);
	}

	return 0;
}

/*void Task1() {
 uint16_t i;
 int8_t fd, val;
 uint16_t buf[128];

 printf("My node's address is %d\r\n", NODE_ADDR);

 printf("Task1 PID=%d\r\n", nrk_get_pid());

 // Open ADC device as read
 fd = nrk_open(FIREFLY_3_SENSOR_BASIC, READ);
 if (fd == NRK_ERROR)
 nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
 val = nrk_set_status(fd, SENSOR_SELECT, LIGHT);
 nrk_kprintf(PSTR("Sensor driver opened\r\n"));

 for (i = 0; i < 100; i++)
 nrk_spin_wait_us(65000);

 for (i = 0; i < 128; i++) {
 nrk_gpio_toggle(NRK_UART1_TXD);
 val = nrk_read(fd, &buf[i], 2);
 //nrk_spin_wait_us(98 );
 }

 for (i = 0; i < 128; i++)
 printf("%u\r\n", buf[i]);

 while (1)
 nrk_spin_wait_us(65000);

 }*/

