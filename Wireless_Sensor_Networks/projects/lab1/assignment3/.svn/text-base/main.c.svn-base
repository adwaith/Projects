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

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void Task2 (void);



void nrk_create_taskset();


int main ()
{
	nrk_setup_ports();
	nrk_setup_uart(UART_BAUDRATE_115K2);

	nrk_init();

	nrk_led_clr(ORANGE_LED);
	nrk_led_clr(BLUE_LED);
	nrk_led_clr(GREEN_LED);
	nrk_led_clr(RED_LED);

	nrk_kprintf( PSTR("Nano-RK Version ") );
	printf( "%d\r\n",NRK_VERSION );

	nrk_time_set(0,0);
	nrk_create_taskset ();
	nrk_start();

	return 0;
}

void calc(int loop)
{
	uint32_t i = 0, j = 0; 
	for (i = 0; i < loop; i++) {
		for (j = 0; j < 20000; j++) {
			asm volatile ("nop");
		}
	}
}

void Task1()
{
	nrk_time_t time;
	while(1) {
		nrk_led_toggle(ORANGE_LED);
		calc(30); 
		nrk_time_get(&time);
		
		printf("Task1: %lu %lu\r\n", time.secs, time.nano_secs);
		nrk_wait_until_next_period();
	}
}

void Task2()
{
	nrk_time_t time;
	while(1) {
		nrk_led_toggle(BLUE_LED);
		calc(30); 
		nrk_time_get(&time);

		printf("Task2: %lu %lu\r\n", time.secs, time.nano_secs);
		nrk_wait_until_next_period();
	}
}

void nrk_create_taskset()
{
	nrk_task_set_entry_function( &TaskOne, Task1);
	nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
	TaskOne.prio = 2;
	TaskOne.FirstActivation = TRUE;
	TaskOne.Type = BASIC_TASK;
	TaskOne.SchType = PREEMPTIVE;
	TaskOne.period.secs = 1;
	TaskOne.period.nano_secs = 0;//0*NANOS_PER_MS;
	TaskOne.cpu_reserve.secs = 1;
	TaskOne.cpu_reserve.nano_secs = 0;//0*NANOS_PER_MS;
	TaskOne.offset.secs = 0;
	TaskOne.offset.nano_secs= 0;

	nrk_task_set_entry_function( &TaskTwo, Task2);
	nrk_task_set_stk( &TaskTwo, Stack2, NRK_APP_STACKSIZE);
	TaskTwo.prio = 1;
	TaskTwo.FirstActivation = TRUE;
	TaskTwo.Type = BASIC_TASK;
	TaskTwo.SchType = PREEMPTIVE;
	TaskTwo.period.secs = 2;
	TaskTwo.period.nano_secs = 0;//0*NANOS_PER_MS;
	TaskTwo.cpu_reserve.secs = 2;
	TaskTwo.cpu_reserve.nano_secs = 0;//0*NANOS_PER_MS;
	TaskTwo.offset.secs = 0;
	TaskTwo.offset.nano_secs= 0;

	nrk_activate_task (&TaskOne);
	nrk_activate_task (&TaskTwo);
}

uint8_t kill_stack(uint8_t val)
{
	char bad_memory[10];
	uint8_t i;
	for(i=0; i<10; i++ ) bad_memory[i]=i;
	for(i=0; i<10; i++ ) printf( "%d ", bad_memory[i]);
	printf( "Die Stack %d\r\n",val );
	if(val>1) kill_stack(val-1);
	return 0;
}


