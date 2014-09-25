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
#include <nrk_error.h>

#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ff_basic_sensor.h>

#define MAC_ADDR	0x0001

nrk_task_type WHACKY_TASK;
NRK_STK whacky_task_stack[NRK_APP_STACKSIZE];
void whacky_task (void);

void nrk_create_taskset ();
uint8_t whacky_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];

uint8_t cmd[RF_MAX_PAYLOAD_SIZE];

void nrk_register_drivers();

int main ()
{
  uint16_t div;
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
  nrk_start ();

  return 0;
}

void whacky_task ()
{
  uint8_t i, len, fd;
  int8_t rssi, val;
  uint8_t *local_buf;
  uint16_t light, node_id, got_poll;
  uint8_t pos;
 
  printf ("whacky_task PID=%d\r\n", nrk_get_pid ());
  
  // Open ADC device as read 
  fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
  if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
  
    printf(PSTR("Sensor opened\r\n"));
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
    node_id = 0;
    got_poll = 0;

    printf(PSTR("Waiting for a Packet\r\n"));
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    // Wait until an RX packet is received
    if(!bmac_rx_pkt_ready())
    {
   	 val = bmac_wait_until_rx_pkt ();
    }
    local_buf = bmac_rx_pkt_get (&len, &rssi);
    printf ("Got RX packet len=%d RSSI=%d [%s]\r\n", len, rssi, local_buf);
    // Check for a poll packet
    if(len>5 && local_buf[0] == 'P' && local_buf[1] == 'O' && local_buf[2] == 'L' &&
		local_buf[3] == 'L' && local_buf[4] == ':')
    {
	// Assume that there is a space after POLL
	pos = 6;
	while(pos < len && local_buf[pos] != '\0' && local_buf[pos] >='0' && local_buf[pos]<='9') 
	{
		node_id *= 10;
		node_id += (local_buf[pos]-'0');
		pos++;
	}
	if(pos > 6)
	{
		got_poll = 1;
	}
    }
    nrk_led_clr (ORANGE_LED);
    // Release the RX buffer so future packets can arrive 
    bmac_rx_pkt_release ();
    
    if(got_poll == 1 && node_id == MAC_ADDR)
    {    
	    val=nrk_set_status(fd,SENSOR_SELECT,LIGHT);
	    val=nrk_read(fd,&light,2);
    
	    sprintf (tx_buf, "Node %d Status %u", MAC_ADDR, light);
	    nrk_led_set (BLUE_LED);
	    val=bmac_tx_pkt(tx_buf, strlen(tx_buf)+1);
	    if(val != NRK_OK)	
	    {
		nrk_kprintf(PSTR("Could not Transmit!\r\n"));
	    }

	    // Task gets control again after TX complete
	    nrk_kprintf (PSTR ("Tx task sent data!\r\n"));
	    printf("%s\r\n", tx_buf);
	    nrk_led_clr (BLUE_LED);
   }
  }
}

void nrk_create_taskset ()
{
  WHACKY_TASK.task = whacky_task;
  nrk_task_set_stk( &WHACKY_TASK, whacky_task_stack, NRK_APP_STACKSIZE);
  WHACKY_TASK.prio = 2;
  WHACKY_TASK.FirstActivation = TRUE;
  WHACKY_TASK.Type = BASIC_TASK;
  WHACKY_TASK.SchType = PREEMPTIVE;
  WHACKY_TASK.period.secs = 2;
  WHACKY_TASK.period.nano_secs = 0;
  WHACKY_TASK.cpu_reserve.secs = 2;
  WHACKY_TASK.cpu_reserve.nano_secs = 0;
  WHACKY_TASK.offset.secs = 0;
  WHACKY_TASK.offset.nano_secs = 0;
  nrk_activate_task (&WHACKY_TASK);

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
