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
/*
Team 15 
Parth Mehta 
Adwaith Venkataraman
Amrita Aurora
*/


#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <bmac.h>
#include <nrk_error.h>

#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>

#define MAC_ADDR	0x0003
#define THRESHOLD	980

nrk_task_type WHACKY_TASK;
NRK_STK whacky_task_stack[NRK_APP_STACKSIZE];
void whacky_task (void);

nrk_task_type LIGHT_TASK;
NRK_STK light_task_stack[NRK_APP_STACKSIZE];
void light_task (void);

void nrk_create_taskset ();
//uint8_t whacky_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];

// the start and end time stamps
nrk_time_t start_time, end_time;

nrk_sem_t *my_semaphore;

uint8_t start_scoring,light_level;

uint8_t cmd[RF_MAX_PAYLOAD_SIZE];

void nrk_register_drivers();

uint8_t fd;

int main ()
{
//  uint16_t div;
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  nrk_time_set (0, 0);
  
  bmac_task_config ();

  nrk_register_drivers();
  nrk_create_taskset ();
  
  // Open ADC device as read 
  fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
  if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
  nrk_kprintf(PSTR("Sensor opened\r\n"));
  	
  my_semaphore = nrk_sem_create(1,4);
  if(my_semaphore==NULL) nrk_kprintf( PSTR("Error creating sem\r\n" ));
  nrk_start ();

  return 0;
}

void light_task () 
{
	int8_t val,myfd;
	uint16_t light;
	printf ("light_task PID=%d\r\n", nrk_get_pid ());
	myfd = fd;

	uint8_t measurements = 0; 
 
	while(1)
	{	
		val = nrk_sem_pend(my_semaphore);
		if(val==NRK_ERROR) nrk_kprintf( PSTR("LT error pend\r\n"));

		if(start_scoring == 0)
		{
			val = nrk_sem_post(my_semaphore);
			if(val ==NRK_ERROR) nrk_kprintf( PSTR("LT error post\r\n"));
			nrk_wait_until_next_period();
			continue;
		}
		
		val = nrk_sem_post(my_semaphore);
		if(val ==NRK_ERROR) nrk_kprintf( PSTR("LT error post\r\n"));

		val=nrk_set_status(myfd,SENSOR_SELECT,LIGHT);
		val=nrk_read(myfd,&light,2);
		measurements++;		

		if(light > THRESHOLD)
		{
			nrk_led_clr(RED_LED);
			
			val = nrk_sem_pend(my_semaphore);
			if(val==NRK_ERROR) nrk_kprintf( PSTR("LT error pend\r\n"));
			start_scoring = 0; 
			light_level = 1; 
			nrk_time_get(&end_time);
			val = nrk_sem_post(my_semaphore);
			if(val ==NRK_ERROR) nrk_kprintf( PSTR("LT error post\r\n"));
			
		}
		else
		{
			val = nrk_sem_pend(my_semaphore);
			if(val==NRK_ERROR) nrk_kprintf( PSTR("LT error pend\r\n"));
			light_level = 0; 
			val = nrk_sem_post(my_semaphore);
			if(val ==NRK_ERROR) nrk_kprintf( PSTR("LT error post\r\n"));
		}

		if(measurements > 10)
		{
			measurements = 0; 
			nrk_wait_until_next_period();
		}

	}
}



