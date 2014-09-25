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


NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1(void);

void nrk_create_taskset();

void InitSyntPorts(void)
{

	DDRD |= (_BV(PORTD3));
	PORTD &= ~_BV(PORTD3);
	DDRD &= ~_BV(PORTD2);
	DDRF |= (_BV(PORTF1));
	PORTF &= ~_BV(PORTF1);
	DDRF |= (_BV(PORTF2));
}
uint16_t Gain; 

int
main ()
{
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);

  InitSyntPorts();
  nrk_init();

  nrk_led_clr(ORANGE_LED);
  nrk_led_clr(BLUE_LED);
  nrk_led_clr(GREEN_LED);
  nrk_led_clr(RED_LED);

  nrk_time_set(0,0);
  nrk_create_taskset ();
  nrk_start();
  
  return 0;
}


//Reads the autogain
void Task1()
{
  int16_t cnt,RdVal;
  int8_t v,ClkCnt;//Keeps track of the clock pulses every period
  uint8_t RdBit;
  printf( "Task2 PID=%u\r\n",nrk_get_pid());
  cnt=0;
 
  while(1) 
  {
	nrk_led_toggle(GREEN_LED);
	RdVal = 0;

	//Generate 8 clock pulses. Read data before L>H
	//LSB received first. MSB rcvd last
	for(ClkCnt=0;ClkCnt<8;ClkCnt++)
	{
		PORTD &= ~_BV(PORTD3);
		nrk_wait_ticks(1);
		RdBit = (PIND & 0x04)>>2;
		RdVal |= (RdBit<<ClkCnt);
		PORTD |= _BV(PORTD3);
		nrk_wait_ticks(1);
	}
	Gain = RdVal;
	printf("\r\nGain = %d ",Gain);
	if(Gain>30)
	{
		nrk_led_set(RED_LED);
		nrk_led_clr(BLUE_LED);
		PORTF |= _BV(PORTF1);
		PORTF &= ~_BV(PORTF2);
	}
	else
	{
		nrk_led_clr(RED_LED);
		nrk_led_set(BLUE_LED);
		PORTF |= _BV(PORTF2);
		PORTF &= ~_BV(PORTF1);
	}

	nrk_wait_until_next_period();
	cnt--;
	}
}

void
nrk_create_taskset()
{
  nrk_task_set_entry_function( &TaskOne, Task1);
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 1;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 100*NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);


}


