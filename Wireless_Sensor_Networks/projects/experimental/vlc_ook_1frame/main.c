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
// Used for the experiments on 21 Feb night - code with toggle usign timer, tasksi
// Single Freq - Task 2 commented out - Timer initialized only in the beginning

#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>



// This is the address in the msg array that this node will use
#define VLC_ADDR	1

// This is the total number of nodes supported
#define TOTAL_NODES	3


#define MSG_SIZE	3
#define MSG_BUF_SIZE	(MSG_SIZE*TOTAL_NODES)		

uint8_t msg_buf[MSG_BUF_SIZE];



	nrk_sig_t uart_rx_signal;
	nrk_sig_mask_t sm;

NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1(void);

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void Task2 (void);
/*
NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type TaskThree;
void Task3 (void);*/

uint16_t compare_vt;
uint8_t f_toggle=0;
uint8_t freq_f=0;
uint8_t led_active;

void nrk_create_taskset();

int
main ()
{
  uint8_t t;
  
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);
  
  DDRF = 0xff;
  nrk_kprintf( PSTR("Starting up...\r\n") );

  nrk_init();

 
  nrk_time_set(0,0);
  nrk_create_taskset ();
  nrk_start();
  return 0;
}

void my_timer_callback()
{

	nrk_led_toggle(ORANGE_LED);
	PORTF ^= 0xff;
	//nrk_gpio_toggle(NRK_DEBUG_0);
		if(f_toggle==0)//ON
		{
			f_toggle=1;
			switch(freq_f)
			{
				case 0://2k
					compare_vt = 4000;
					break;

				case 1://2.5k
					compare_vt = 3258;
					break;

				case 2://3k
					compare_vt = 2747;
					break;

				case 3://8k
					compare_vt = 1500;
					break;
			}
			//compare_vt = 4000;
		}
		else	//OFF
		{
			f_toggle = 0;
			//compare_vt = 4000;//334 for 3k; 500 for 2k
			switch(freq_f)
			{
				case 0://2k
					compare_vt = 4000;
				break;

				case 1://2.5k
					compare_vt = 3142;
				break;

				case 2://3k
					compare_vt = 2587;
				break;

				case 3://8k
					compare_vt = 500;
				break;

			}
		}
		OCR3AH = (compare_vt >> 8) & 0xFF;
		OCR3AL = (compare_vt & 0xFF);
		nrk_timer_int_reset(NRK_APP_TIMER_0);
}

void Task1()
{
	

  printf( "VLC address is %d\r\n",VLC_ADDR);
  printf( "Task1 PID=%d\r\n",nrk_get_pid());

		
	while(1)
		nrk_wait_until_next_period();
}