void whacky_task ()
{
	uint8_t len;
	int8_t rssi, val;
	uint8_t *local_buf;
	uint8_t pos;
	uint8_t slave_no, light_status, status, round_started,my_light_level;
	uint32_t score; 
	nrk_time_t result;  

	printf ("whacky_task PID=%d\r\n", nrk_get_pid ());
  
   
  	// init bmac on channel 15 
	bmac_init (15);
	printf(PSTR("bmac init\r\n"));
	
	srand(1);

  	// This sets the next RX buffer.
  	// This can be called at anytime before releasing the packet
  	// if you wish to do a zero-copy buffer switch
	bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);
	printf(PSTR("RX buffer set\r\n"));
  	while (1) 
  	{
    	printf(PSTR("Waiting for a Packet\r\n"));
    	// Get the RX packet 
    	//nrk_led_set (ORANGE_LED);
	    // Wait until an RX packet is received
	    if(!bmac_rx_pkt_ready())
	    {
	   	 val = bmac_wait_until_rx_pkt ();
	    }
	    local_buf = bmac_rx_pkt_get (&len, &rssi);
	    printf ("Got RX packet len=%d RSSI=%d [%s]\r\n", len, rssi, local_buf);

	    slave_no = 0;
	    pos = 0;
	    while(pos < len && local_buf[pos] != ':' && local_buf[pos] >='0' && local_buf[pos]<='9') 
		{
			slave_no *= 10;
			slave_no += (local_buf[pos]-'0');
			pos++;
		}
     	if(slave_no == MAC_ADDR)
		{
			pos += 2;
			light_status = local_buf[pos];
			//nrk_led_clr (ORANGE_LED);
    		// Release the RX buffer so future packets can arrive 
    		bmac_rx_pkt_release ();
			
			if(light_status == 'U')
			{
				if(round_started == 0)
				{
					nrk_led_set(RED_LED);
					nrk_time_get(&start_time);
					
					val = nrk_sem_pend(my_semaphore);
					if(val==NRK_ERROR) nrk_kprintf( PSTR("WT error pend\r\n"));
					start_scoring = 1; 
					light_level = 0; 
					val = nrk_sem_post(my_semaphore);
					if(val ==NRK_ERROR) nrk_kprintf( PSTR("WT error post\r\n"));
					
					round_started = 1; 
				}
				
				
				val = nrk_sem_pend(my_semaphore);
				if(val==NRK_ERROR) nrk_kprintf( PSTR("WT error pend\r\n"));
				my_light_level = light_level; 
				val = nrk_sem_post(my_semaphore);
				if(val ==NRK_ERROR) nrk_kprintf( PSTR("WT error post\r\n"));

				if(my_light_level == 1)
				{
					nrk_kprintf(PSTR("Light is 1"));
					sprintf (tx_buf, "%d:LUA:1", MAC_ADDR);
				}
				else if(my_light_level == 0)
				{
					nrk_kprintf(PSTR("Light is 0"));
					sprintf(tx_buf,"%d:LUA:0",MAC_ADDR);
				}
				else
					nrk_kprintf(PSTR("Unknown value of light level"));
			}
			else if(light_status == 'D')
			{
				round_started = 0; 
				
				val = nrk_sem_pend(my_semaphore);
				if(val==NRK_ERROR) nrk_kprintf( PSTR("WT error pend\r\n"));
				nrk_time_sub(&result,end_time,start_time);
				val = nrk_sem_post(my_semaphore);
				if(val ==NRK_ERROR) nrk_kprintf( PSTR("WT error post\r\n"));
				//calculate score and send in the tx packet; 
				score = result.secs*100 + result.nano_secs/10000000;
				sprintf(tx_buf,"%d:LDA:%d",MAC_ADDR,score);	
			}
			else
				nrk_kprintf(PSTR("Message unknown"));

			//nrk_led_set (BLUE_LED);
	    	val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
	    	if(val != NRK_OK)	
	    		nrk_kprintf(PSTR("Could not Transmit!\r\n"));
	    
	    	// Task gets control again after TX complete
		    nrk_kprintf (PSTR ("Tx task sent data!\r\n"));
	    	printf("%s\r\n", tx_buf);
	    	//nrk_led_clr (BLUE_LED);
		}
	bmac_rx_pkt_release ();
  }
}

void nrk_create_taskset ()
{
  WHACKY_TASK.task = whacky_task;
  nrk_task_set_stk( &WHACKY_TASK, whacky_task_stack, NRK_APP_STACKSIZE);
  WHACKY_TASK.prio = 1;
  WHACKY_TASK.FirstActivation = TRUE;
  WHACKY_TASK.Type = BASIC_TASK;
  WHACKY_TASK.SchType = PREEMPTIVE;
  WHACKY_TASK.period.secs = 2;
  WHACKY_TASK.period.nano_secs = 0;
  WHACKY_TASK.cpu_reserve.secs = 1;
  WHACKY_TASK.cpu_reserve.nano_secs = 0;// 500*NANOS_PER_MS;
  WHACKY_TASK.offset.secs = 0;
  WHACKY_TASK.offset.nano_secs = 0;
  nrk_activate_task (&WHACKY_TASK);

  LIGHT_TASK.task = light_task;
  nrk_task_set_stk( &LIGHT_TASK, light_task_stack, NRK_APP_STACKSIZE);
  LIGHT_TASK.prio = 2;
  LIGHT_TASK.FirstActivation = TRUE;
  LIGHT_TASK.Type = BASIC_TASK;
  LIGHT_TASK.SchType = PREEMPTIVE;
  LIGHT_TASK.period.secs = 0;
  LIGHT_TASK.period.nano_secs = 200*NANOS_PER_MS;
  LIGHT_TASK.cpu_reserve.secs = 0;
  LIGHT_TASK.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  LIGHT_TASK.offset.secs = 0;
  LIGHT_TASK.offset.nano_secs = 0;
  nrk_activate_task (&LIGHT_TASK);


  printf ("Create done\r\n");
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


