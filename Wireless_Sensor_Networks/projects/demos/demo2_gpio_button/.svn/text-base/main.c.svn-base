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
#include <bmac.h>
#include <nrk_defs.h>
#include <nrk_error.h>


nrk_task_type TASK1;
NRK_STK task1_stack[NRK_APP_STACKSIZE];

void nrk_create_taskset ();

int main ()
{
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  nrk_time_set (0, 0);

  nrk_create_taskset ();
  nrk_start ();

  return 0;
}

void task1_workload()
{
	nrk_gpio_direction(NRK_BUTTON, NRK_PIN_INPUT);
	while (1) {
		int ret = nrk_gpio_get(NRK_BUTTON); // button is inverter:
		if (ret == 1) {
			nrk_led_clr(0);
			nrk_led_clr(1);
			nrk_led_clr(2);
			nrk_led_clr(3);
		}
		else if (ret == 0) {
			nrk_led_set(0);
			nrk_led_set(1);
			nrk_led_set(2);
			nrk_led_set(3);
		}
		nrk_gpio_set(NRK_BUTTON);

		nrk_wait_until_next_period();
	}
}

void nrk_create_taskset ()
{
	TASK1.task = task1_workload;
	nrk_task_set_stk( &TASK1, task1_stack, NRK_APP_STACKSIZE);
	TASK1.prio = 1;
	TASK1.FirstActivation = TRUE;
	TASK1.Type = BASIC_TASK;
	TASK1.SchType = PREEMPTIVE;
	TASK1.period.secs = 0;
	TASK1.period.nano_secs = 100 * NANOS_PER_MS;
	TASK1.cpu_reserve.secs = 0;
	TASK1.cpu_reserve.nano_secs = 100 * NANOS_PER_MS;
	TASK1.offset.secs = 0;
	TASK1.offset.nano_secs = 0;
	nrk_activate_task (&TASK1);
}