void Task2()
{
	uint16_t cnt,compare_v;
	uint8_t bcnt=0,c;
	uint8_t bit,i;
	int8_t val;
	uint8_t tx_val;
	uint8_t Nbits = 9;
	uint8_t index,ready;
	uint8_t f_high,f_low;


	

        printf( "My node's address is %d\r\n",VLC_ADDR);
	printf( "Task2 PID=%d\r\n",nrk_get_pid());
  	cnt=0;


	if(VLC_ADDR>=TOTAL_NODES) {
	nrk_kprintf( PSTR("VLC address greated than total nodes\r\n"));
	nrk_led_set(RED_LED);	
	while(1);
	}

 	// Get the signal for UART RX  
    	//uart_rx_signal=nrk_uart_rx_signal_get();
	// Register your task to wakeup on RX Data 
        //if(uart_rx_signal==NRK_ERROR) nrk_kprintf( PSTR("Get Signal ERROR!\r\n") );
        //nrk_signal_register(uart_rx_signal);
 

	val=nrk_timer_int_configure(NRK_APP_TIMER_0, 1, 1600, &my_timer_callback );// this will give a 400HZ timer int
	if(val==NRK_OK) nrk_kprintf( PSTR("Callback timer setup\r\n"));
	else nrk_kprintf( PSTR("Error setting up timer callback\r\n"));

	while(1)
	{
		
		// stop interrupt
		nrk_timer_int_stop(NRK_APP_TIMER_0);
		// turn off LED
		nrk_led_clr(ORANGE_LED);
		PORTF = 0x0;

/*
		while(nrk_uart_data_ready(NRK_DEFAULT_UART)!=0)
                	{
                	// Read Character
                	c=getchar();
                	printf( "got: %d\r\n",c);
                	}
                sm=nrk_event_wait(SIG(uart_rx_signal));
                if(sm != SIG(uart_rx_signal))
                	nrk_kprintf( PSTR("RX signal error") );
*/
		index=0;
		ready=0;


		// Polling UART receive to avoid timing overhead of using signals (up to 1ms)
		do {
		c=getchar();
		if(index==MSG_BUF_SIZE && c=='\r' ) ready=1;
		else msg_buf[index]=c;	
		if(index<MSG_BUF_SIZE) index++;
		else  { index=0; nrk_led_set(RED_LED); }
		if(c=='\r' && ready==0) { index=0; nrk_led_set(RED_LED);  }
		} while(!ready);
		nrk_led_clr(RED_LED);


		// Grab low frequency value
		f_low=msg_buf[VLC_ADDR*MSG_SIZE];
		f_high=msg_buf[VLC_ADDR*MSG_SIZE+1];
		tx_val=msg_buf[VLC_ADDR*MSG_SIZE+2];


		// Zero the timer...
		nrk_timer_int_reset(NRK_APP_TIMER_0);
		// Start the timer...
		nrk_timer_int_start(NRK_APP_TIMER_0);
		led_active=1;		
		nrk_led_toggle(BLUE_LED);


	// Send data...	
	for(bcnt=0; bcnt<Nbits; bcnt++ )
	{	
		
		switch(bcnt)
		{
		case 0:
			// preamble freq
			freq_f=0;
			break;
		case 1:
			freq_f=f_low;
			break;
		default:
			bit = (tx_val>>(Nbits-1-bcnt))&0x01;
			if(bit==1)
				freq_f = f_low;
			else
				freq_f = f_high;
		}

		//nrk_wait_until_next_period();
		

		// really terrible hack to trick OS into doing proper timing...
		_nrk_os_timer_reset();
		nrk_wait_until_ticks(33);
	}
		// After data is done, we can printf the values
		// This can't be before the data or it will impact timing...
		printf( "* f_low: %d f_high: %d value: %d\r\n",f_low,f_high,tx_val );

	}
}

/*
void Task3()
{
uint16_t cnt;
uint16_t i;
  printf( "Task3 PID=%d\r\n",nrk_get_pid());
  cnt=0;
  while(1) {
	printf( "Task3 cnt=%d\r\n",cnt );
	nrk_wait_until_next_period();
	cnt++;
	}
}*/



void
nrk_create_taskset()
{
/*  TaskOne.task = Task1;
  TaskOne.Ptos = (void *) &Stack1[NRK_APP_STACKSIZE];
  TaskOne.Pbos = (void *) &Stack1[0];
  TaskOne.prio = 1;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 250*NANOS_PER_MS;//250
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 50*NANOS_PER_MS;//50
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);
*/
  TaskTwo.task = Task2;
  TaskTwo.Ptos = (void *) &Stack2[NRK_APP_STACKSIZE];
  TaskTwo.Pbos = (void *) &Stack2[0];
  TaskTwo.prio = 2;
  TaskTwo.FirstActivation = TRUE;
  TaskTwo.Type = BASIC_TASK;
  TaskTwo.SchType = PREEMPTIVE;
  TaskTwo.period.secs = 0;
  //TaskTwo.period.nano_secs = 0;//*NANOS_PER_MS;
  TaskTwo.period.nano_secs = 33*NANOS_PER_MS;//67
  // Disable reserves
  TaskTwo.cpu_reserve.secs = 0;
  TaskTwo.cpu_reserve.nano_secs = 0;
  TaskTwo.offset.secs = 0;
  TaskTwo.offset.nano_secs= 0;
  nrk_activate_task (&TaskTwo);
/*

  TaskThree.task = Task3;
  TaskThree.Ptos = (void *) &Stack3[NRK_APP_STACKSIZE];
  TaskThree.Pbos = (void *) &Stack3[0];
  TaskThree.prio = 3;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 1;
  TaskThree.period.nano_secs = 0;
  TaskThree.cpu_reserve.secs = 0;
  TaskThree.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TaskThree.offset.secs = 0;
  TaskThree.offset.nano_secs= 0;
  nrk_activate_task (&TaskThree);*/




}
