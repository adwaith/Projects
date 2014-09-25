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
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>

NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1(void);


void nrk_create_taskset();
void nrk_register_drivers();
uint8_t kill_stack(uint8_t val);

int
main ()
{
  uint8_t t;
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);


  printf( PSTR("starting...\r\n") );

  nrk_init();
  nrk_time_set(0,0);

  nrk_register_drivers();
  nrk_create_taskset ();
  nrk_start();
  
  return 0;
}


void Task1()
{
uint16_t cnt;
int8_t i,fd,val, flag, sit_count, stand_still, walk_p_count, walk_n_count, run_p_count, run_n_count;
uint16_t buf;
uint64_t bbuf;
int16_t y_axis[100];
i = 0;

  printf( "My node's address is %d\r\n",NODE_ADDR );

  printf( "Task1 PID=%d\r\n",nrk_get_pid());

  
  	// Open ADC device as read 
  	fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
  	if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
  flag = 0;
  cnt=0;
  sit_count = 0;
  stand_still = 0;
  walk_p_count = 0;
  run_p_count = 0;
  walk_n_count = 0;
  run_n_count = 0;
  while(1) {

	// Example of setting a sensor 
//	val=nrk_set_status(fd,SENSOR_SELECT,ACC_X);
//	val=nrk_read(fd,&buf,2);
//	printf( "\nacc_x=%d",buf);
	val=nrk_set_status(fd,SENSOR_SELECT,ACC_Y);
	val=nrk_read(fd,&buf,2);
//	printf( "\nacc_y=%d\r\n",buf);
	y_axis[i] = buf - 430;
//	y_axis[i] -= 430;
	if (y_axis[i] <= 40 && y_axis[i] > -20)
		stand_still++;
	if (y_axis[i] <= 90 && y_axis[i] > 40)
		sit_count++;
	else if (y_axis[i] >= 30 && y_axis[i] <= 125)
		walk_p_count++;
	else if (y_axis[i] >= 150 && y_axis[i] <= 200)
		run_p_count++;
	else if (y_axis[i] >= -250 && y_axis[i] <= -175)
		walk_n_count++;
	else if (y_axis[i] >= -350 && y_axis[i] <= -300)
		run_n_count++; 
	
	if (i == 99)
	{
//		printf("\nsit_count %d; stand_still %d; walk_p_count %d; walk_n_count %d; run_p_count %d; run_n_count %d\r\n", sit_count, stand_still, walk_p_count, walk_n_count, run_p_count, run_n_count);
		if (sit_count > 75 && stand_still < 45 && walk_p_count < 3 && run_p_count < 3)
			flag = 1;
//			printf("\nSi\r\n");
		else if (stand_still >= 45 && walk_p_count < 3 && run_p_count < 3)
			flag = 2;
//			printf("\nS\r\n");
		else if (((walk_p_count + walk_n_count) >= 3) && ((run_p_count + run_n_count) <= 3) && (sit_count < 40))
			flag = 3;
//			printf("\nW\r\n");
		else if ((run_p_count + run_n_count) > 3)
			flag = 4;
//			printf("\nR\r\n");
		
		if (flag == 3)
			printf("\n%d:%d\r\n", flag, walk_p_count);
		else if (flag == 4)
			printf("\n%d:%d\r\n", flag, run_p_count+run_n_count);
		else
			printf("\n%d\r\n", flag);
		flag = 0;
		sit_count = 0;
		stand_still = 0;
		walk_p_count = 0;
		run_p_count = 0;
		walk_n_count = 0;
		run_n_count = 0;
		i = 0;
	}
	
	i++;
//	val=nrk_set_status(fd,SENSOR_SELECT,ACC_Z);
//	val=nrk_read(fd,&buf,2);
//	printf( " motion=%d\r\n",buf);
	//nrk_close(fd);
	nrk_wait_until_next_period();
	cnt++;
	}
}


void
nrk_create_taskset()
{
  TaskOne.task = Task1;
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 1;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 100*NANOS_PER_MS; //*NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs =  8*NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);

}

void nrk_register_drivers()
{
int8_t val;

// Register the Basic FireFly Sensor device driver
// Make sure to add: 
//     #define NRK_MAX_DRIVER_CNT  
//     in nrk_cfg.h
// Make sure to add: 
//     SRC += $(ROOT_DIR)/src/drivers/platform/$(PLATFORM_TYPE)/source/ff_basic_sensor.c
//     in makefile
val=nrk_register_driver( &dev_manager_ff3_sensors,FIREFLY_3_SENSOR_BASIC);
if(val==NRK_ERROR) nrk_kprintf( PSTR("Failed to load my ADC driver\r\n") );

}

